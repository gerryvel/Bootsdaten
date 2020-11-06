/*
Helper is a collection of small "Helferlein"
# Time, read local time
# Read free Heapspace
# Wifi diagnosis
# Read files from SPIFFS 

*/

#ifndef HELPER_H
#define HELPER_H

#include <stdio.h>
#include <time.h>
#include <Arduino.h>
#include <SPIFFS.h>
#include <FS.h>
#include "configuration.h"
#include <ArduinoJson.h>

void ShowTime()
{
	time_t now = time(NULL);
	struct tm tm_now;
	localtime_r(&now, &tm_now);
	char buff[100];
	strftime(buff, sizeof(buff), "%d-%m-%Y %H:%M:%S", &tm_now);
	printf("Zeit: %s\n", buff);
}

void freeHeapSpace()
{
	static unsigned long last = millis();
	if (millis() - last > 5000) {
		last = millis();
		Serial.printf("\n[MAIN] Free heap: %d bytes\n", ESP.getFreeHeap());
		iFreeHeap = ESP.getFreeHeap();
	}
}

void WiFiDiag(void) {

	Serial.println("\nWifi-Diag:");
	AP_IP = WiFi.softAPIP();
	CL_IP = WiFi.localIP();
	Serial.print("AP IP address: ");
	Serial.println(AP_IP.toString());
	Serial.print("Client IP address: ");
	Serial.println(CL_IP.toString());
	WiFi.printDiag(Serial);
	Serial.print("\nScan AP's ");
	int n = WiFi.scanNetworks();
	Serial.printf("%i network(s) found\n", n);
	for (int i = 0; i < n; i++)
	{
		Serial.println(WiFi.SSID(i));
	}
	Serial.println();
}


void listDir(char * dir) {

	File root = SPIFFS.open(dir);
	File file = root.openNextFile();

	while (file) {

		Serial.print("FILE: ");
		Serial.println(file.name());

		file = root.openNextFile();
	}
}


void readConfig(String filename) {
	StaticJsonDocument<200> testDocument;
	File configFile = SPIFFS.open(filename);
	if (configFile)
	{
		Serial.println("opened config file");
		DeserializationError error = deserializeJson(testDocument, configFile);

		// Test if parsing succeeds.
		if (error)
		{
			Serial.print(F("deserializeJson() failed: "));
			Serial.println(error.f_str());
			return;
		}

		Serial.println("deserializeJson ok");
		{
			Serial.println("Lese Daten aus Config - Datei");
			strcpy(AP_Config.AP_SSID, testDocument["SSID"] | "Bootsdaten");
			strcpy(AP_Config.AP_IP, testDocument["IP"] | "192.168.16.1");
			strcpy(AP_Config.AP_Password, testDocument["Password"] | "12345678");
			strcpy(AP_Config.Kiel_Offset, testDocument["Kiel_Offset"] | "70.0");
			Serial.println(AP_Config.AP_SSID);
		}
		configFile.close();
		Serial.println("Config - Datei geschlossen");
	}

	else
	{
		Serial.println("failed to load json config");
	}
}


bool writeConfig(String json)
{
	Serial.println(json);

	Serial.println("neue Konfiguration speichern");

	File configFile = SPIFFS.open("/config.json", FILE_WRITE);
	if (configFile)
	{
		Serial.println("Config - Datei öffnen");
		File configFile = SPIFFS.open("/config.json", FILE_WRITE);
		if (configFile)
		{
			Serial.println("Config - Datei zum Schreiben geöffnet");
			StaticJsonDocument<200> testDocument;
			Serial.println("JSON - Daten übergeben");
			DeserializationError error = deserializeJson(testDocument, json);
			// Test if parsing succeeds.
			if (error)
			{
				Serial.print(F("deserializeJson() failed: "));
				Serial.println(error.f_str());
				// bei Memory - Fehler den <Wert> in StaticJsonDocument<200> testDocument; erhöhen
				return false;
			}
			Serial.println("Konfiguration schreiben...");
			serializeJson(testDocument, configFile);
			Serial.println("Konfiguration geschrieben...");

			// neue Config in Serial ausgeben zur Kontrolle
			serializeJsonPretty(testDocument, Serial);

			Serial.println("Config - Datei geschlossen");
			configFile.close();
		}
	}
	return true;
}


#endif