/*
You should only have to edit the code in this file to add new custom device types. See deviceDefinitions.h
for the customization variables common to all types

This file is setup and tested to work with a NodeMCU 1.0 module (ESP-12E) - defined as ESP8266 in deviceDefinitions.h.
If you have a variant with a different PIN-arrangement you may need to edit the pin definitions listed below.

//TODO - UPDATE THIS WHEN DONE
To make this work in your home you need to:
	1) Connect to your device as an access point when it first boots and
	provide the WiFi SSID and password of you network
	(the device should come up as a WiFi network with the name you selected as a
	device on your phone's WiFi list - so goto your WiFi selection and switch to
	it to connect to the device the first time).

	2) Edit below to pick a unique name for this device.
	(This name will be used by Alexa or SmartThings).

	3) Compile and upload this file to your device.
	(if you change your WiFi password you will need to update the file
	and reload it for each device. The USB port should remain accessible
	in your fully assembled device for that purpose).

Notes:
- Use ESPfauxmo board version 2.3.0; newer versions don't seem to be discoverable via updated Alexa devices
*/

//Libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>  			//https://github.com/esp8266/Arduino
#include <ESP8266mDNS.h>

#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <ESPAsyncTCP.h>
#include <WebSocketsServer.h> 		//for ASYNC websockets - go into webSockets.h and uncomment #define WEBSOCKETS_NETWORK_TYPE NETWORK_ESP8266_ASYNC - comment out other #define WEBSOCKETS_NETWORK_TYPE lines
#include <fauxmoESP.h>
#include <AccelStepper.h>
#include <EEPROM.h>
#include <StreamString.h>

//custom definitions
#include "CommonDeviceDefinitions.h"  		// device settings common to all devices (names, debug paramters, etc)
#include "CommonGlobals.h"					// global variables
#include "CommonWebresources.h"				// strings for device served webpages
#include "2-State-ActuatorGlobals.h" 		// global variables specific to 2-state-actuators
#include "2-State-ActuatorWebresources.h" 	// webresources specific to 2-state-actuators

//the list of device types incoroprated into this build
#define DEVICETYPES "{2-State-Actuator}"

//Used for DEBUG_TIMING if enabled
int count;
unsigned long tmrs[]={0,0,0,0,0,0,0,0};

//this is the base setup routine called first on power up or reboot
void setup()
{
	serialStart(); //if debugging turn on serial, supports all "show" functions
	showPersonalityDataSize(); //if debugging - show the EEPROM size for all the data
	retrieveDataFromEEPROM(); //
	showEEPROMPersonalityData();
	initializePersonalityIfNew();
	showEEPROMPersonalityData();	
	WifiFauxmoAndDeviceSetup();
	deviceSetup();
	announceReadyOnUDP();	
	count=0;
}


//this is the main operating loop (MOL) that is repeatedly executed
void loop()
{
	//TODO - clean out all this timing stuff if desired
	static unsigned long lastMillisTiming; if(DEBUG_TIMING){count=count+1;lastMillisTiming = millis();}
	dns.processNextRequest(); if(DEBUG_TIMING) {tmrs[0]=tmrs[0] + (millis()-lastMillisTiming);lastMillisTiming = millis();}
	//reset device if flagged to
	checkResetOnLoop(); if(DEBUG_TIMING) {tmrs[1]=tmrs[1] + (millis()-lastMillisTiming); lastMillisTiming = millis();}
	//process any net (UDP) traffic
	UDPprocessPacket(); if(DEBUG_TIMING) {tmrs[3]=tmrs[3] + (millis()-lastMillisTiming); lastMillisTiming = millis();}
	//talk on net and drop dead notes from list
	UDPkeepAliveAndCull(); if(DEBUG_TIMING) {tmrs[4]=tmrs[4] + (millis()-lastMillisTiming); lastMillisTiming = millis();}
	
	// Since fauxmoESP 2.0 the library uses the "compatibility" mode by
	// default, this means that it uses WiFiUdp class instead of AsyncUDP.
	// The later requires the Arduino Core for ESP8266 staging version
	// whilst the former works fine with current stable 2.3.0 version.
	// But, since it's not "async" anymore we have to manually poll for UDP
	// packets
	fauxmo.handle(); if(DEBUG_TIMING) {tmrs[5]=tmrs[5] + (millis()-lastMillisTiming); lastMillisTiming = millis();}
	//run state machine for device
	operateDevice(); if(DEBUG_TIMING) {tmrs[6]=tmrs[6] + (millis()-lastMillisTiming); lastMillisTiming = millis();}
	
	showHeapAndProcessSerialInput(); //if debugging allow heap display and take serial commands
	
	if(DEBUG_TIMING) 
	{
		tmrs[7]=tmrs[7] + (millis()-lastMillisTiming); lastMillisTiming = millis();
		if (count%100000==0) {Serial.println();for(int iii=0;iii<8;iii++){Serial.printf("%d) delta: %lu\n",iii,tmrs[iii]);}}
	}
}

