/*
    Name:       bootsdaten.ino
    Created:	22.10.2020
	Update: 	26.09.2023
    Author:     astec-PG\gerryadmin
*/

#include <Arduino.h>
#include <ArduinoOTA.h>
#include "BoardInfo.h"
#include "configuration.h"
#include "helper.h"
#include "web.h"
#include <esp.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <ESP_WiFi.h>
#include <ESPAsyncWebServer.h>
#include "LED.h"
#include <SparkFun_MMA8452Q.h>
#include <Wire.h>
#include "Analog.h"
#include <LittleFS.h>
#include "NMEA0183Telegram.h"
#include <NMEA2000_CAN.h>
#include <NMEA2000.h>
#include <N2kMessages.h>

// NMEA2000
Preferences preferences;             // Nonvolatile storage on ESP32 - To store LastDeviceAddress

//Gyro
MMA8452Q mma;

//NMEA 0183 Stream
WiFiUDP udp;  // Create UDP instance

void SendNMEA0183Message(String var) {
  udp.beginPacket(udpAddress, udpPort);   // Send to UDP
  udp.println(var);
  udp.endPacket();
}

// Set the information for other bus devices, which messages we support
const unsigned long TransmitMessages[] PROGMEM = {127257L, // Yaw,Pitch,Roll                                              
                                                  0
                                                 };


	bool IsTimeToUpdate(unsigned long NextUpdate) {
  		return (NextUpdate < millis());
	}
	unsigned long InitNextUpdate(unsigned long Period, unsigned long Offset = 0) {
  		return millis() + Period + Offset;
	}
	void SetNextUpdate(unsigned long &NextUpdate, unsigned long Period) {
  		while ( NextUpdate < millis() ) NextUpdate += Period;
	}

	// Create NMEA2000 message
	void SendN2kPitch(double Yaw, double Pitch, double Roll) {   //sende Yaw, Pitch, Roll
	static unsigned long SlowDataUpdated = InitNextUpdate(SlowDataUpdatePeriod);				
	tN2kMsg N2kMsg;

  	if ( IsTimeToUpdate(SlowDataUpdated) ) {
    	SetNextUpdate(SlowDataUpdated, SlowDataUpdatePeriod);

    	Serial.printf("Krängung: %3.1f °\n",Pitch);

  		SetN2kPGN127257(N2kMsg, 0, Yaw, Pitch, Roll); 
  		NMEA2000.SendMsg(N2kMsg);
		}
}

