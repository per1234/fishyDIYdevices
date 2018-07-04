//------------------------------------------------------------------------------
//--------------------------------------- WiFi----------------------------------
// -----------------------------------------------------------------------------
void WiFiSetup()
{
	//reset saved settings (for testing)--------------------------------------------
	//WiFiManager.resetSettings();
	//------------------------------------------------------------------------------

	//if SSID and Password haven't been saved from before this opens an AP from the device
	//allowing you to connect to the device from a phone/computer by joining its "network"
	//from your wifi list - the name of the network will be the device name.
	//After first configuration it will auto connect unless things fail and it needs to be reset
	WiFiManager.autoConnect(EEPROMdata.namestr);

	if (DEBUG_MESSAGES)
	{
		Serial.println();
	}

	// Connected!
	if (DEBUG_MESSAGES)
	{
		Serial.printf("[WiFi] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
	}

	Udp.begin(UDP_LOCAL_PORT); //start listening on UDP port for node-node traffic
	UDPbroadcast();			   //get a poll going

	String hostName;
	if (EEPROMdata.master)
	{
		hostName = "fishyDIY";
	}
	else
	{
		hostName = "fishyDIYNode" + String(WiFi.localIP()[3]);
	}
	MDNS.begin(hostName.c_str());					  //start mDNS to fishyDIYmaster.local
	httpServer.on("/genericArgs", handleGenericArgs); //Associate the handler function to the path
	httpServer.on("/", handleRoot);
	httpServer.on("/SWupdater", handleSWupdater);
	httpServer.on("/SWupdateDevForm", HTTP_GET, handleSWupdateDevForm);
	httpServer.on("/SWupdateDevForm", HTTP_POST, handleSWupdateDevPostDone,  handleSWupdateDevPost);
	httpServer.on("/device.JSON", handleJSON);
	httpServer.onNotFound(handleNotFound);
	//httpUpdater.setup(&httpServer);
	httpServer.begin();

	MDNS.addService("http", "tcp", 80);
	fastBlinks(5);
	if (DEBUG_MESSAGES)
	{
		Serial.printf("%s is now ready! Open http://%s.local/update in your browser\n", EEPROMdata.namestr, hostName.c_str());
	}
}

//-----------------------------------------------------------------------------
//-------------------------Web Server Functions--------------------------------
//-----------------------------------------------------------------------------

 String _updaterError;

//provide a JSON structure with all the deviceArray data
void handleJSON()
{
	//TODO - recomment out when done testing
	//if (EEPROMdata.master || (masterIP.toString()=="0.0.0.0"))
	//{
		UDPbroadcast();
		//TODO - IF NEW JSON DEVICE NOT IN THE CONTROL THEN CTRL PANEL SHOULD BE TOLD TO REFRESH
		httpServer.send(200, "text/html", getJSON().c_str());
	//}
	//else
	//{
	//	handleNotMaster();
	//}
}

void handleNotMaster()
{
	String ipToShow = masterIP.toString();
	if(ipToShow == "0.0.0.0") {
		ipToShow = WiFi.localIP().toString();
	}
	String response = " <!DOCTYPE html> <html> <head> <title>fishDIY Device Network</title> <style> body {background-color: #edf3ff;font-family: \"Lucida Sans Unicode\", \"Lucida Grande\", sans-serif;}  .fishyHdr {align:center; border-radius:25px;font-size: 18px;     font-weight: bold;color: white;background: #3366cc; vertical-align: middle; } </style>  <body> <div class=\"fishyHdr\">Go to <a href=\"http://" + ipToShow + "\"> Master (" + ipToShow + ")</a></div> </body> </html> ";
	httpServer.send(200, "text/html", response.c_str());
}

String getJSON()
{
	String temp;
	temp = "{\"fishyDevices\":[";
	for (int i = 0; i < MAX_DEVICE; i++)
	{
		if (!deviceArray[i].dead)
		{
			if (i > 0)
			{
				temp += ",";
			}
/* 
put fishyDevice data in a string.
Note - this is parsed by scripts in webresources.h 
and paralleled by UDPpollReply; 
if adding new data elements all these functions need updating. Current dataset (in order) is:
{ip,isCalibrated,isMaster,motorPos,name,openIsCCW,port,group,note,swVer,devType,initStamp}
*/
			temp += "{\"deviceID\":\"" + String(i) + "\",\"IP\":\"" + deviceArray[i].ip.toString() + "\",\"dead\":\"" + String(deviceArray[i].dead) +
					"\",\"isCalibrated\":\"" + String(deviceArray[i].isCalibrated ? "true" : "false") +
					"\",\"isMaster\":\"" + String(deviceArray[i].isMaster ? "true" : "false") + "\",\"motorPos\":\"" + String(deviceArray[i].motorPos) +
					"\",\"deviceName\":\"" + String(deviceArray[i].name) + "\",\"openIsCCW\":\"" + String(deviceArray[i].openIsCCW ? "true" : "false") +
					"\",\"port\":\"" + String(deviceArray[i].port) + "\",\"group\":\"" + String(deviceArray[i].group) + "\",\"note\":\"" + String(deviceArray[i].note) + "\",\"swVer\":\"" + String(deviceArray[i].swVer) + "\",\"devType\":\"" + String(deviceArray[i].devType) + "\",\"initStamp\":\"" + String(deviceArray[i].initStamp) + "\",\"range\":\"" + String(deviceArray[i].range) + "\"}";
		}
	}
	temp += "]}";
	//if(DEBUG_MESSAGES){Serial.println("[getJSON] built: " + temp);}
	return temp;
}

void handleGenericArgs()
{ //Handler
	if (EEPROMdata.master || (masterIP.toString()=="0.0.0.0"))
	{
		IPAddress IPtoSend;
		String IPforCommand = "";
		char command[MAXCMDSZ] = "";
		String message = "Number of args received:";
		message += httpServer.args(); //Get number of parameters
		message += "\n";			  //Add a new line
		if (DEBUG_MESSAGES)
		{
			Serial.println("[handleGenericArgs] :" + message);
		}
		for (int i = 0; i < httpServer.args(); i++)
		{
			message += "Arg #" + String(i) + " -> "; //Include the current iteration value
			message += httpServer.argName(i) + ": "; //Get the name of the parameter
			message += httpServer.arg(i) + "\n";	 //Get the value of the parameter

			if (httpServer.argName(i) == "IPCommand")
			{
				String response = httpServer.arg(i);

				//IP
				int strStrt = 0;
				int strStop = response.indexOf(";", strStrt);
				String strIP = response.substring(strStrt, strStop);
				IPtoSend[3] = strIP.substring(strIP.lastIndexOf(".") + 1).toInt();
				strIP = strIP.substring(0, strIP.lastIndexOf("."));
				IPtoSend[2] = strIP.substring(strIP.lastIndexOf(".") + 1).toInt();
				strIP = strIP.substring(0, strIP.lastIndexOf("."));
				IPtoSend[1] = strIP.substring(strIP.lastIndexOf(".") + 1).toInt();
				strIP = strIP.substring(0, strIP.lastIndexOf("."));
				IPtoSend[0] = strIP.toInt();
				response = response.substring(strStop + 1);
				if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
				{
					Serial.println("[handleGenericArgs] strIP:response " + IPtoSend.toString() + ":" + response);
				}
				message += "[handleGenericArgs] strIP:response " + IPtoSend.toString() + ":" + response;
				
				response.toCharArray(command, MAXCMDSZ);
				if (IPtoSend == WiFi.localIP())
				{
					if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
					{
						Serial.println("[handleGenericArgs] MASTER node processing configuration change...");
					}
					executeCommands(command, WiFi.localIP());
				}
				else
				{
					if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
					{
						Serial.println("[handleGenericArgs] Sending slave node configuration change...");
					}
					Udp.beginPacket(IPtoSend, UDP_LOCAL_PORT);
					Udp.write(response.c_str());
					Udp.endPacket();
				}
			}
		}
		httpServer.send(200, "text/plain", message); //Response to the HTTP request
	}
	else
	{
		handleNotMaster();
	}
}
void handleRoot()
{
	//only process the webrequest if you are the master or if there is no master
	if (EEPROMdata.master || (masterIP.toString()=="0.0.0.0"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[handleRoot] 1: ");
		}

		int szchnk = 2900;

		//LARGE STRINGS - Break response into parts
		httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
		httpServer.sendHeader("Pragma", "no-cache");
		httpServer.sendHeader("Expires", "-1");
		httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN); 
		httpServer.send(200, "text/html", "");

		//Send PART1:
		handleStrPartResp(WEBSTRPT1,szchnk);
		
		//Send JSON:
		handleStrPartResp(String("<script>var fishyNetJSON ='" + getJSON() + "';</script>"),szchnk);
		
		//Send PART2:
		handleStrPartResp(WEBSTRPT2,szchnk);

		//Send PART3: - this is the svg which is really optional
		handleStrPartResp(WEBSTRPT3,szchnk);

		//Send PART4:
		handleStrPartResp(WEBSTRPT4,szchnk);
		
		//Send PART5:
		handleStrPartResp(WEBSTRPT5,szchnk);
		
		httpServer.sendContent("");
		httpServer.client().stop();

		if (DEBUG_MESSAGES)
		{
			Serial.println("[handleRoot]\n");
		}
	}
	else
	{
		handleNotMaster();
	}
}

