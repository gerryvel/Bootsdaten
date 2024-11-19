/*
    Name:       bootsdaten.ino
    Created:	22.10.2020
	Update: 	V2.3 vom 19.11.2024
    Author:     Gerry Sebb
*/

#include <Arduino.h>
#include <ArduinoOTA.h>
#include "BoardInfo.h"
#include "configuration.h"
#include "helper.h"
#include "web.h"
#include "Analog.h"
#include "LED.h"
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

// NMEA2000
Preferences preferences;             // Nonvolatile storage on ESP32 - To store LastDeviceAddress

//MMA8452Q Board
#include <SparkFun_MMA8452Q.h>
MMA8452Q mma;
bool bMMA_Status = 0;

//LSM6 & LIS3 Board
#include <LIS3MDL.h>
#include <LSM6.h>
LSM6 gyro;
LIS3MDL mag;
LIS3MDL::vector<int16_t> m_min = {32767, 32767, 32767}, m_max = {-32768, -32768, -32768};
bool bGyro_Status = 0;
char report[80];

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

// The setup() function runs once each time the micro-controller starts
void setup()
{

	Serial.begin(115200);

	Serial.printf("BD Sensor setup %s start\n", Version);

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
	
// Gyro LSM6
	bool Gyrobegin = gyro.init();
	switch (Gyrobegin) {
	case 0:
		Serial.println("\nGyro LSM6 could not start!");
		bGyro_Status = 1;
		break;
	case 1:
		Serial.println("\nGyro LSM6 found!");	
		gyro.enableDefault();	
		bGyro_Status = 1;        
	}

//Compass
	bool MAGbegin = mag.init();
	switch (MAGbegin) {
	case 0:
		Serial.println("\nCompass could not start!");
		break;
	case 1:
		Serial.println("\nCompass found!");
		mag.enableDefault();
	}

// Set I2C Status 
	if (bGyro_Status ==1)
		sI2C_Status = "Gyro LSM6 aktiv!";
		bI2C_Status = 1;
	if (bMMA_Status ==1)
		sI2C_Status = "Gyro MMA8452Q aktiv!";
		bI2C_Status = 1;
	if (!bGyro_Status && !bMMA_Status)
		sI2C_Status = "Gyro nicht gefunden!";	

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
	Serial.print("AP IP configured with address: \n");
	Serial.println(myIP);
	

	if (!MDNS.begin(HostName)) {
		Serial.println("Error setting up MDNS responder!");
		while (1) {
			delay(1000);
		}
	}
	Serial.println("mDNS responder started");

	// Start TCP (HTTP) server
	server.begin();
	Serial.println("TCP server started\n");

	// Add service to MDNS-SD
	MDNS.addService("http", "tcp", 80);

	//Website
	website();

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

template <typename T> float computeHeading(LIS3MDL::vector<T> from)
{
  LIS3MDL::vector<int32_t> temp_m = {mag.m.x, mag.m.y, mag.m.z};

  // copy acceleration readings from LSM6::vector into an LIS3MDL::vector
  LIS3MDL::vector<int16_t> a = {gyro.a.x, gyro.a.y, gyro.a.z};

  // subtract offset (average of min and max) from magnetometer readings
  temp_m.x -= ((int32_t)m_min.x + m_max.x) / 2;
  temp_m.y -= ((int32_t)m_min.y + m_max.y) / 2;
  temp_m.z -= ((int32_t)m_min.z + m_max.z) / 2;

  // compute E and N
  LIS3MDL::vector<float> E;
  LIS3MDL::vector<float> N;
  LIS3MDL::vector_cross(&temp_m, &a, &E);
  LIS3MDL::vector_normalize(&E);
  LIS3MDL::vector_cross(&a, &E, &N);
  LIS3MDL::vector_normalize(&N);

  // compute heading
  float heading = atan2(LIS3MDL::vector_dot(&E, &from), LIS3MDL::vector_dot(&N, &from)) * 180 / PI;
  if (heading < 0) heading += 360;
  return heading;

	LEDoff();

}


void loop()
{
	//Wifi variables
	bConnect_CL = WiFi.status() == WL_CONNECTED ? 1 : 0;
	//bAP_on = WiFi.enableAP(bAP_on);
	
	// LED visu Wifi
	LEDflash(LED(Green)); // Betrieb ok
	if (bI2C_Status == 0){
		//LEDflash(LED(Red)); // Sensorfehler
		Serial.print("Sensorfehler\n");
	} 

// OTA	
	ArduinoOTA.handle();

// Status AP 
  Serial.printf("Stationen mit AP verbunden = %d\n", WiFi.softAPgetStationNum());
  Serial.printf("Soft-AP IP address = %s\n", WiFi.softAPIP().toString());
  sCL_Status = sWifiStatus(WiFi.status());

// read MMA
uint8_t Orientation = 0;
if (bMMA_Status == 1) {
	fKraengung = mma.getX() / 11.377;
	fKraengung = (abs(fKraengung));
	fGaugeKraengung = mma.getX() / 11.377;
	fGieren = mma.getY() / 11.377;
	fRollen = mma.getZ() / 11.377;
	Serial.print("MMA auslesen:\n");
	Serial.printf("X, Krängung: %f °\n", fKraengung);
	Serial.printf("Y, Gieren: %f °\n", fGieren);
	Serial.printf("Z, Rollen: %f °\n", fRollen);
	Orientation = mma.readPL();
}

// read Gyro
float fGyroTemp = 0;
if (bGyro_Status == 1) {
	gyro.read();
	fKraengung = atan2(-gyro.a.x, sqrt(gyro.a.y * gyro.a.y + gyro.a.z * gyro.a.z)) * 180.0 / PI;
	fGaugeKraengung = fKraengung;
	fGieren = atan2(sqrt(gyro.a.y * gyro.a.y + gyro.a.z * gyro.a.z), gyro.a.x) * 180 / PI;
	fRollen = atan2(gyro.a.y, gyro.a.z) * 180.0 / PI;
	Serial.print("IMU auslesen:\n");
	Serial.printf("X, Krängung: %f °\n", fKraengung);
	Serial.printf("Y, Gieren: %f °\n", fGieren);
	Serial.printf("Z, Rollen: %f °\n", fRollen);
	Serial.printf("Temperatur: %f °C\n", fGyroTemp);
}

// Direction Kraengung
	if (fKraengung < -1)
		sSTBB = "Backbord";
	else sSTBB = "Steuerbord";
	Serial.printf("Kraengung nach: %s %f °\n", sSTBB, fKraengung);

// read I2C Orientation Sensor
	bool bSFM = 0; // SensorFalschMontiert
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

// LED Kraengung, LED aktivieren wenn am Modul STB(green) / BB(red) LED-Anzeige gewünscht
	if (bSFM == 0 && bI2C_Status == 1)
	{
		if (sSTBB == "Backbord")
		{
			//digitalWrite(LED(Red), HIGH);
		}
		else 
		{
			//digitalWrite(LED(Green), HIGH);
		}
	}
	else if (bI2C_Status == 1)
	{
		digitalWrite(LED(Blue), HIGH);
	}
	
// compass read
	mag.read();

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


//AI Distance-Sensor
	iDistance = analogRead(iMaxSonar);
	Serial.printf("Analogeingang: %i\n", iDistance);
	int Err = 0;
	fSStellung = analogInScale(iDistance, 3800, 300, 7.0, 80.0, Err);
	Serial.printf("Schwert: %f cm\n", fSStellung);
	fAbsTief = fSStellung + fKielOffset;
	Serial.printf("Tiefgang: %f cm\n", fAbsTief);

// Send Messages NMEA 0183
	SendNMEA0183Message(sendXDR()); // Send NMEA0183 Message

	delay(200);

// Send Messages NMEA 2000

	SendN2kPitch(DegToRad(fKraengung), DegToRad(fRollen), DegToRad(fGieren));
	SendN2kHeading(fheadingRad);

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

// Website Data	
	freeHeapSpace();
	
	if (IsRebootRequired) {
		Serial.println("Rebooting ESP32: "); 
		delay(1000); // give time for reboot page to load
		ESP.restart();
		}

if ((!mma.isUp()) && (!gyro.init())) bI2C_Status = 0;


}
