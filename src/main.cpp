#include <esp_now.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "../config.h" // Make sure this path is correct based on your project structure
#include <cstdint>
// Declaration of functions before use
void initWiFi();
void sendToWLED(bool motionDetected);
extern uint8_t macAddress[];
// Global Variables
const char* ssid = wifi_name;
const char* password = wifi_password;
const char* wled_ip = ip_address; 
extern uint8_t macAddress[6]; // Not strictly necessary in this case, but good practice for clarity


typedef struct struct_message {
    int motionDetected;
} struct_message;

struct_message myData;

// Function Definitions

// Function to handle WiFi connection and reconnection
void initWiFi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Reconnecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
}

// Function to control WLED based on motion detection
void sendToWLED(bool motionDetected) {
    if(WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin("http://" + String(wled_ip) + "/json/state");
        http.addHeader("Content-Type", "application/json");
        String payload = "{\"on\":";
        payload += motionDetected ? "true" : "false";
        payload += ", \"bri\":128}";

        int httpResponseCode = http.PUT(payload);
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println(httpResponseCode);
            Serial.println(response);
        } else {
            Serial.println("Error on sending PUT");
        }
        http.end();
    } else {
        Serial.println("WiFi not connected. Cannot send to WLED.");
    }
}

// Setup and Loop

void setup() {
    Serial.begin(115200);
    initWiFi(); 
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, macAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }

    pinMode(13, INPUT);
}

void loop() {
    if(WiFi.status() != WL_CONNECTED) {
        initWiFi();
    }

    int motionStatus = digitalRead(13);
    if (motionStatus == HIGH) {
        myData.motionDetected = 1;
        esp_now_send(macAddress, (uint8_t *) &myData, sizeof(myData));
        sendToWLED(true);
        Serial.println("Motion Detected!");
        delay(30000);
        sendToWLED(false);
        Serial.println("Light Off.");
    } else {
        myData.motionDetected = 0;
        Serial.println("No Motion.");
    }
    delay(1000);
}