//show the SW update form for the specifc device (function for every device webserver)
void handleSWupdateDevForm()
{
 int szchnk = 2900;
  //ESP8266WebServer *_server = &httpServer;
  //build the device info string
 String WEBSTR_SWUPDATE_DEVICE_INFO = "Type: " + String(EEPROMdata.typestr) + "<br>Software Version:" + String(EEPROMdata.swVer) + "<br>Initialization String:" + String(EEPROMdata.initstr) + "<br>";

//open server response
httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
httpServer.sendHeader("Pragma", "no-cache");
httpServer.sendHeader("Expires", "-1");
httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN); 
httpServer.send(200, "text/html", "");


//send content in chunks
handleStrPartResp(WEBSTR_SWUPDATE_PT1,szchnk);
handleStrPartResp(WEBSTR_SWUPDATE_DEVICE_INFO,szchnk);
handleStrPartResp(WEBSTR_SWUPDATE_PT2,szchnk);

//close server response
httpServer.sendContent("");
httpServer.client().stop();

}

void setUpdaterError()
{
  if (DEBUG_MESSAGES) Update.printError(Serial);
  StreamString str;
  Update.printError(str);
  _updaterError = str.c_str();
}

void handleSWupdateDevPostDone()
{
 	if (Update.hasError()) {
        httpServer.send(200, F("text/html"), String(F("Update error: ")) + _updaterError);
    } else {
        httpServer.client().setNoDelay(true);
        httpServer.send_P(200, PSTR("text/html"),  WEBSTR_SWUPDATE_SUCCESS);
        delay(100);
        httpServer.client().stop();
        ESP.restart();
    }
}

