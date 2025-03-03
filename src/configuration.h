#ifndef __configuration__H__
#define __configuration__H__

#include <Arduino.h>

// Versionierung
#define Version "V2.3 vom 19.11.2024"  // Version

// Configuration N2k
#define ESP32_CAN_TX_PIN GPIO_NUM_4  // Set CAN TX port to 4 
#define ESP32_CAN_RX_PIN GPIO_NUM_5  // Set CAN RX port to 5
#define N2K_SOURCE 15
int NodeAddress;                     // To store Last Node Address
uint8_t chipid[6];
uint32_t id = 0;
int i = 0;
#define PGN1SendOffset 0
#define PGN2endOffset 40
#define PGN3SendOffset 80
#define BPGN4SendOffset 100
#define SlowDataUpdatePeriod 1000  // Time between CAN Messages sent

//Configuration Web Page 
#define PAGE_REFRESH 10 // x Sec.
#define WEB_TITEL "Bootsdaten"

//Configuration mit Webinterface
struct Web_Config
{
	char wAP_IP[20];
	char wAP_SSID[64];
	char wAP_Password[12];
	char wKiel_Offset[5];
};
Web_Config tWeb_Config;

//Configuration AP 
#define HostName        "Bootsdaten"
const int   channel        = 10;                // WiFi Channel number between 1 and 13
const bool  hide_SSID      = false;             // To disable SSID broadcast -> SSID will not appear in a basic WiFi scan
const int   max_connection = 4;                 // Maximum simultaneous connected clients on the AP

// Variables for WIFI-AP
IPAddress IP = IPAddress(192, 168, 15, 20);
IPAddress Gateway = IPAddress(192, 168, 15, 1);
IPAddress NMask = IPAddress(255, 255, 255, 0);
IPAddress AP_IP;
String AP_SSID = "Bootsdaten";
String AP_PASSWORD  = "12345678";
IPAddress CL_IP;
IPAddress SELF_IP;

//Configuration Client (Network Data Windsensor)
#define CL_SSID      "NoWa"					//Windmesser
#define CL_PASSWORD  "12345678"		
int iSTA_on = 0;                            // Status STA-Mode
int bConnect_CL = 0;
bool bClientConnected = 0;

//Confuration Sensors I2C
#define I2C_SDA 21                      //Standard 21
#define I2C_SCL 22                      //Standard 22
#define SEALEVELPRESSURE_HPA (1013.25)  //1013.25
float fbmp_temperature = 0;
float fbmp_pressure = 0;
float fbmp_altitude = 0;
String sI2C_Status = "";
bool bI2C_Status = 0;
int I2C_Typ = 0;
String I2C_address = "";

// Bootsdaten > Krängung etc
double fKraengung = 0;				//MMA
float fGaugeKraengung = 0;
double fGieren = 0;
double fRollen = 0;
String sSTBB = "";
String sOrient = "";

float fheading = 0;					//Compass
float fheadingRad = 0;

const int iMaxSonar = 35;			//Analoginput 
int iDistance = 0;
float fSStellung = 0;
float fAbsTief = 0;
float fKielOffset = 0;


//Definiton NMEA0183 MWV
double dMWV_WindDirectionT = 0;
double dMWV_WindSpeedM = 0;
double dVWR_WindDirectionM = 0;
double dVWR_WindAngle = 0;
double dVWR_WindSpeedkn = 0;
double dVWR_WindSpeedms = 0;

//Configuration NMEA0183
#define SERVER_HOST_NAME "192.168.4.1"		//"192.168.76.34"
#define TCP_PORT 6666						//6666
#define DNS_PORT 53

//Variable NMEA 0183 Stream
const char *udpAddress = "192.168.15.255"; // Set network address for broadcast
const int udpPort = 4444;                 // UDP port

#endif  