//determine if initalization string is different than stored in EEPROM - 
//if so load in new data from compiled code; if not just load SW version info
//but keep stored perosnality data (software update without personality change)
void initializePersonalityIfNew(){
	// change EEPROMdata in RAM
	if (String(INITIALIZE) != String(EEPROMdata.initstr))
	{
		if (DEBUG_MESSAGES)
		{
			Serial.println("[SETUP] Updating...");
		}

		//store specified personality data
		strncpy(EEPROMdata.swVer, SW_VER, 11);
		strncpy(EEPROMdata.initstr, INITIALIZE, 13);
		strncpy(EEPROMdata.namestr, CUSTOM_DEVICE_NAME, 41);
		strncpy(EEPROMdata.groupstr, CUSTOM_GROUP_NAME, 41);
		strncpy(EEPROMdata.typestr, CUSTOM_DEVICE_TYPE, 21);
		strncpy(EEPROMdata.note, CUSTOM_NOTE, 56);
		EEPROMdata.timeOut = DEVICE_TIMEOUT;
		EEPROMdata.deviceTimedOut = false;	//initialized to false on boot

		//Get the default device specific data - load it into EEPROMdata, and decode it into the device specific EEPROMdeviceData
		//this call also stores the encoded char[256] into EEPROMdata.deviceCustomData
		initializeDeviceCustomData();

		if (MASTER_NODE)
		{
			if (DEBUG_MESSAGES)	{Serial.println("[SETUP] Setting this node as MASTER.");}
			EEPROMdata.master = true;
		}
		else
		{
			EEPROMdata.master = false;
		}
		storeDataToEEPROM();
	}
	else
	{
		//always show the latest SW_VER
		strncpy(EEPROMdata.swVer, SW_VER, 11);
		if (DEBUG_MESSAGES)	
		{
			Serial.println("[SETUP] Actual swVER: " + String(EEPROMdata.swVer));
		}
		if (DEBUG_MESSAGES)
		{
			Serial.println("[SETUP] Nothing else to update.");
		}
		storeDataToEEPROM();
	}

}
//dump EEPROM personality data to Serial
void showEEPROMPersonalityData()
{	
	if (DEBUG_MESSAGES)
	{
		Serial.println("[SETUP] Init string: "+String(EEPROMdata.initstr)+", Name string: "+String(EEPROMdata.namestr)+", Master: " + String(EEPROMdata.master?"True":"False")+", Group name string: "+String(EEPROMdata.groupstr)+",Type string: "+String(EEPROMdata.typestr)+",Note string: "+String(EEPROMdata.note)+", SW Version string: "+String(EEPROMdata.swVer) + ", Motor Timeout "+String(EEPROMdata.timeOut)+ ", Device Timedout "+String(EEPROMdata.deviceTimedOut));
		Serial.println("[SETUP-device] Compiled init string: " + String(INITIALIZE) + ". Stored init string: " + String(EEPROMdata.initstr));
		
		showEEPROMdevicePersonalityData();		
	}
}
//dump fishyDevice data to Serial
void showThisNode(fishyDevice holder){
	if (DEBUG_MESSAGES)
	{
		Serial.println("[SETUP] IP: "+holder.ip.toString()+", Name string: "+String(holder.name)+", Master: " + String(holder.isMaster?"True":"False")+", Group name string: "+String(holder.groupstr)+",Type string: "+String(holder.typestr)+",Status string: "+String(holder.statusstr)+", Error state: "+String(holder.inError?"True":"False") + ", Dead state: "+String(holder.dead?"True":"False") + ", Time Stamp: "+String(holder.timeStamp));
	}
}

