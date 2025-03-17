#include <Arduino.h>
#include "web.h"
#include <Littlefs.h>
#include <Update.h>

// Credits : this is a mashup of code from the following repositories, plus OTA firmware update feature
// https://github.com/smford/esp32-asyncwebserver-fileupload-example
// https://randomnerdtutorials.com/esp32-web-server-spiffs-spi-flash-file-system/


static File LittleFSFile;

static String server_directory(bool ishtml = false);
static void server_not_found(AsyncWebServerRequest *request);
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
  }


inline int littlefs_chunked_read(uint8_t *buffer, int maxLen)
{
    return 0;
}