// The setup() function runs once each time the micro-controller starts
void setup()
{

	Serial.begin(115200);

	//Filesystem prepare for Webfiles
	if (!LittleFS.begin(true)) {
		Serial.println("An Error has occurred while mounting LittleFS");
		return;
	}
	Serial.println("Speicher LittleFS benutzt:");
	Serial.println(LittleFS.usedBytes());

	File root = LittleFS.open("/");
  	listDir(LittleFS, "/", 3);
	// file exists, reading and loading config file
	readConfig("/config.json");
	AP_SSID = tAP_Config.wAP_SSID;
	AP_PASSWORD = tAP_Config.wAP_Password;
	fKielOffset = atof(tAP_Config.wKiel_Offset);

	pinMode(iMaxSonar, INPUT);

	// I2C
  	Wire.begin(I2C_SDA, I2C_SCL);
  	I2C_scan();

	//LED
  	LEDInit();
  	LEDoff();
	
	// Boardinfo	
  	sBoardInfo = boardInfo.ShowChipIDtoString();

	//MMA
	bool MMAbegin = mma.init();

	switch (MMAbegin) {
	case 0:
		Serial.println("\nMMA could not start!");
		sI2C_Status = "Keinen Sensor gefunden!";
		bI2C_Status = 0;
		break;
	case 1:
		Serial.println("\nMMA found!");
		mma.init(SCALE_2G);
		Serial.print("Range = "); Serial.print(2 << mma.available());
		Serial.println("G");   
		sI2C_Status = "Sensor gefunden!"; 
		bI2C_Status = 1;          
	}
	
	//WIFI
	if (!WiFi.setHostname(HostName))
		Serial.println("\nSet Hostname success");
	else
		Serial.println("\nSet Hostname not success");

	//WiFiServer AP starten
	WiFi.mode(WIFI_AP_STA);
	WiFi.softAP(AP_SSID, AP_PASSWORD);
	delay(1000);
	if (WiFi.softAPConfig(IP, Gateway, NMask))
		Serial.println("\nIP config success");	
	else
		Serial.println("IP config not success");	

	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP configured with address: ");
	Serial.println(myIP);
	

	if (!MDNS.begin(HostName)) {
		Serial.println("Error setting up MDNS responder!");
		while (1) {
			delay(1000);
		}
	}
	Serial.println("mDNS responder started");

	server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/favicon.ico", "image/x-icon");
	});
	server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(LittleFS, "/index.html", String(), false, replaceVariable);
	});
	server.on("/system.html", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(LittleFS, "/system.html", String(), false, replaceVariable);
	});
	server.on("/settings.html", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(LittleFS, "/settings.html", String(), false, replaceVariable);
	});
	server.on("/ueber.html", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(LittleFS, "/ueber.html", String(), false, replaceVariable);
	});
	server.on("/gauge.min.js", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(LittleFS, "/gauge.min.js");
	});
	server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(LittleFS, "/style.css", "text/css");
	});
	server.on("/settings.html", HTTP_POST, [](AsyncWebServerRequest *request)
	{
		int count = request->params();
		Serial.printf("Anzahl: %i\n", count);
		for (int i = 0;i < count;i++)
		{
			AsyncWebParameter* p = request->getParam(i);
			Serial.print("PWerte von der Internet - Seite: ");
			Serial.print("Param name: ");
			Serial.println(p->name());
			Serial.print("Param value: ");
			Serial.println(p->value());
			Serial.println("------");
			// p->value in die config schreiben
			writeConfig(p->value());
		}
		request->send(200, "text/plain", "Daten gespeichert");
	});

	// Start TCP (HTTP) server
	server.begin();
	Serial.println("TCP server started");

	// Add service to MDNS-SD
	MDNS.addService("http", "tcp", 80);

	// NMEA2000
  	NMEA2000.SetN2kCANMsgBufSize(8);
  	NMEA2000.SetN2kCANReceiveFrameBufSize(250);
  	NMEA2000.SetN2kCANSendFrameBufSize(250);

  	esp_efuse_mac_get_default(chipid);
  	for (i = 0; i < 6; i++) id += (chipid[i] << (7 * i));

  	// Set product information
  	NMEA2000.SetProductInformation("BD01", // Manufacturer's Model serial code
                                 100, // Manufacturer's product code
                                 "BD Sensor Module",  // Manufacturer's Model ID
                                 "1.0.3.00 (2023-09-30)",  // Manufacturer's Software version code
                                 "1.0.3.0 (2023-09-30)" // Manufacturer's Model version
                                );
  	// Set device information
  	NMEA2000.SetDeviceInformation(01, // Unique number. Use e.g. Serial number.
                                180, // 180 Device function=Attitude (Pitch, Roll, Yaw) Control. See codes on http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                40, // 40 Device class=Steering and Control Surfaces. See codes on  http://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                2046 // Just choosen free from code list on http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                               );

	NMEA2000.SetForwardType(tNMEA2000::fwdt_Text); // Show in clear text. Leave uncommented for default Actisense format.

  	preferences.begin("nvs", false);                          // Open nonvolatile storage (nvs)
  	NodeAddress = preferences.getInt("LastNodeAddress", 34);  // Read stored last NodeAddress, default 33
  	preferences.end();
  	Serial.printf("NodeAddress=%d\n", NodeAddress);

  	NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, NodeAddress);
  	NMEA2000.ExtendTransmitMessages(TransmitMessages);
  	NMEA2000.Open();

	ArduinoOTA
		.onStart([]() {
		String type;
		if (ArduinoOTA.getCommand() == U_FLASH)
			type = "sketch";
		else
			type = "filesystem";
		Serial.println("start updating " + type);
	})
		.onEnd([]() {
		Serial.println("\nEnd");
	})
		.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	})
		.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
		else if (error == OTA_END_ERROR) Serial.println("End Failed");
	});

	ArduinoOTA.setHostname(HostName);
	ArduinoOTA.begin();

}

