#ifndef _WEB_H_
#define _WEB_H_

#include <arduino.h>
#include "BoardInfo.h"
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "helper.h"
#include "async_server.h"

// Set web server port number to 80
AsyncWebServer server(80);
AsyncEventSource events("/events");

// Info Board for HTML-Output
String sBoardInfo;
BoardInfo boardInfo;

//Variables for website
String sCL_Status = sWifiStatus(WiFi.status());
bool IsRebootRequired = false;

String processor(const String& var)
{
	if (var == "CONFIGPLACEHOLDER")
	{
		String buttons = "";
		buttons += "<form onSubmit = \"event.preventDefault(); formToJson(this);\">";
		buttons += "<p class=\"CInput\"><label>SSID </label><input type = \"text\" name = \"SSID\" value=\"";
		buttons += tWeb_Config.wAP_SSID;
		buttons += "\"/></p>";
		buttons += "<p class=\"CInput\"><label>IP </label><input type = \"text\" name = \"IP\" value=\"";
		buttons += tWeb_Config.wAP_IP;
		buttons += "\"/></p>";
		buttons += "<p class=\"CInput\"><label>Password </label><input type = \"text\" name = \"Password\" value=\"";
		buttons += tWeb_Config.wAP_Password;
		buttons += "\"/></p>";
		buttons += "<p class=\"CInput\"><label>Kieloffset </label><input type = \"text\" name = \"Kiel_Offset\" value=\"";
		buttons += tWeb_Config.wKiel_Offset;
		buttons += "\"/> cm</p>";	
		buttons += "<p><input type=\"submit\" value=\"Speichern\"></p>";
		buttons += "</form>";
		return buttons;
	}
	return String();
}

String replaceVariable(const String& var)
{
	if (var == "sKraengung")return String(fKraengung,1);
	if (var == "sGaugeKraengung")return String(fGaugeKraengung, 1);
	if (var == "sCompassHeading")return String(fheading, 1);
	if (var == "sRollen")return String(fRollen, 1);
	if (var == "sGieren")return String(fGieren, 1);
	if (var == "sSStellung")return String(fSStellung,1);
	if (var == "sAbsTief")return String(fAbsTief,1);
	if (var == "sSTBB")return sSTBB;
	if (var == "sBoardInfo")return sBoardInfo;
	if (var == "sFS_USpace")return String(LittleFS.usedBytes());
	if (var == "sFS_TSpace")return String(LittleFS.totalBytes());
  	if (var == "sAP_Clients")return String(WiFi.softAPgetStationNum());
	if (var == "sAP_IP")return WiFi.softAPIP().toString();
  	if (var == "sCL_Addr")return WiFi.localIP().toString();
  	if (var == "sCL_Status")return String(sCL_Status);
  	if (var == "sI2C_Status")return String(sI2C_Status);
	if (var == "sI2C_Adress")return I2C_address;
	if (var == "sOrient")return sOrient;
	if (var == "sVersion")return Version;
	if (var == "BUILD_TIMESTAMP")return String(__DATE__) + " " + String(__TIME__);
	if (var == "CONFIGPLACEHOLDER")return processor(var);
  	return "NoVariable";
}

void website(){
	server.onNotFound(server_not_found);
	server.onFileUpload(server_handle_upload);
	server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(LittleFS, "/favicon.ico", "image/x-icon");
		});
	server.on("/logo80.jpg", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(LittleFS, "/logo80.jpg", "image/jpg");
		});
	server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
			request->send(LittleFS, "/index.html", String(), false, replaceVariable);
		});
	server.on("/system.html", HTTP_GET, [](AsyncWebServerRequest* request) {
			request->send(LittleFS, "/system.html", String(), false, replaceVariable);
		});
	server.on("/firmware.html", HTTP_GET, [](AsyncWebServerRequest* request) {
			request->send(LittleFS, "/firmware.html", String(), false, replaceVariable);
		});	
	server.on("/directory", HTTP_GET, [](AsyncWebServerRequest* request) {
			request->send(200, "text/plain", server_directory(true));
		});	
	server.on("/file", HTTP_GET, [](AsyncWebServerRequest* request) {
		if (request->hasParam("name") && request->hasParam("action")) {
			const char *fileName = request->getParam("name")->value().c_str();
			const char *fileAction = request->getParam("action")->value().c_str();
	
			if (!LittleFS.exists(fileName)) {
			  request->send(400, "text/plain", "ERROR: file does not exist");
			  } 
			else {
			  if (strcmp(fileAction, "download") == 0) {
				LittleFSFile = LittleFS.open(fileName, "r");
				int sizeBytes = LittleFSFile.size();
				Serial.println("large file, chunked download required");
				AsyncWebServerResponse *response = request->beginResponse("application/octet-stream", sizeBytes, [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
				  return littleFS_chunked_read(buffer, maxLen);
				  });
				char szBuf[80];
				sprintf(szBuf, "attachment; filename=%s", &fileName[1]);// get past the leading '/'
				response->addHeader("Content-Disposition", szBuf);
				response->addHeader("Connection", "close");
				request->send(response);
				} 
			  else 
			  if (strcmp(fileAction, "delete") == 0) {
				LittleFS.remove(fileName);
				request->send(200, "text/plain", "Deleted File: " + String(fileName));
				} 
			  else {
				request->send(400, "text/plain", "ERROR: invalid action param supplied");
				}
			  }
		  } 
		else {
		  request->send(400, "text/plain", "ERROR: name and action params required");
		  }
		}); 
	server.on("/settings.html", HTTP_GET, [](AsyncWebServerRequest* request) {
			request->send(LittleFS, "/settings.html", String(), false, replaceVariable);
		});
	server.on("/ueber.html", HTTP_GET, [](AsyncWebServerRequest* request) {
			request->send(LittleFS, "/ueber.html", String(), false, replaceVariable);
		});
	server.on("/compass.html", HTTP_GET, [](AsyncWebServerRequest* request) {
			request->send(LittleFS, "/compass.html", String(), false, replaceVariable);
		});
	server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest * request) {
			request->send(LittleFS, "/reboot.html", String(), false, processor);
			IsRebootRequired = true;
		});
	server.on("/gauge.min.js", HTTP_GET, [](AsyncWebServerRequest* request) {
			request->send(LittleFS, "/gauge.min.js");
		});
	server.on("/logo80.jpg", HTTP_GET, [](AsyncWebServerRequest *request){
    		request->send(LittleFS, "/logo80.jpg", "image/jpg");
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
}

#endif