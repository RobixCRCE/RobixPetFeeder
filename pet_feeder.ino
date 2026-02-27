#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include "HX711.h"
#include <EEPROM.h>

// -------------------- Load Cell --------------------
const int LOADCELL_DOUT_PIN = 16;
const int LOADCELL_SCK_PIN = 4;
HX711 scale;

float defaultCalibration = 207.66f;
float calibrationFactor;

const int EE_CAL_ADDR = 0;

float displayWeight = 0.0f;
static float filteredWeight = 0.0f;
bool firstSampleTaken = false;
unsigned long lastSampleMillis = 0;
const unsigned long sampleIntervalMs = 300;

// -------------------- Water Level Params --------------------
float maxDist = 24.4;   // Distance when bowl is empty
float minDist = 20.0;   // Distance when bowl is full
float waterPercent = 0.0;

// -------------------- Pump + Servo --------------------
#define PUMP_PIN 26
#define SERVO_PIN 27
Servo servoMotor;

bool feedDirection = false; 
bool pumpState = false;

// -------------------- Ultrasonic --------------------
#define TRIG_PIN 32
#define ECHO_PIN 33
float displayDistance = 0.0f;  // cm

float readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 25000);
  if (duration == 0) return displayDistance; // keep last value

  float distance = duration * 0.0343 / 2.0;

  static float smoothed = distance;
  smoothed = (smoothed * 0.8) + (distance * 0.2);
  return smoothed;
}

// -------------------- WiFi --------------------
const char* ssid = "ESP32_Feeder";
const char* password = "12345678";
WebServer server(80);

// -------------------- EEPROM Helpers --------------------
void saveCalibration(float value) {
  EEPROM.put(EE_CAL_ADDR, value);
  EEPROM.commit();
}
float loadCalibration() {
  float value;
  EEPROM.get(EE_CAL_ADDR, value);
  if (isnan(value) || value < 10 || value > 10000) {
    return defaultCalibration;
  }
  return value;
}

// -------------------- Webpage --------------------
String htmlPage() {
  String page = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  page += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  page += "<title>Smart Feeder</title>";
  page += "<style>body{font-family:sans-serif;text-align:center;background:#f4f4f4;padding:20px;}"
          "button{padding:14px 30px;margin:8px;font-size:18px;border:none;border-radius:10px;cursor:pointer;}"
          ".on{background:#4CAF50;color:white;}.off{background:#f44336;color:white;}";
  page += "</style></head><body>";

  page += "<h1>🐾 Smart Pet Feeder</h1>";
  page += "<h2>Current Food Weight: " + String(displayWeight, 1) + " g</h2>";
  page += "<h2>Water Level: " + String(waterPercent, 1) + "%</h2>";

  if (pumpState)
    page += "<form action='/fill'><button class='off'>STOP WATER</button></form>";
  else
    page += "<form action='/fill'><button class='on'>FILL WATER</button></form>";

  page += "<form action='/servo_toggle'><button class='on'>FEED</button></form>";

  page += "<div>Pump is " + String(pumpState ? "ON" : "OFF") + "</div>";
  page += "</body></html>";
  return page;
}

// -------------------- Web Handlers --------------------
void handleRoot() { server.send(200, "text/html", htmlPage()); }

void handleFill() {
  pumpState = !pumpState;
  digitalWrite(PUMP_PIN, pumpState ? HIGH : LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleServoToggle() {
  if (!feedDirection) { servoMotor.write(0); delay(500); servoMotor.write(90); }
  else { servoMotor.write(180); delay(500); servoMotor.write(90); }
  feedDirection = !feedDirection;
  server.sendHeader("Location", "/");
  server.send(303);
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);

  EEPROM.begin(32);
  calibrationFactor = loadCalibration();
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibrationFactor);

  delay(2000);
  scale.tare();

  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);

  servoMotor.setPeriodHertz(50);
  servoMotor.attach(SERVO_PIN, 500, 2400);
  servoMotor.write(90);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  WiFi.softAP(ssid, password);
  server.on("/", handleRoot);
  server.on("/fill", handleFill);
  server.on("/servo_toggle", handleServoToggle);
  server.begin();

  firstSampleTaken = false;
  lastSampleMillis = millis();
}

// -------------------- Loop --------------------
void loop() {
  server.handleClient();

  unsigned long now = millis();
  if (now - lastSampleMillis >= sampleIntervalMs) {
    lastSampleMillis = now;

    // Load Cell
    if (scale.is_ready()) {
      float raw = scale.get_units(5);
      if (!firstSampleTaken) { filteredWeight = raw; firstSampleTaken = true; }
      else { filteredWeight = (0.85f * filteredWeight) + (0.15f * raw); }
      displayWeight = round(filteredWeight * 10.0f) / 10.0f;
    }

    // Ultrasonic
    displayDistance = readDistanceCM();

    // ---- WATER LEVEL PERCENT CALC ----
    if (displayDistance <= minDist) {
      waterPercent = 100;
    }
    else if (displayDistance >= maxDist) {
      waterPercent = 0;
    }
    else {
      waterPercent = ((maxDist - displayDistance) / (maxDist - minDist)) * 100.0;
    }
  }
}
