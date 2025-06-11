# 1 "C:\\Users\\GERRYA~1\\AppData\\Local\\Temp\\tmpjk04mq_o"
#include <Arduino.h>
# 1 "C:/Users/gerryadmin/Documents/Bootsdaten/src/bootsdaten.ino"







#include <Arduino.h>
#include <ArduinoOTA.h>
#include "BoardInfo.h"
#include "configuration.h"
#include "helper.h"
#include "web.h"
#include "Analog.h"
#include "LED.h"
#include "imucal.h"
#include <esp.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <ESP_WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Wire.h>
#include <NMEA2000_CAN.h>
#include <NMEA2000.h>
#include <N2kMessages.h>
#include "NMEA0183Telegram.h"


Preferences preferences;


#include <SparkFun_MMA8452Q.h>
MMA8452Q mma;
bool bMMA_Status = 0;


#include <LIS3MDL.h>
#include <LSM6.h>
#include <Adafruit_LSM6DSOX.h>
Adafruit_LSM6DSOX lsm;

LIS3MDL mag;

LIS3MDL::vector<int16_t> m_min = {-32767, -32767, -32767}, m_max = {+32767, +32767, +32767};
bool bGyroMag_Status = 0;
char report[80];


#include "oled91.h"


WiFiUDP udp;
void SendNMEA0183Message(String var);
bool IsTimeToUpdate(unsigned long NextUpdate);
void SetNextUpdate(unsigned long &NextUpdate, unsigned long Period);
void SendN2kPitch(double Yaw, double Pitch, double Roll);
void SendN2kHeading(double Heading);
void setup();
void loop();
#line 55 "C:/Users/gerryadmin/Documents/Bootsdaten/src/bootsdaten.ino"
void SendNMEA0183Message(String var) {
  udp.beginPacket(udpAddress, udpPort);
  udp.println(var);
  udp.endPacket();
}


