#include <esp_now.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Replace with the WiFi credentials and WLED IP address
const char* ssid = "fast_af_boi";
const char* password = "Rapunzel123Rapunzel";
const char* wled_ip = "192.168.50.192"; // Replace with your WLED ESP32 IP address

// MAC address of the receiver board
uint8_t receiverAddress[] = {0xFC, 0xB4, 0x67, 0xF6, 0x46, 0x6C};

typedef struct struct_message {
    int motionDetected;
} struct_message;

struct_message myData;

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, receiverAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }

    pinMode(13, INPUT);
}

void sendToWLED(bool motionDetected) {
    HTTPClient http;
    http.begin("http://" + String(wled_ip) + "/json/state");
    http.addHeader("Content-Type", "application/json");
    String payload = "{\"on\":";
    payload += motionDetected ? "true" : "false";
    payload += ", \"bri\":128}"; // Customize as needed

    int httpResponseCode = http.PUT(payload);
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println(httpResponseCode);
        Serial.println(response);
    } else {
        Serial.println("Error on sending PUT");
    }
    http.end();
}
void loop() {
    int motionStatus = digitalRead(13);
    if (motionStatus == HIGH) {
        myData.motionDetected = 1;
        esp_now_send(receiverAddress, (uint8_t *) &myData, sizeof(myData));
        sendToWLED(true);
        Serial.println("Motion Detected!");
        delay(300); // Wait for 10 seconds
        sendToWLED(false); // Turn off the light after 10 seconds
        Serial.println("Light Off.");
    } else {
        myData.motionDetected = 0;
        // sendToWLED(false); // This line can be commented out as the light will be turned off after 10 seconds.
        Serial.println("No Motion.");
    }
    delay(1000);
}