// Add the main program code into the continuous loop() function
void loop()
{
	//Wifi variables
	bConnect_CL = WiFi.status() == WL_CONNECTED ? 1 : 0;
	//bAP_on = WiFi.enableAP(bAP_on);
	
	// LED visu Wifi
	LEDoff();
	LEDflash(LED(LEDBoard)); // Betrieb ok
	if (bI2C_Status == 0) LEDblink(LED(Red)); // Sensorfehler

// OTA	
	ArduinoOTA.handle();

// Status AP 
  Serial.printf("Stationen mit AP verbunden = %d\n", WiFi.softAPgetStationNum());
  Serial.printf("Soft-AP IP address = %s\n", WiFi.softAPIP().toString());
  sCL_Status = sWifiStatus(WiFi.status());

// read MMA, use x for Kraengung (Roll), y for Gieren (Pitch)
	fKraengung = mma.getX() / 11.377;
	fKraengung = (abs(fKraengung));
	fGaugeKraengung = mma.getX() / 11.377;
	fGieren = mma.getY() / 11.377;
	fRollen = mma.getZ() / 11.377;
	Serial.printf("X, Krängung: %f °\n", fKraengung);
	Serial.printf("Y, Gieren: %f °\n", fGieren);
	Serial.printf("Z, Rollen: %f °\n", fRollen);

	// Direction Kraengung
	if (mma.getX() < -1)
		sSTBB = "Backbord";
	else sSTBB = "Steuerbord";
	Serial.printf("Kraengung nach: %s %f °\n", sSTBB, fKraengung);

	// read I2C Orientation Sensor
	bool bSFM = 0; // SensorFalschMontiert
	uint8_t Orientation = mma.readPL();
	switch (Orientation) {
	case PORTRAIT_U: sOrient = "Oben";break;
	case PORTRAIT_D: sOrient = "Unten"; 
		bSFM = 1;
		Serial.print("Sensor falsch montiert\n");
		break;
	case LANDSCAPE_R: sOrient = "Rechts";break;		
	case LANDSCAPE_L: sOrient = "Links";break;		
	case LOCKOUT: sOrient = "Horizontal";
		bSFM = 1;
		Serial.print("Sensor falsch montiert\n");
		break;
	}
	Serial.printf("Orientation: %s\n", sOrient);

	// LED Kraengung
	if (bSFM == 0 && bI2C_Status == 1)
	{
		if (sSTBB == "Backbord")
		{
			digitalWrite(LED(Red), HIGH);
		}
		else 
		{
			digitalWrite(LED(Green), HIGH);
		}
	}
	else if (bI2C_Status == 1)
	{
		digitalWrite(LED(Blue), HIGH);
	}	

//AI Distance-Sensor
	iDistance = analogRead(iMaxSonar);
	Serial.printf("Analogeingang: %i\n", iDistance);
	int Err = 0;
	fSStellung = analogInScale(iDistance, 3800, 300, 7.0, 80.0, Err);
	Serial.printf("Schwert: %f cm\n", fSStellung);
	fAbsTief = fSStellung + fKielOffset;
	Serial.printf("Tiefgang: %f cm\n", fAbsTief);

// config von LittleFS lesen, Kieloffset schreiben
	readConfig("/config.json");
	fKielOffset = atof(tAP_Config.wKiel_Offset);

// Send Messages NMEA
	SendNMEA0183Message(sendXDR()); // Send NMEA0183 Message

delay(200);

// Daten für N2k
Serial.print("Daten für N2k:\n");
Serial.printf("Krängung: %3.0f °, Gieren: %3.0f °, Rollen: %3.0f °\n", fKraengung, fGieren, fRollen);
Serial.print("SendN2kPitch............\n");
 
SendN2kPitch(DegToRad(fKraengung), DegToRad(fRollen), DegToRad(fGieren)); // Send NMEA2000 Message

delay(200);
Serial.print("NMEA2000.ParseMessages............\n");

    NMEA2000.ParseMessages();       // Parse NMEA2000 Messages
    int SourceAddress = NMEA2000.GetN2kSource();
  	if (SourceAddress != NodeAddress) { // Save potentially changed Source Address to NVS memory
    	NodeAddress = SourceAddress;      // Set new Node Address (to save only once)
    	preferences.begin("nvs", false);
    	preferences.putInt("LastNodeAddress", SourceAddress);
    	preferences.end();
    	Serial.printf("Address Change: New Address=%d\n", SourceAddress);
  }

  // Dummy to empty input buffer to avoid board to stuck with e.g. NMEA Reader
  	if ( Serial.available() ) {
    Serial.read();
  }
	
	freeHeapSpace();
}