const unsigned long TransmitMessages[] PROGMEM = {127257L,
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


void SendN2kPitch(double Yaw, double Pitch, double Roll) {
  static unsigned long SlowDataUpdated = InitNextUpdate(SlowDataUpdatePeriod, PGN1SendOffset);
  tN2kMsg N2kMsg;

  if ( IsTimeToUpdate(SlowDataUpdated) ) {
    SetNextUpdate(SlowDataUpdated, SlowDataUpdatePeriod);

    Serial.printf("Krängung			: %3.1f °\n",Yaw);
    Serial.printf("Gieren			: %3.1f °\n",Pitch);
    Serial.printf("Rollen			: %3.1f °\n",Roll);

    SetN2kPGN127257(N2kMsg, 0, Yaw, Pitch, Roll);
    NMEA2000.SendMsg(N2kMsg);
  }
}

void SendN2kHeading(double Heading) {
  static unsigned long SlowDataUpdated = InitNextUpdate(SlowDataUpdatePeriod, PGN2endOffset);
  tN2kMsg N2kMsg;

  if ( IsTimeToUpdate(SlowDataUpdated) ) {
    SetNextUpdate(SlowDataUpdated, SlowDataUpdatePeriod);

    Serial.printf("Heading     		: %3.1f °\n", Heading);

    SetN2kMagneticHeading(N2kMsg, 2, Heading, N2kDoubleNA, N2kDoubleNA);
    NMEA2000.SendMsg(N2kMsg);
  }
}


void setup()
{
  Serial.begin(115200);

  Serial.printf("BD Sensor setup %s start\n", Version);


  if (!LittleFS.begin(true)) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  Serial.println("Speicher LittleFS benutzt:");
  Serial.println(LittleFS.usedBytes());

  File root = LittleFS.open("/");
  listDir(LittleFS, "/", 3);


  readConfig("/config.json");
  AP_SSID = tWeb_Config.wAP_SSID;
  AP_PASSWORD = tWeb_Config.wAP_Password;
  fKielOffset = atof(tWeb_Config.wKiel_Offset);

  Serial.println("\nConfigdata : AP SSID: " + AP_SSID + " , Passwort: " + AP_PASSWORD + " , Kieloffset: " + fKielOffset);

  pinMode(iMaxSonar, INPUT);


  Wire.begin(I2C_SDA, I2C_SCL);
  I2C_scan();

  oledsetup();
  testdrawbitmap();
  oledCoursor0(Version);
  delay(1000);


  switch (mma.init()) {
    case 0:
      Serial.println("\nGyro MMA could not start!");
      bMMA_Status = 0;
      break;
    case 1:
      Serial.println("\nMMA found!");
      Serial.println(I2C_address);
      mma.init(SCALE_2G);
      Serial.print("Range = "); Serial.print(2 << mma.available());
      Serial.println("G");
      bMMA_Status = 1;
  }

  switch (lsm.begin_I2C()) {
    case 0:
      Serial.println("\nGyro LSM6 could not start!");
      bGyroMag_Status = 0;
      break;
    case 1:
      Serial.println("\nGyro LSM6 found!");
      lsm.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS);
      lsm.setGyroDataRate(LSM6DS_RATE_12_5_HZ);
      lsm.setAccelRange(LSM6DS_ACCEL_RANGE_8_G);
      lsm.setAccelDataRate(LSM6DS_RATE_12_5_HZ);
      bGyroMag_Status = 1;
  }


  bool MAGbegin = mag.init();
  switch (MAGbegin) {
    case 0:
      Serial.println("\nCompass could not start!");
      break;
    case 1:
      Serial.println("\nCompass found!");
      mag.enableDefault();
  }


  if (bGyroMag_Status ==1) {
    sI2C_Status = "Gyro LSM6 aktiv!";
    oledCoursor10(sI2C_Status);
    bI2C_Status = 1;
  }
  if (bMMA_Status ==1) {
    sI2C_Status = "Gyro MMA8452Q aktiv!";
    oledCoursor10(sI2C_Status);
    bI2C_Status = 1;
  }
  if (!bGyroMag_Status && !bMMA_Status) {
    sI2C_Status = "Gyro nicht gefunden!";
    oledCoursor10(sI2C_Status);
  }


  callab_setup();


  LEDInit();


  sBoardInfo = boardInfo.ShowChipIDtoString();



  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  delay(1000);
  if (WiFi.softAPConfig(IP, Gateway, NMask)){
    Serial.println("Network " + String(AP_SSID) + " running");
    Serial.println("\nNetwork IP " + IP.toString() + " ,GW: " + Gateway.toString() + " ,Mask: " + NMask.toString() + " set");
    oledCoursor0(IP.toString());
  } else {
    Serial.println("IP config not success");
  }

  if (!WiFi.setHostname(HostName))
    Serial.println("\nSet Hostname success");
  else
    Serial.println("\nSet Hostname not success");

  delay(1000);

  WiFiDiag();

  if (!MDNS.begin(HostName)) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");


  server.begin();
  Serial.println("TCP server started\n");


  MDNS.addService("http", "tcp", 80);


  website();



  NMEA2000.SetN2kCANMsgBufSize(8);
  NMEA2000.SetN2kCANReceiveFrameBufSize(250);
  NMEA2000.SetN2kCANSendFrameBufSize(250);

  esp_efuse_mac_get_default(chipid);
  for (i = 0; i < 6; i++) id += (chipid[i] << (7 * i));


  NMEA2000.SetProductInformation("BD01",
                                 100,
                                 "BD Sensor Module",
                                 "2.0.0.00 (2024-09-30)",
                                 "1.0.3.0 (2023-09-30)"
                                );

  NMEA2000.SetDeviceInformation(01,
                                180,
                                40,
                                2046
                               );

  NMEA2000.SetForwardType(tNMEA2000::fwdt_Text);

  preferences.begin("nvs", false);
  NodeAddress = preferences.getInt("LastNodeAddress", 34);
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

template <typename T> float computeHeading(LIS3MDL::vector<T> from)
{
  LIS3MDL::vector<int32_t> temp_m = {mag.m.x, mag.m.y, mag.m.z};


  LIS3MDL::vector<int16_t> a = {lsm.rawGyroX, lsm.rawGyroY, lsm.rawGyroZ};


  temp_m.x -= ((int32_t)m_min.x + m_max.x) / 2;
  temp_m.y -= ((int32_t)m_min.y + m_max.y) / 2;
  temp_m.z -= ((int32_t)m_min.z + m_max.z) / 2;


  LIS3MDL::vector<float> E;
  LIS3MDL::vector<float> N;
  LIS3MDL::vector_cross(&temp_m, &a, &E);
  LIS3MDL::vector_normalize(&E);
  LIS3MDL::vector_cross(&a, &E, &N);
  LIS3MDL::vector_normalize(&N);


  float heading = atan2(LIS3MDL::vector_dot(&E, &from), LIS3MDL::vector_dot(&N, &from)) * 180 / PI;
  if (heading < 0) heading += 360;
  return heading;

  LEDoff();
}

void loop()
{

  bConnect_CL = WiFi.status() == WL_CONNECTED ? 1 : 0;



  LEDflash(LED(Green));
  if (bI2C_Status == 0){

    Serial.print("Sensorfehler\n");
  }


  ArduinoOTA.handle();


  Serial.printf("Stationen mit AP verbunden = %d\n", WiFi.softAPgetStationNum());
  Serial.printf("Soft-AP IP address = %s\n", WiFi.softAPIP().toString());
  sCL_Status = sWifiStatus(WiFi.status());


  uint8_t Orientation = 0;
  if (bMMA_Status == 1) {
    Serial.print("Read MMA 8452\n");
    fKraengung = mma.getX() / 11.377;
    fKraengung = (abs(fKraengung));
    fGaugeKraengung = mma.getX() / 11.377;
    fGieren = mma.getY() / 11.377;
    fRollen = mma.getZ() / 11.377;
    Serial.print("MMA 8452 auslesen:\n");
    Serial.printf("X, Krängung: %f °\n", fKraengung);
    Serial.printf("Y, Gieren  : %f °\n", fGieren);
    Serial.printf("Z, Rollen  : %f °\n", fRollen);
    Orientation = mma.readPL();
  }

  float fTemperatur = 0.0;
  if (bGyroMag_Status == 1) {

    sensors_event_t accel;
    sensors_event_t gyro;
    sensors_event_t temp;
    lsm.getEvent(&accel, &gyro, &temp);

    fKraengung = atan2(-lsm.rawGyroX, sqrt(lsm.rawGyroY * lsm.rawGyroY + lsm.rawGyroZ * lsm.rawGyroZ)) * 180.0 / PI;
    fGaugeKraengung = fKraengung;

    fGieren = atan2(sqrt(lsm.rawGyroY * lsm.rawGyroY + lsm.rawGyroZ * lsm.rawGyroZ), lsm.rawGyroX) * 180 / PI;

    fRollen = atan2(lsm.rawGyroY, lsm.rawGyroZ) * 180.0 / PI;
    fTemperatur = temp.temperature;

    Serial.print("Gyro LSM6 auslesen:\n");
    Serial.printf("X, Krängung: %f °\n", fKraengung);
    Serial.printf("Y, Gieren  : %f °\n", fGieren);
    Serial.printf("Z, Rollen  : %f °\n", fRollen);
    Serial.printf("Temperatur : %f °C\n", fTemperatur);
  }


  if (fKraengung < -1)
    sSTBB = "Backbord";
  else sSTBB = "Steuerbord";
  Serial.printf("Kraengung nach: %s %f °\n", sSTBB, fKraengung);


  bool bSFM = 0;
  switch (Orientation) {
    case PORTRAIT_U: sOrient = "Oben"; break;
    case PORTRAIT_D: sOrient = "Unten";
      bSFM = 1;
      Serial.print("Sensor falsch montiert\n");
      break;
    case LANDSCAPE_R: sOrient = "Rechts"; break;
    case LANDSCAPE_L: sOrient = "Links"; break;
    case LOCKOUT: sOrient = "Horizontal";
      bSFM = 1;
      Serial.print("Sensor falsch montiert\n");
      break;
  }
  Serial.printf("Orientation: %s\n", sOrient);


  if (bSFM == 0 && bI2C_Status == 1)
  {
    if (sSTBB == "Backbord")
    {

    }
    else
    {

    }
  }
  else if (bI2C_Status == 1)
  {
    digitalWrite(LED(Blue), HIGH);
  }


  if (bGyroMag_Status == 1) {
    mag.read();
    Serial.print("Read LIS3 Compass\n");
    m_min.x = min(m_min.x, mag.m.x);
    m_min.y = min(m_min.y, mag.m.y);
    m_min.z = min(m_min.z, mag.m.z);
    m_max.x = max(m_max.x, mag.m.x);
    m_max.y = max(m_max.y, mag.m.y);
    m_max.z = max(m_max.z, mag.m.z);
    snprintf(report, sizeof(report), "min: {%+6d, %+6d, %+6d}   max: {%+6d, %+6d, %+6d}",
      m_min.x, m_min.y, m_min.z,
      m_max.x, m_max.y, m_max.z);
    Serial.println(report);

    fheading = computeHeading((LIS3MDL::vector<int>){1, 0, 0});
    fheadingRad = DegToRad(fheading);
    Serial.printf("Heading (Grad): %f °\n", fheading);
    Serial.printf("Heading (Radian): %f °\n", fheadingRad);
  }


  callab_loop();


  iDistance = analogRead(iMaxSonar);
  Serial.printf("Analogeingang: %i\n", iDistance);
  int Err = 0;
  fSStellung = analogInScale(iDistance, 3800, 300, 7.0, 80.0, Err);
  Serial.printf("Schwert: %f cm\n", fSStellung);
  fAbsTief = fSStellung + fKielOffset;
  Serial.printf("Tiefgang: %f cm\n", fAbsTief);


  SendNMEA0183Message(sendXDR());

  delay(200);



  SendN2kPitch(DegToRad(fKraengung), DegToRad(fRollen), DegToRad(fGieren));
  SendN2kHeading(fheadingRad);

  delay(200);

  Serial.print("NMEA2000.ParseMessages............\n");

  NMEA2000.ParseMessages();
  int SourceAddress = NMEA2000.GetN2kSource();
  if (SourceAddress != NodeAddress) {
    NodeAddress = SourceAddress;
    preferences.begin("nvs", false);
    preferences.putInt("LastNodeAddress", SourceAddress);
    preferences.end();
    Serial.printf("Address Change: New Address=%d\n", SourceAddress);
  }


  if ( Serial.available() ) {
    Serial.read();
  }





  freeHeapSpace();

  if (IsRebootRequired) {
    Serial.println("Rebooting ESP32: ");
    oledCoursor10("Rebooting ESP32: ");
    delay(1000);
    ESP.restart();
  }

  if ((!mma.isUp()) && (!lsm.begin_I2C())) bI2C_Status = 0;
}