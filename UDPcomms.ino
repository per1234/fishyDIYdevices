//process UDP packets
void UDPprocessPacket()
{
	//USED FOR UDP COMMS
	char packetBuffer[MAXCMDSZ]; //buffer to hold incoming packet

	// if there's data available, read a packet
	int packetSize = Udp.parsePacket();
	if (packetSize)
	{
		IPAddress remoteIp = Udp.remoteIP();
		if (DEBUG_MESSAGES)
		{
			Serial.print("[UDPprocessPacket] Received packet of size ");
			Serial.println(packetSize);
			Serial.print("[UDPprocessPacket] From ");
			Serial.print(remoteIp);
			Serial.print(", port ");
			Serial.println(Udp.remotePort());
		}
		// read the packet into packetBufffer
		int len = Udp.read(packetBuffer, MAXCMDSZ);
		if (len > 0)
		{
			packetBuffer[len] = 0;
		}
		if (DEBUG_MESSAGES)
		{
			Serial.print("[UDPprocessPacket] Executing:");
			Serial.println(packetBuffer);
		}
		executeCommands(packetBuffer, remoteIp);
	}
}

//broadcast on subnet to see who will respond
void UDPbroadcast()
{
	IPAddress broadcast = WiFi.localIP();
	broadcast[3] = 255;

	//process this devices data first, storing it in the deviceArray, then poll the rest of the network
	dealWithThisNode(makeMyFishyDevice());

	Udp.beginPacket(broadcast, UDP_LOCAL_PORT);
	Udp.write("ANYFISHYDEV_THERE");
	//UDPdataDump();
	Udp.endPacket();
}

void UDPannounceMaster()
{
	IPAddress broadcast = WiFi.localIP();
	broadcast[3] = 255;
	Udp.beginPacket(broadcast, UDP_LOCAL_PORT);
	Udp.write("FISHYDIYMASTER");
	Udp.endPacket();
}

//put out this devices data on the net
void UDPpollReply(IPAddress remote)
{

	fishyDevice holder; //temp
	holder = makeMyFishyDevice();

	String response = "POLL_RESPONSE ";

	Udp.beginPacket(remote, UDP_LOCAL_PORT);

/* 
send fishyDevice data.
Note - this is parsed by UDPparsePollResponse and paralleled by getJSON; 
if adding elements all three need updating.
{ip,isCalibrated,isMaster,motorPos,motorPosAtCCW,motorPosAtCW,motorPosAtFullCCW,motorPosAtFullCW,name,openIsCCW,port,group,note,swVer,devType}
*/
	response += "{" + holder.ip.toString() + "," + 
			String(holder.isCalibrated ? "true" : "false") + "," +
			String(holder.isMaster ? "true" : "false") + "," + 
			String(holder.motorPos) + "," +
			String(holder.motorPosAtCCW) + "," + 
			String(holder.motorPosAtCW) + "," + 
			String(holder.motorPosAtFullCCW) + "," +
			String(holder.motorPosAtFullCW) + "," + 
			String(holder.name) + ","  +  
			String(holder.openIsCCW ? "true" : "false") + "," + 
			String(holder.port) + "," + 
			String(holder.group) + "," +
			String(holder.note) + "," + 
			String(holder.swVer) + "," + 
			String(holder.devType) + "}";

	Udp.write(response.c_str());
	Udp.endPacket();

	if (EEPROMdata.master)
	{
		UDPannounceMaster();
	} //make sure they know who is in charge
}

