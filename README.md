#Smart Pet Feeder – ESP32 (Access Point Mode)

Overview
ESP32-based smart pet feeder with:
Food weight measurement (HX711 load cell)
Water level monitoring (Ultrasonic sensor)
Pump ON/OFF control
Servo-based food dispensing
Built-in WiFi hotspot with web interface
ESP32 runs in Access Point mode (no internet required).

Hardware Required: 
ESP32 Development Board
HX711 Load Cell Amplifier
Load Cell (5kg recommended)
HC-SR04 Ultrasonic Sensor
Servo Motor (SG90/MG90S)
5V Water Pump
L298N or transistor module
External 5V power supply

Pin Configuration:
Component	GPIO
HX711 DOUT	16
HX711 SCK	4
Pump	26
Servo	27
Ultrasonic TRIG	32
Ultrasonic ECHO	33
All grounds must be connected together.

Software Requirements:
1. Arduino IDE
Download: https://www.arduino.cc/en/software

2. Install ESP32 Board:
Add this to Preferences → Additional Board URLs:
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
Then install ESP32 by Espressif Systems from Boards Manager.
Select: Tools → Board → ESP32 Dev Module

Required Libraries:
Install via Library Manager,
ESP32Servo (Kevin Harrington)
HX711 (Bogde)
WiFi, WebServer, EEPROM are included with ESP32 core.
WiFi Access

After uploading code:
SSID: ESP32_Feeder
Password: 12345678
Open browser and go to:
192.168.4.1

Features:
Displays food weight (grams)
Displays water level (%)
Toggle water pump ON/OFF
Servo food dispensing
EEPROM calibration storage
Sensor filtering for stable readings
Power Notes

Use external 5V supply for servo and pump

Connect all GNDs together

Do not power servo directly from ESP32 if unstable
