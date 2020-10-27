/*
    Name:       bootsdaten.ino
    Created:	22.10.2020 23:23:36
    Author:     astec-PG\gerryadmin
*/

#include <ArduinoOTA.h>
#include "BoardInfo.h"
#include "configuration.h"
#include "helper.h"
#include <esp.h>
#include <ESPmDNS.h>
#include <ESP_WiFi.h>
#include <ESPAsyncWebServer.h>
#include "LED.h"
#include <SparkFun_MMA8452Q.h>
#include <Wire.h>
#include "Analog.h"
#include <SPIFFS.h>

// Set web server port number to 80
AsyncWebServer server(80);

// Info Board for HTML-Output
String sBoardInfo;
BoardInfo boardInfo;

//Gyro
MMA8452Q mma;

//Variables for website
String replaceVariable(const String& var)
{
	if (var == "sKraengung")return String(fKraengung,1);
	if (var == "sGaugeKraengung")return String(fGaugeKraengung, 1);
	if (var == "sSStellung")return String(fSStellung,1);
	if (var == "sSTBB")return sSTBB;
	if (var == "sBoardInfo")return sBoardInfo;
	if (var == "AP_IP")return AP_IP.toString();
}

// The setup() function runs once each time the micro-controller starts
void setup()
{
	Serial.begin(115200);

	//Filesystem prepare for Webfiles
	if (!SPIFFS.begin(true)) {
		Serial.println("An Error has occurred while mounting SPIFFS");
		return;
	}
	Serial.println("Speicher SPIFFS benutzt:");
	Serial.println(SPIFFS.usedBytes());

	File root = SPIFFS.open("/");

	listDir("/");

	sBoardInfo = boardInfo.ShowChipIDtoString();

	LEDinit();
	pinMode(iMaxSonar, INPUT);

	Wire.begin();

	//MMA
	bool MMAbegin = mma.init();

	switch (MMAbegin) {
	case 0:
		Serial.println("\nMMA could not start!");
		break;
	case 1:
		Serial.println("\nMMA found!");
		mma.init(SCALE_2G);
		Serial.print("Range = "); Serial.print(2 << mma.available());
		Serial.println("G");
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
	

	if (!MDNS.begin("Bootsdaten")) {
		Serial.println("Error setting up MDNS responder!");
		while (1) {
			delay(1000);
		}
	}
	Serial.println("mDNS responder started");

	server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(SPIFFS, "/index.html", String(), false, replaceVariable);
	});
	server.on("/system.html", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(SPIFFS, "/system.html", String(), false, replaceVariable);
	});
	server.on("/ueber.html", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(SPIFFS, "/ueber.html", String(), false, replaceVariable);
	});
	server.on("/gauge.min.js", HTTP_GET, [](AsyncWebServerRequest* request) {
		request->send(SPIFFS, "/gauge.min.js");
	});
	server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send(SPIFFS, "/style.css", "text/css");
	});

	// Start TCP (HTTP) server
	server.begin();
	Serial.println("TCP server started");

	// Add service to MDNS-SD
	MDNS.addService("http", "tcp", 80);



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

	ArduinoOTA.setHostname("Bootsdaten");
	ArduinoOTA.begin();
}

// Add the main program code into the continuous loop() function
void loop()
{
	//Wifi variables
	bConnect_CL = WiFi.status() == WL_CONNECTED ? 1 : 0;
	bAP_on = WiFi.enableAP(bAP_on);

	// LED visu Wifi
	switch (bConnect_CL || bAP_on)
	{
	case 0:LEDoff(); break;
	case 1:LEDblinkslow(); break;
	default:LEDoff();
	}

	ArduinoOTA.handle();
	delay(1000);

	WiFiDiag();

// read MMA, use only x for Krängung
	Serial.printf("X: %f °\n", mma.getX() / 11.377);
	fKraengung = mma.getX() / 11.377;
	fKraengung = (abs(fKraengung));
	fGaugeKraengung = mma.getX() / 11.377;
	Serial.printf("Y: %f °\n", mma.getY() / 11.377);
	Serial.printf("Z: %f °\n", mma.getZ() / 11.377);

	// Direction Krängung
	if (mma.getX() < -1)
		sSTBB = "Backbord";
	else sSTBB = "Steuerbord";

	// Orientation Sensor 
	uint8_t Orientation = mma.readPL();
	switch (Orientation) {
	case PORTRAIT_U: sOrient = "Oben";break;
	case PORTRAIT_D: sOrient = "Unten";break;
	case LANDSCAPE_R: sOrient = "Rechts";break;
	case LANDSCAPE_L: sOrient = "Links";break;
	case LOCKOUT: sOrient = "Horizontal";break;
	}
	Serial.printf("Orientation: %s", sOrient);

	//AI Distance-Sensor read
	iDistance = analogRead(iMaxSonar);
	int Err = 0;
	fSStellung = analogInScale(iDistance, 4000, 220, 400.0, 30.0, Err);
	Serial.printf("Schwert: %f cm", fSStellung);

	freeHeapSpace();
}


