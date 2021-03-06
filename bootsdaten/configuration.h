#ifndef CONFIGURATION_H
#define CONFIGURATION_H


// Configuration N2k
#define ESP32_CAN_TX_PIN GPIO_NUM_5  // Set CAN TX port to 5 
#define ESP32_CAN_RX_PIN GPIO_NUM_4  // Set CAN RX port to 4
#define N2K_SOURCE 15

//Configuration Refresh Page x Sec.
#define PAGE_REFRESH 10 

//Configuration AP 
struct AP_config
{
	char AP_IP[20];
	char AP_SSID[64];
	char AP_Password[12];
	char Kiel_Offset[5];
};
AP_config AP_Config;


// Variables for AP
IPAddress IP = IPAddress(192, 168, 16, 1);
IPAddress Gateway = IPAddress(192, 168, 16, 1);
IPAddress NMask = IPAddress(255, 255, 255, 0);
IPAddress AP_IP;
String AP_Name = "";

//Configuration Client
#define CL_SSID      "NoWa"					
#define CL_PASSWORD  "12345678"	
IPAddress CL_IP;
IPAddress SELF_IP;
bool bAP_on = 0;
bool bConnect_CL = 0;
bool bClientConnected = 0;

//Confuration Sensors
#define MMA_STA 21
#define MMA_SCL 22
const int iMaxSonar = 34;			//Analoginput 
int iDistance = 0;
float fSStellung = 0;
//float fOffset = 30;
float fAbsTief = 0;
float fKraengung = 0;
float fGaugeKraengung = 0;
String sOrient = "";
String sSTBB = "";

//Definiton NMEA0183

//Configuration NMEA0183
#define SERVER_HOST_NAME "192.168.4.1"		//IP Windmesser NoWa
#define TCP_PORT 6666						//Port Windmeser Nowa
#define DNS_PORT 53

//Confuguration SPIFFS
int SPIFFbytes = 0;

//Boardinfo
u32_t iFreeHeap = 0;

#endif