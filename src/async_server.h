#include <Arduino.h>
#include "web.h"
#include <Littlefs.h>
#include <Update.h>

// Credits : this is a mashup of code from the following repositories, plus OTA firmware update feature
// https://github.com/smford/esp32-asyncwebserver-fileupload-example
// https://randomnerdtutorials.com/esp32-web-server-spiffs-spi-flash-file-system/


typedef struct LOGON_CONFIG_ {
  String httpuser;           // username to access web admin
  String httppassword;       // password to access web admin
} LOGON_CONFIG;

// AsyncWebServer *server = NULL;  

const String default_httpuser = "admin";
const String default_httppassword = "admin";

static LOGON_CONFIG config;
static File LittleFSFile;

static String server_directory(bool ishtml = false);
static void server_not_found(AsyncWebServerRequest *request);
static bool server_authenticate(AsyncWebServerRequest * request);
static void server_handle_upload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
static void server_handle_littleFS_upload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
static String server_string_processor(const String& var);
static void firmware_configure();
static String server_ui_size(const size_t bytes);
static void server_handle_OTA_update(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
static int littlefs_chunked_read(uint8_t* buffer, int maxLen);



// list all of the files, if ishtml=true, return html rather than simple text
static String server_directory(bool ishtml) {
  String returnText = "";
  Serial.println("Listing files stored on LittleFS");
  File root = LittleFS.open("/");
  File foundfile = root.openNextFile();
  if (ishtml) {
    returnText += "<table align='center'><tr><th align='left'>Name</th><th align='left'>Size</th><th></th><th></th></tr>";
  }
  while (foundfile) {
    if (ishtml) {
      returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td>" + server_ui_size(foundfile.size()) + "</td>";
      returnText += "<td><button class='directory_buttons' onclick=\"directory_button_handler(\'" + String(foundfile.name()) + "\', \'download\')\">Download</button>";
      returnText += "<td><button class='directory_buttons' onclick=\"directory_button_handler(\'" + String(foundfile.name()) + "\', \'delete\')\">Delete</button></tr>";
    } else {
      returnText += "File: " + String(foundfile.name()) + " Size: " + server_ui_size(foundfile.size()) + "\n";
    }
    foundfile = root.openNextFile();
  }
  if (ishtml) {
    returnText += "</table>";
  }
  root.close();
  foundfile.close();
  return returnText;
}

// Make size of files human readable
// source: https://github.com/CelliesProjects/minimalUploadAuthESP32

static String server_ui_size(const size_t bytes) {
  if (bytes < 1024) return String(bytes) + " B";
  else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
  else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
  else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
  }

 
// replace %SOMETHING%  in webpage with dynamically generated string
/*static String server_string_processor(const String& var) {
    if (var == "BUILD_TIMESTAMP") {
        return String(__DATE__) + " " + String(__TIME__); 
        }
    else
    if (var == "FREELittleFS") {
        return server_ui_size((LittleFS.totalBytes() - LittleFS.usedBytes()));
        }
    else
    if (var == "USEDLittleFS") {
        return server_ui_size(LittleFS.usedBytes());
        }
    else
    if (var == "TOTALLittleFS") {
        return server_ui_size(LittleFS.totalBytes());
        }
    else
        return "?";
    }
*/

static int littleFS_chunked_read(uint8_t* buffer, int maxLen) {              
  //Serial.printf("MaxLen = %d\n", maxLen);
  if (!LittleFSFile.available()) {
    LittleFSFile.close();
    return 0;
    }
  else {
    int count = 0;
    while (LittleFSFile.available() && (count < maxLen)) {
      buffer[count] = LittleFSFile.read();
      count++;
      }
    return count;
    }
}

/*
void firmware_configure() {
  // if url isn't found
  server.onNotFound(server_not_found);

  // run handleUpload function when any file is uploaded
  server.onFileUpload(server_handle_upload);

  // visiting this page will cause you to be logged out
  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->requestAuthentication();
    request->send(LittleFS, "/index.html", String(), false, server_string_processor);
  });

  // presents a "you are now logged out webpage
  server.on("/logged-out", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    Serial.println(logmessage);
    request->send(LittleFS, "/index.html", String(), false, server_string_processor);
  });

  server.on("/firmware", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + + " " + request->url();
    if (server_authenticate(request)) {
      logmessage += " Auth: Success";
      Serial.println(logmessage);
      request->send(LittleFS, "/firmware.html", String(), false, server_string_processor);
    } else {
      logmessage += " Auth: Failed";
      Serial.println(logmessage);
      return request->requestAuthentication();
    }
  });



  server.on("/directory", HTTP_GET, [](AsyncWebServerRequest * request)  {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    if (server_authenticate(request)) {
      logmessage += " Auth: Success";
      Serial.println(logmessage);
      request->send(200, "text/plain", server_directory(true));
    } else {
      logmessage += " Auth: Failed";
      Serial.println(logmessage);
      return request->requestAuthentication();
    }
  });


  server.on("/file", HTTP_GET, [](AsyncWebServerRequest * request) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    if (server_authenticate(request)) {
      logmessage += " Auth: Success";
      Serial.println(logmessage);

      if (request->hasParam("name") && request->hasParam("action")) {
        const char *fileName = request->getParam("name")->value().c_str();
        const char *fileAction = request->getParam("action")->value().c_str();

        logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url() + "?name=" + String(fileName) + "&action=" + String(fileAction);

        if (!LittleFS.exists(fileName)) {
          Serial.println(logmessage + " ERROR: file does not exist");
          request->send(400, "text/plain", "ERROR: file does not exist");
          } 
        else {
          Serial.println(logmessage + " file exists");
          if (strcmp(fileAction, "download") == 0) {
            logmessage += " downloaded";
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
            logmessage += " deleted";
            LittleFS.remove(fileName);
            request->send(200, "text/plain", "Deleted File: " + String(fileName));
            } 
          else {
            logmessage += " ERROR: invalid action param supplied";
            request->send(400, "text/plain", "ERROR: invalid action param supplied");
            }
          Serial.println(logmessage);
          }
      } 
    else {
      request->send(400, "text/plain", "ERROR: name and action params required");
      }
    } 
  else {
    logmessage += " Auth: Failed";
    Serial.println(logmessage);
    return request->requestAuthentication();
    }
  });
}
*/
#if 0
void read_file_chunk(uint8_t* buffer, int maxlen) {
      int bytesRemaining = (int)(FlashLogFreeAddress - flashAddr);
      do {
         int numXmitBytes =  bytesRemaining > 256 ? 256 : bytesRemaining;  
		   spiflash_readBuffer(flashAddr, buffer, numXmitBytes);
         pServer->sendContent_P((const char*)buffer, numXmitBytes);
         flashAddr += numXmitBytes;
         bytesRemaining = (int)(FlashLogFreeAddress - flashAddr);
         delayMs(10);
		   } while (bytesRemaining >= 0);
	   }
   else {
      server_reportFileNotFound("datalog"); 
      }
   }
