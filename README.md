#BD Sensor Modul

The ESP32 in this project is an Adafruit Huzzah! ESP32. This is a small module without USB connector.

With the ESP32 and 2 Sensors for gyro (MMA8452) and distance (Sharp GP2Y0A21) it's possible
monitoring the varaible keel (Kiel) and the boat-position Yaw, Pitch, Roll (Krängung, Rollen, Gieren). 
All data is available in the web interface. 
Roll-data are available as NMEA0183 UDP-Stream over Wlan.
NMEA2000 are Yaw/Roll/Pitch available with PGN 127257.
It's a visualisation per LED (Kraengung red/green) possible, uncomment LED output for this in the loop.

The 12 Volt is reduced to 5 Volt with a DC Step-Down_Converter. 12V DC comes from the N2k Bus Connector with the M12 Connector.

The Website use LittleFS Filesystem. You must use Partition Schemes "Minimal SPIFFS with APPS and OTA".
The HTML Data upload separately with 
- Arduino IDE > "ESP 32 Sketch Data upload" or 
- VS-Code > Build Filesystem and Upload Filesystem Image (PlatformIO) 
from /data directory.

# Physik:

![strauch_02_03_035](https://github.com/gerryvel/Bootsdaten/assets/17195231/089ffe03-a30e-45d0-bf36-3a9d06e02bc7)


# Partlist:

- Adafruit Huzzah! ESP32 (for programming need USB-Adapter)[Link](https://www.exp-tech.de/plattformen/internet-of-things-iot/9350/adafruit-huzzah32-esp32-breakout-board)
- SN65HVD230 [Link](https://www.reichelt.de/high-speed-can-transceiver-1-mbit-s-3-3-v-so-8-sn-65hvd230d-p58427.html?&trstct=pos_0&nbc=1)
- Traco-Power TSR 1-2450 for 12V / 5V [Link](https://www.reichelt.de/dc-dc-wandler-tsr-1-1-w-5-v-1000-ma-sil-to-220-tsr-1-2450-p116850.html?search=tsr+1-24)
- Gyro MMA8452Q [Link](https://www.reichelt.de/entwicklerboards-beschleunigungsmesser-board-mma8452q-debo-sens-acc3-p284397.html)
- Case Wago 789 [Link](https://www.conrad.de/de/p/wago-789-905-hutschienen-gehaeuse-90-x-17-5-x-55-polyamid-6-6-grau-1-set-530120.html)
- Resistor 200Ohm , 10kOhm


# Wiring diagram

![grafik](https://github.com/gerryvel/Bootsdaten/assets/17195231/5571a0f5-8a37-4b18-a9da-5ba11bb2f8b1)

# PCB and Housing assembly

![photo_2023-07-20_13-50-08](https://github.com/gerryvel/Bootsdaten/assets/17195231/ef5a9be6-c718-4481-8ee6-a68689e1808c)

![Bootsdaten](https://github.com/gerryvel/Bootsdaten/assets/17195231/b4be1809-5393-4396-8dcf-747c5ca8a09e)

PCB by Aisler [Link](https://aisler.net/p/NZFHAMAJ)

# Webinterface

![image](https://github.com/user-attachments/assets/d4dc4f33-6e58-43d9-b7c4-10e848a5ee16)

![image](https://github.com/user-attachments/assets/f69d6575-1ac2-499b-8d04-a23bc08548e9)

![image](https://github.com/user-attachments/assets/2348caff-3b6f-46d9-891f-991ea3ca2e12)

![image](https://github.com/user-attachments/assets/b88cad09-4c7a-4b57-999e-0fbe037e8dbd)

# Plotter

![IMG_2314](https://github.com/gerryvel/Bootsdaten/assets/17195231/febcb30e-3672-4694-8fb3-9ba91a55eb29)

![IMG_2316](https://github.com/gerryvel/Bootsdaten/assets/17195231/5e494e51-6be4-4165-be44-a78ecafa7947)


# Versions

- 2.3 Add Pololu MinIMU-9 V5 with Compassmodul, add Website and programm
- 2.2 Update PCB
- 2.1 Update Website Gauge's
- 2.0 Complete Website update, NMEA0183 Stream output
- 1.3 Update Windsensor Wlan
- 1.2 Update Website
- 1.1 Update PCB
- 1.0 working Version
