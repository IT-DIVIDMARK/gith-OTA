#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

// ===== WiFi credentials =====
const char* ssid = "admin";
const char* password = "password";

// ===== OTA configuration =====
#define DEVICE_ID 2
#define FIRMWARE_VERSION "1.0.15"

const char* versionURL = "https://raw.githubusercontent.com/IT-DIVIDMARK/gith-OTA/main/version.json";
String firmwareURL;

// ===== Function prototypes =====
bool checkForUpdate();
void downloadFirmware(String url);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nStarting OTA updater...");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected: " + WiFi.localIP().toString());

    // Check for OTA update
    if (checkForUpdate()) {
      Serial.println("New firmware found. Updating...");
      downloadFirmware(firmwareURL);
    } else {
      Serial.println("Device already running latest firmware or not targeted.");
    }
  } else {
    Serial.println("\nWiFi connection failed.");
  }
}

void loop() {
  // Nothing here — updates handled in setup
}

// ===== OTA Functions =====
bool checkForUpdate() {
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(versionURL);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.println("JSON parse failed!");
      return false;
    }

    int jsonDeviceID = doc["device_id"];
    String latestVersion = doc["version"];
    firmwareURL = doc["firmware"].as<String>();

    Serial.println("Device ID:       " + String(DEVICE_ID));
    Serial.println("JSON Device ID:  " + String(jsonDeviceID));
    Serial.println("Current version: " + String(FIRMWARE_VERSION));
    Serial.println("Latest version:  " + latestVersion);
    Serial.println("Firmware URL:    " + firmwareURL);

    // Check if this JSON entry matches this device
    if (jsonDeviceID != DEVICE_ID) {
      Serial.println("Device ID does not match. Skipping update.");
      return false;
    }

    // Check if new firmware is available
    if (latestVersion != FIRMWARE_VERSION) {
      return true;
    } else {
      Serial.println("Already running latest firmware.");
    }
  } else {
    Serial.println("HTTP error: " + String(httpCode));
  }

  http.end();
  return false;
}

void downloadFirmware(String url) {
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    int contentLength = http.getSize();
    if (contentLength <= 0) {
      Serial.println("Invalid content length");
      return;
    }

    bool canBegin = Update.begin(contentLength);
    if (canBegin) {
      WiFiClient& client = http.getStream();
      size_t written = Update.writeStream(client);

      if (written == contentLength && Update.end()) {
        Serial.println("Update successful! Rebooting...");
        delay(2000);
        ESP.restart();
      } else {
        Serial.println("Update failed!");
      }
    } else {
      Serial.println("Not enough space for OTA");
    }
  } else {
    Serial.println("Failed to download firmware: " + String(httpCode));
  }

  http.end();
}
