# Bootsdaten

With ESP32 and 2 Sensors for gyro (MMA8452) and distance (MB1043 HRLV-MaxSonar-EZ4) it's possible
monitoring the varaible Kiel and the Inclined position (Krängung).
For the Voltage regulation use the Pololu 5V, 1A Step-Down Voltage Regulator D24V10F5.

The Website use SPIFFS Filesystem. You must use Partition Schemes "Minimal SPIFFS with APPS and OTA".
The HTML Data upload seperately with "ESP 32 Scetch Data upload" from /data directory.