void handleSWupdateDevPost()
{
	 HTTPUpload& upload = httpServer.upload();

      if(upload.status == UPLOAD_FILE_START){

        WiFiUDP::stopAll();
        if (DEBUG_MESSAGES)
          Serial.printf("Update: %s\n", upload.filename.c_str());
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if(!Update.begin(maxSketchSpace)){//start with max available size
          setUpdaterError();
        }
      } else if( upload.status == UPLOAD_FILE_WRITE && !_updaterError.length()){
        if (DEBUG_MESSAGES) Serial.printf(".");
        if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
          setUpdaterError();
        }
      } else if(upload.status == UPLOAD_FILE_END && !_updaterError.length()){
        if(Update.end(true)){ //true to set the size to the current progress
          if (DEBUG_MESSAGES) Serial.printf("Update Was Successful: %u\nRebooting...\n", upload.totalSize);
        } else {
          setUpdaterError();
        }
        if (DEBUG_MESSAGES) Serial.setDebugOutput(false);
      } else if(upload.status == UPLOAD_FILE_ABORTED){
        Update.end();
        if (DEBUG_MESSAGES) Serial.println("Update was aborted");
      }
      delay(0);
}
//show the SW update webasite for all the fishyDIY devices in the network (MASTER function)
void handleSWupdater()
{
	//only process the webrequest if you are the master or if there is no master
	if (EEPROMdata.master || (masterIP.toString()=="0.0.0.0"))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[handleSWupdater] 1: ");
		}

		int szchnk = 2900;

		//LARGE STRINGS - Break response into parts
		httpServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
		httpServer.sendHeader("Pragma", "no-cache");
		httpServer.sendHeader("Expires", "-1");
		httpServer.setContentLength(CONTENT_LENGTH_UNKNOWN); 
		httpServer.send(200, "text/html", "");

		//Send PART1:
		handleStrPartResp(WEBSTRPT1,szchnk);
		
		//Send JSON:
		handleStrPartResp(String("<script>var fishyNetJSON ='" + getJSON() + "';</script>"),szchnk);
		
		//Send PART2ALT:
		handleStrPartResp(WEBSTRPT2ALT,szchnk);

		//Skip PART3 Send PART4ALT:
		handleStrPartResp(WEBSTRPT4ALT,szchnk);
		
		//Send PART5:
		handleStrPartResp(WEBSTRPT5,szchnk);
		
		httpServer.sendContent("");
		httpServer.client().stop();

		if (DEBUG_MESSAGES)
		{
			Serial.println("[handleSWupdater]\n");
		}
	}
	else
	{
		handleNotMaster();
	}
}

//send a str that is part of a webresponse and break it into chunks of szchnk
void handleStrPartResp(String str,int szchnk){
	int strt = 0;
	int stp = 0;
	int len = str.length();
	if (DEBUG_MESSAGES)	{Serial.println("[handleRoot] PT1 str.len: " + String(len));}
	while (stp < len)
	{
		strt = stp;
		stp = strt + szchnk;
		if (stp > len)
		{
			stp = len;
		}
		httpServer.sendContent(str.substring(strt, stp).c_str());
		if (DEBUG_MESSAGES)
		{
			Serial.println(str.substring(strt, stp));
		}
	}
}

//web server response to other
void handleNotFound()
{
	handleNotMaster();
}