#endif

static void server_not_found(AsyncWebServerRequest *request) {
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
  Serial.println(logmessage);
  request->send(404, "text/plain", "Not found");
  }
  
// used by server.on functions to discern whether a user has the correct httpapitoken OR is authenticated by username and password
bool server_authenticate(AsyncWebServerRequest * request) {
  bool isAuthenticated = false;

  if (request->authenticate(config.httpuser.c_str(), config.httppassword.c_str())) {
    Serial.println("is authenticated via username and password");
    isAuthenticated = true;
  }
  return isAuthenticated;
}


static void server_handle_upload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (filename.endsWith(".bin") ) {
      server_handle_OTA_update(request, filename, index, data, len, final);
      }
    else {
      server_handle_littleFS_upload(request, filename, index, data, len, final);
    }
}


// handles non .bin file uploads to the LittleFS directory
static void server_handle_littleFS_upload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  // make sure authenticated before allowing upload
  //if (server_authenticate(request)) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    Serial.println(logmessage);

    if (!index) {
      logmessage = "Upload Start: " + String(filename);
      // open the file on first call and store the file handle in the request object
      request->_tempFile = LittleFS.open("/" + filename, "w");
      Serial.println(logmessage);
    }

    if (len) {
      // stream the incoming chunk to the opened file
      request->_tempFile.write(data, len);
      logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
      Serial.println(logmessage);
    }

    if (final) {
      logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
      // close the file handle as the upload is now done
      request->_tempFile.close();
      Serial.println(logmessage);
      request->redirect("/");
    }
  //} else {
    //Serial.println("Auth: Failed");
    //return request->requestAuthentication();
  }



// handles OTA firmware update
static void server_handle_OTA_update(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  // make sure authenticated before allowing upload
  if (server_authenticate(request)) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    Serial.println(logmessage);

    if (!index) {
      logmessage = "OTA Update Start: " + String(filename);
      Serial.println(logmessage);
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
        }
    }

    if (len) {
     // flashing firmware to ESP
     if (Update.write(data, len) != len) {
        Update.printError(Serial);
        }      
      logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
      Serial.println(logmessage);
    }

    if (final) {
     if (Update.end(true)) { //true to set the size to the current progress
         logmessage = "OTA Complete: " + String(filename) + ",size: " + String(index + len);
         Serial.println(logmessage);
          } 
     else {
          Update.printError(Serial);
          }
      request->redirect("/");
      }
  } else {
    Serial.println("Auth: Failed");
    return request->requestAuthentication();
  }
}

inline int littlefs_chunked_read(uint8_t *buffer, int maxLen)
{
    return 0;
}