//parses a UDP poll reply and takes action
//TODO - parse the response further and add new nodes to the list and drop off old nodes
void UDPparsePollResponse(String responseIn, IPAddress remote)
{
	if (EEPROMdata.master )
	{
/* 
parse fishyDevice data.
Note - this data set is sent by UDPparsePollResponse and getJSON; 
it is also parsed by scripts in webresources.h if adding elements all three need updating:
{ip,isCalibrated,isMaster,motorPos,motorPosAtCCW,motorPosAtCW,motorPosAtFullCCW,motorPosAtFullCW,name,openIsCCW,port,group,note,swVer,devType}
*/
		String response = responseIn.substring(14); //strip off "POLL RESPONSE"
		fishyDevice holder;
		holder.dead = false;

		//IP
		int strStrt = 1;
		int strStop = response.indexOf(",", strStrt);
		String strIP = response.substring(strStrt, strStop);
		holder.ip[3] = strIP.substring(strIP.lastIndexOf(".") + 1).toInt();
		strIP = strIP.substring(0, strIP.lastIndexOf("."));
		holder.ip[2] = strIP.substring(strIP.lastIndexOf(".") + 1).toInt();
		strIP = strIP.substring(0, strIP.lastIndexOf("."));
		holder.ip[1] = strIP.substring(strIP.lastIndexOf(".") + 1).toInt();
		strIP = strIP.substring(0, strIP.lastIndexOf("."));
		holder.ip[0] = strIP.toInt();
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strIP: " + holder.ip.toString());
		}

		//isCalibrated
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.isCalibrated = (response.substring(strStrt, strStop) == "false") ? false : true;
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.print("[UDPparsePollResponse] isCalibrated: ");
			Serial.println(holder.isCalibrated ? "true" : "false");
		}

		//isMaster
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.isMaster = (response.substring(strStrt, strStop) == "false") ? false : true;
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.print("[UDPparsePollResponse] isMaster: ");
			Serial.println(holder.isMaster ? "true" : "false");
		}

		//motorPos
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.motorPos = response.substring(strStrt, strStop).toInt();
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] motorPos: " + String(holder.motorPos));
		}

		//motorPosAtCCW
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.motorPosAtCCW = response.substring(strStrt, strStop).toInt();
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] motorPosAtCCW: " + String(holder.motorPosAtCCW));
		}

		//motorPosAtCW
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.motorPosAtCW = response.substring(strStrt, strStop).toInt();
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strMotorPosAtCW: " + String(holder.motorPosAtCW));
		}

		//motorPosAtFullCCW
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.motorPosAtFullCCW = response.substring(strStrt, strStop).toInt();
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strMotorPosAtFullCCW: " + String(holder.motorPosAtFullCCW));
		}

		//motorPosAtFullCW
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.motorPosAtFullCW = response.substring(strStrt, strStop).toInt();
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strMotorPosAtFullCW: " + String(holder.motorPosAtFullCW));
		}

		//name
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.name = response.substring(strStrt, strStop);
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strName: " + holder.name);
		}

		//openIsCCW
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.openIsCCW = (response.substring(strStrt, strStop) == "false") ? false : true;
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.print("[UDPparsePollResponse] openIsCCW: ");
			Serial.println(holder.openIsCCW ? "true" : "false");
		}

		//port
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.port = response.substring(strStrt, strStop).toInt(); 
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strPort: " + String(holder.port));
		}

		//group
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.group = response.substring(strStrt, strStop); 
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strGroup: " + holder.group);
		}

		//note
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.note = response.substring(strStrt, strStop);
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strnote: " + holder.note);
		}

		//swVer
		strStrt = strStop + 1;
		strStop = response.indexOf(",", strStrt);
		holder.swVer = response.substring(strStrt, strStop); 
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strswVer: " + holder.swVer);
		}

		//devType
		strStrt = strStop + 1;
		strStop = response.indexOf("}", strStrt);
		holder.devType = response.substring(strStrt, strStop); 
		if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
		{
			Serial.println("[UDPparsePollResponse] strdevType: " + holder.devType);
		}

		dealWithThisNode(holder);
	}
}

void UDPparseConfigResponse(String responseIn, IPAddress remote){
	String response = responseIn.substring(7); //strip off "CONFIG"
	int strStrt, strStop;

/*
		parseString in this order: {ccwLim, cwLim, openIsCCW, isMaster, devName, groupName, devType, note}
*/

	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[UDPparseConfigResponse] Got this: " + responseIn);
	}	
	//ccwLim
	strStrt = response.indexOf("=", 1)+1;
	strStop = response.indexOf(";", strStrt);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.print("[UDPparseConfigResponse] ccwLim: ");
		Serial.println(response.substring(strStrt, strStop));
	}
	//cwLim
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.print("[UDPparseConfigResponse] cwLim: ");
		Serial.println(response.substring(strStrt, strStop));
	}
	//openIsCCW
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
	strncpy(EEPROMdata.openIsCCW, (response.substring(strStrt, strStop) == "false") ? "CW" : "CCW", 4);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.print("[UDPparseConfigResponse] openIsCCW: ");
		Serial.println(EEPROMdata.openIsCCW ? "true" : "false");
	}
	//isMaster
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
	EEPROMdata.master = (response.substring(strStrt, strStop) == "false") ? false : true;
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.print("[UDPparseConfigResponse] isMaster: ");
		Serial.println(EEPROMdata.master ? "true" : "false");
	}
	//devName
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
	strncpy(EEPROMdata.namestr, response.substring(strStrt, strStop).c_str(), 41);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[UDPparseConfigResponse] devName: " + String(EEPROMdata.namestr));
	}
	//groupName
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
	strncpy(EEPROMdata.groupstr, response.substring(strStrt, strStop).c_str(), 41);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[UDPparseConfigResponse] groupName: " + String(EEPROMdata.groupstr));
	}
	//devType
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
	strncpy(EEPROMdata.typestr, response.substring(strStrt, strStop).c_str(), 21);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[UDPparseConfigResponse] devType: " + String(EEPROMdata.typestr));
	}
	//note
	strStrt = response.indexOf("=", strStop)+1;
	strStop = response.indexOf(";", strStrt);
	strncpy(EEPROMdata.note, response.substring(strStrt, strStop).c_str(), 56);
	if (DEBUG_MESSAGES && UDP_PARSE_MESSAGES)
	{
		Serial.println("[UDPparseConfigResponse] note: " + String(EEPROMdata.note));
	}

	uint addr = 0;
	EEPROM.begin(EEPROMsz);
	// replace values in EEPROM
	EEPROM.put(addr, EEPROMdata);
	EEPROM.commit();
	resetOnNextLoop = true;
	
}