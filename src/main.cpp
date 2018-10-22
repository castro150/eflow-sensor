#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#define USE_SERIAL Serial

ESP8266WiFiMulti WiFiMulti;

unsigned long previousMillis = 0;
const long INTERVAL = 5000;

const byte INTERRUPT_PIN = 13; // D7 - Node MCU ESP8266
volatile float pulseCounter = 0;
volatile float liters = 0;
volatile float totalLiters = 0;
volatile float flow = 0;

// TODO remover logs

void handleInterrupt() {
    pulseCounter++;
}

void setup() {
    USE_SERIAL.begin(115200);
    pinMode(INTERRUPT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), handleInterrupt, RISING);

    for(uint8_t t = 4; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }

    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP("CASA", "abcdef1234");

    // TODO buscar litros do servidor
}

void loop() {
    unsigned long currentMillis = millis();
    
    // wait for WiFi connection
    if(currentMillis - previousMillis >= INTERVAL) {
        liters = pulseCounter / 516;
        USE_SERIAL.printf("[PULSE] count %f...\n", pulseCounter);
        pulseCounter = 0;
        flow = liters * 12;
        totalLiters += liters;
        // TODO remover logs
        USE_SERIAL.printf("[FLOW] count %f...\n", flow);
        USE_SERIAL.printf("[LITERS] count %f...\n", liters);
        USE_SERIAL.printf("[TOTAL LITERS] count %f...\n", totalLiters);
        previousMillis = currentMillis;

        if (WiFiMulti.run() == WL_CONNECTED) {
            char buff[100];
            char flowString[20];
            dtostrf(flow, 4, 6, flowString);  // 4 is mininum width, 6 is precision

            HTTPClient http;

            USE_SERIAL.print("[HTTP] begin...\n");
            // configure traged server and url
            http.begin("http://eflow-server.herokuapp.com/readings/");

            USE_SERIAL.print("[HTTP] POST...\n");
            // start connection and send HTTP request
            http.addHeader("Content-Type", "application/json");
            snprintf(buff, sizeof(buff), "{\"sensor\":\"5bc4f1ec82c009052c8d881b\",\"flow\":%s}", flowString);
            USE_SERIAL.printf("[SENDING] ");
            USE_SERIAL.printf(buff);
            USE_SERIAL.printf("\n");
            int httpCode = http.POST(buff);

            // httpCode will be negative on error
            if (httpCode > 0) {
                // HTTP header has been send and Server response header has been handled
                USE_SERIAL.printf("[HTTP] POST... code: %d\n", httpCode);

                if(httpCode == HTTP_CODE_OK) {
                    String payload = http.getString();
                    USE_SERIAL.println(payload);
                }
            } else {
                USE_SERIAL.printf("[HTTP] POST... code: %d\n", httpCode);
                USE_SERIAL.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
            }

            http.end();
        }
    }
}

