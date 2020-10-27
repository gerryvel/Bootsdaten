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

#endif