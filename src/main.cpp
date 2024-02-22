#include <esp_now.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "../config.h"

// Replace with the WiFi credentials and WLED IP address
const char* ssid = wifi_name;
const char* password = wifi_password;
const char* wled_ip = ip_address; // Replace with your WLED ESP32 IP address
int deviceID;
// MAC address of the receiver board
#define SENSOR_PIN_ONE 13
#define SENSOR_PIN_TWO 27

typedef struct struct_message {
    int motionONEdetected;
    int motionTWOdetected;
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
        payload += ", \"ps\":"; // Corrected the syntax here
        payload += deviceID; // Appending the deviceID directly
        payload += ", \"bri\":128}"; // Brightness
        Serial.println(payload); // Printing the payload to the serial monitor for debugging purposes
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

    pinMode(SENSOR_PIN_ONE, INPUT);
    pinMode(SENSOR_PIN_TWO, INPUT);
}

void loop() {
    if(WiFi.status() != WL_CONNECTED) {
        initWiFi();
    }

    int sensorONE = digitalRead(SENSOR_PIN_ONE);
    int sensorTWO = digitalRead(SENSOR_PIN_TWO);
    
    if (sensorONE == HIGH) {
        myData.motionONEdetected = 1;
        deviceID = 1;
        esp_now_send(macAddress, (uint8_t *) &myData, sizeof(myData));
        sendToWLED(true);
        Serial.println("Motion one Detected!");
        delay(3000);
        sendToWLED(false);
        Serial.println("Light Off.");
    }
    else if (sensorTWO == HIGH){
        myData.motionTWOdetected = 1;
        deviceID = 2;
        esp_now_send(macAddress, (uint8_t *) &myData, sizeof(myData));
        sendToWLED(true);
        Serial.println("Motion two Detected!");
        delay(3000);
        sendToWLED(false);
        Serial.println("Light Off.");
    }
    else {
        myData.motionONEdetected = 0;
        myData.motionTWOdetected = 0;
        Serial.println("No Motion.");
    }
    delay(1000);
}