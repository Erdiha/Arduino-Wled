#include <esp_now.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "../config.h"

// Replace with the WiFi credentials and WLED IP address
const char* ssid = wifi_name;
const char* password = wifi_password;
const char* wled_ip = ip_address; // Replace with your WLED ESP32 IP address

int deviceID = 0; // Initialize with a default value
unsigned long lastMotionTime = 0;
unsigned long delayTime = 10000;
unsigned long defaultDelay = 1000;

// MAC address of the receiver board
#define SENSOR_PIN_ONE 13
#define SENSOR_PIN_TWO 27

typedef struct struct_message {
    int motionONEdetected;
    int motionTWOdetected;
} struct_message;

struct_message myData;

// Function Definitions
void initWiFi() {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(defaultDelay);
        Serial.println("Reconnecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
}

void sendToWLED(bool motionDetected) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin("http://" + String(wled_ip) + "/json/state");
        http.addHeader("Content-Type", "application/json");
        String payload = "{\"on\":" + String(motionDetected ? "true" : "false") +
                         ", \"ps\":" + String(deviceID) + ", \"bri\":128}";
        Serial.println(payload);
        int httpResponseCode = http.PUT(payload);
        if (httpResponseCode > 0) {
            Serial.println(httpResponseCode);
            Serial.println(http.getString());
        } else {
            Serial.println("Error on sending PUT");
        }
        http.end();
    } else {
        Serial.println("WiFi not connected. Cannot send to WLED.");
    }
}

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
    static bool lightOn = false;
    static unsigned long motionDetectedTime = 0;
    static bool sensorOneLastState = false;
    static bool sensorTwoLastState = false;
    
    if (WiFi.status() != WL_CONNECTED) {
        initWiFi();
    }

    bool sensorOneState = digitalRead(SENSOR_PIN_ONE) == HIGH;
    bool sensorTwoState = digitalRead(SENSOR_PIN_TWO) == HIGH;

    // Check if any sensor is triggered and light is not already on
    if ((sensorOneState || sensorTwoState) && !lightOn) {
        lightOn = true; // Turn on the light
        sendToWLED(true);
        Serial.println("Motion Detected!");
        motionDetectedTime = millis(); // Update the time of motion detection

        if (sensorOneState) {
            deviceID = 1;
            myData.motionONEdetected = 1;
        }
        if (sensorTwoState) {
            deviceID = 2;
            myData.motionTWOdetected = 1;
        }

        esp_now_send(macAddress, (uint8_t *)&myData, sizeof(myData)); // Use macADDR from config.h
        sensorOneLastState = sensorOneState;
        sensorTwoLastState = sensorTwoState;
    } else if (lightOn) {
        // If the light is on, check conditions for turning it off
        bool noNewMotion = (sensorOneState == sensorOneLastState) && (sensorTwoState == sensorTwoLastState);
        bool delayPassed = millis() - motionDetectedTime > delayTime;
        
        if (noNewMotion && delayPassed) {
            // Turn off the light if no new motion is detected after delayTime
            sendToWLED(false);
            Serial.println("Light Off.");
            lightOn = false;
            myData.motionONEdetected = 0;
            myData.motionTWOdetected = 0;
        } else if (!noNewMotion) {
            // Update the last state and motionDetectedTime if new motion is detected
            motionDetectedTime = millis();
            sensorOneLastState = sensorOneState;
            sensorTwoLastState = sensorTwoState;
            if (sensorOneState) {
                deviceID = 1;
                myData.motionONEdetected = 1;
            }
            if (sensorTwoState) {
                deviceID = 2;
                myData.motionTWOdetected = 1;
            }
            esp_now_send(macAddress, (uint8_t *)&myData, sizeof(myData));
        }
    }
    
    delay(defaultDelay); // Small delay to prevent excessive WiFi communication
}
