#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define USE_SERIAL Serial

ESP8266WiFiMulti WiFiMulti;

unsigned long previousMillis = 0;
const long INTERVAL = 5000;

const byte INTERRUPT_PIN = 13; // D7 - Node MCU ESP8266
volatile float pulseCounter = 0;
volatile float liters = 0;
volatile float totalLiters = 0;
volatile float flow = 0;

void handleInterrupt() {
    pulseCounter++;
}

void updateActualConsumption() {
    if (WiFiMulti.run() == WL_CONNECTED) {
        HTTPClient http;

        // configure traged server and url
        http.begin("http://eflow-server.herokuapp.com/readings/actual/");

        USE_SERIAL.print("[HTTP] GET consumption...\n");
        // start connection and send HTTP request
        int httpCode = http.GET();

        // httpCode will be negative on error
        if (httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
            USE_SERIAL.printf("[HTTP] GET code: %d\n", httpCode);

            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                StaticJsonBuffer<300> JSONBuffer;
                JsonObject& parsed = JSONBuffer.parseObject(payload);
                totalLiters = parsed["consumption"];
                USE_SERIAL.println(payload);
            }
        } else {
            USE_SERIAL.printf("[HTTP] GET code: %d, error: %s\n", httpCode, http.errorToString(httpCode).c_str());
        }

        http.end();
    }
}

void setup() {
    USE_SERIAL.begin(115200);
    pinMode(INTERRUPT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), handleInterrupt, RISING);

    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP("CASA", "abcdef1234");
    while (WiFiMulti.run() != WL_CONNECTED) {
        USE_SERIAL.print(".");
        delay(1000);
    }
    USE_SERIAL.print("\n");

    updateActualConsumption();
}

void loop() {
    unsigned long currentMillis = millis();
    
    // wait for WiFi connection
    if(currentMillis - previousMillis >= INTERVAL) {
        liters = pulseCounter / 516;
        pulseCounter = 0;
        flow = liters * 12;
        totalLiters += liters;
        USE_SERIAL.printf("[FLOW] %f / [LITERS] %f / [TOTAL LITERS] %f\n", flow, liters, totalLiters);
        previousMillis = currentMillis;

        if (WiFiMulti.run() == WL_CONNECTED) {
            char buff[100];
            char consumptionString[20];
            char flowString[20];
            dtostrf(totalLiters, 4, 6, consumptionString);  // 4 is mininum width, 6 is precision
            dtostrf(flow, 4, 6, flowString);  // 4 is mininum width, 6 is precision

            HTTPClient http;

            // configure traged server and url
            http.begin("http://eflow-server.herokuapp.com/readings/");

            USE_SERIAL.print("[HTTP] POST new reading...\n");
            // start connection and send HTTP request
            http.addHeader("Content-Type", "application/json");
            snprintf(buff, sizeof(buff), "{\"sensor\":\"5bc4f1ec82c009052c8d881b\",\"consumption\":%s,\"flow\":%s}", consumptionString, flowString);
            USE_SERIAL.printf("[SENDING] %s\n", buff);
            int httpCode = http.POST(buff);

            // httpCode will be negative on error
            if (httpCode > 0) {
                // HTTP header has been send and Server response header has been handled
                USE_SERIAL.printf("[HTTP] POST code: %d\n", httpCode);

                if(httpCode == HTTP_CODE_OK) {
                    String payload = http.getString();
                    USE_SERIAL.println(payload);
                }
            } else {
                USE_SERIAL.printf("[HTTP] POST code: %d, error: %s\n", httpCode, http.errorToString(httpCode).c_str());
            }

            http.end();
        }
    }
}