//sets up a fauxmo device if enabled to allow control via Alexa, etc
void WifiFauxmoAndDeviceSetup(){
	// You can enable or disable the library at any moment
	// Disabling it will prevent the devices from being discovered and switched
	fauxmo.enable(false);

	if (!DEBUG_WiFiOFF)
	{
		// WiFi - see wifi-and-webserver.ino
		WiFiSetup();
		if(FAUXMO_ENABLED){
			fauxmo.enable(true);
			// Add virtual device
			fauxmo.addDevice(EEPROMdata.namestr);
		}
	}
	
	// Device 
	if (DEBUG_MESSAGES)
	{
		Serial.printf("[SETUP] Device: %s; deviceState: %s.\n", EEPROMdata.namestr, deviceState ? "ON" : "OFF");
	}

	if(FAUXMO_ENABLED){
		// fauxmoESP 2.0.0 has changed the callback signature to add the device_id,
		// this way it's easier to match devices to action without having to compare strings.
		fauxmo.onSetState([](unsigned char device_id, const char *device_name, bool state) {
			if (DEBUG_MESSAGES)
			{
				Serial.printf("[SETUP] Device #%d (%s) state was set: %s\n", device_id, device_name, state ? "ON" : "OFF");
			}
			executeState(state);

		});

		// Callback to retrieve current state (for GetBinaryState queries)
		fauxmo.onGetState([](unsigned char device_id, const char *device_name) {
			return deviceState;
		});
	}

}
//as name implies - dump memory remaining on serial - process every second
void showHeapAndProcessSerialInput(){
	if (DEBUG_MESSAGES)
	{
		if (DEBUG_HEAP_MESSAGE)
		{
			static unsigned long last = millis();
			if (millis() - last > 1000)
			{
				last = millis();
				Serial.printf("[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
			}
		}

		//take serial commands in
		if (Serial.available() > 0)
		{
			char inputMsg[MAXCMDSZ];
			int sizeRead;
			sizeRead = Serial.readBytesUntil('\r', inputMsg, sizeof(inputMsg));
			if (sizeRead)
			{
				executeCommands(inputMsg, IPAddress(0, 0, 0, 0));
			}
			Serial.flush();
		}
		
	}
}
// Initialize serial port and clean garbage
void serialStart(){	
	if (DEBUG_MESSAGES||DEBUG_TIMING)
	{
		Serial.begin(SERIAL_BAUDRATE);
		Serial.println();
		Serial.println();
	}
}
//reset if commanded to by someone last loop
void checkResetOnLoop(){	
	if(resetOnNextLoop){
		//allow time for any commit to settle and webresponses to process before bailing
		delay(2000);
		resetController();
	}
}
//Setup Device Personaility and update EEPROM data as needed
void showPersonalityDataSize(){
	if (DEBUG_MESSAGES)
	{
		Serial.println("[SETUP] The personality data needs " + String(sizeof(EEPROMdata)) + " Bytes in EEPROM.");
		Serial.println("[SETUP] initstr " + String(sizeof(EEPROMdata.initstr)));
		Serial.println("[SETUP] namestr " + String(sizeof(EEPROMdata.namestr)));
		Serial.println("[SETUP] master " + String(sizeof(EEPROMdata.master)));
		Serial.println("[SETUP] typestr " + String(sizeof(EEPROMdata.typestr)));
		Serial.println("[SETUP] groupstr " + String(sizeof(EEPROMdata.groupstr)));
		Serial.println("[SETUP] note " + String(sizeof(EEPROMdata.note)));
		Serial.println("[SETUP] swVer " + String(sizeof(EEPROMdata.swVer)));
		Serial.println("[SETUP] timeOut " + String(sizeof(EEPROMdata.timeOut)));
		Serial.println("[SETUP] deviceTimedOut " + String(sizeof(EEPROMdata.deviceTimedOut)));
		Serial.println("[SETUP] deviceCustomData " + String(sizeof(EEPROMdata.deviceCustomData)));
	}
}

void fastBlinks(int numBlinks)
{

	pinMode(LED_BUILTIN, OUTPUT); // Initialize GPIO2 pin as an output

	for (int i = 0; i < numBlinks; i++)
	{
		digitalWrite(LED_BUILTIN, LOW);  // Turn the LED on by making the voltage LOW
		delay(100);						 // Wait for a second
		digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
		delay(200);						 // Wait for two seconds
	}
	pinMode(LED_BUILTIN, INPUT_PULLUP);
}

void slowBlinks(int numBlinks)
{

	pinMode(LED_BUILTIN, OUTPUT); // Initialize GPIO2 pin as an output

	for (int i = 0; i < numBlinks; i++)
	{
		digitalWrite(LED_BUILTIN, LOW);  // Turn the LED on by making the voltage LOW
		delay(1000);					 // Wait for a second
		digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
		delay(1000);					 // Wait for two seconds
	}
	pinMode(LED_BUILTIN, INPUT_PULLUP);
}


//makes the name string 255 in length and adds H3 tags (space after tags so ignored by browser)
String paddedH3Name(String name)
{
	String newName = "<H3>" + name + "</H3>";
	int growBy = 255 - name.length();
	for (int i = 0; i < growBy; i++)
	{
		newName += " ";
	}
	return newName;
}
//return motor position as a [%] of allowed range
/* String motPosStr(int intPadded, fishyDevice device)
{
	int relPos = device.motorPos;
	//int range = device.motorPosAtCW - device.motorPosAtCCW;
	int answer;
	if (device.range > 0)
	{
		answer = (int)(100 * relPos / device.range);
		return paddedInt(intPadded, answer);
	}
	else
		return paddedInt(intPadded, 0);
} */

String threeDigits(int i)
{
	String threeStr;
	if (i < 10)
	{
		threeStr = "00";
	}
	else if (i < 100)
	{
		threeStr = "0";
	}
	else
	{
		threeStr = "";
	}
	threeStr += String(i);
	return threeStr;
}

String paddedInt(int lengthInt, int val)
{
	String paddedStr = String(val);
	while (paddedStr.length() < lengthInt)
	{
		paddedStr += " ";
	}
	return paddedStr;
}

String paddedIntQuotes(int lengthInt, int val)
{
	String paddedStr = "'" + String(val) + "'";
	while (paddedStr.length() < lengthInt + 2)
	{
		paddedStr += " ";
	}
	return paddedStr;
}

//put the controller in a unknown state ready for calibration
void resetController()
{
	ESP.restart();
}