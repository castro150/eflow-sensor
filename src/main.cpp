/**
 * BasicHTTPClient.ino
 *
 *  Created on: 24.05.2015
 *
 */

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#define USE_SERIAL Serial

ESP8266WiFiMulti WiFiMulti;

unsigned long previousMillis = 0;
const long INTERVAL = 5000;

const byte INTERRUPT_PIN = 13; // D7 - Node MCU ESP8266
volatile uint32_t pulseCounter = 0;
volatile uint64_t liters = 0;

void handleInterrupt() {
    pulseCounter++;
    // TODO remover log
    USE_SERIAL.printf("[INTERRUPT] count %d...\n", pulseCounter);
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
    if((WiFiMulti.run() == WL_CONNECTED) && (currentMillis - previousMillis >= INTERVAL)) {
        // TODO obter litros do contador de pulso, zerá-lo e somar aos litros atuais
        // TODO calcular vazão
        // TODO enviar tudo para servidor

        previousMillis = currentMillis;
        HTTPClient http;

        USE_SERIAL.print("[HTTP] begin...\n");
        // configure traged server and url
        http.begin("http://eflow-server.herokuapp.com/users/");

        USE_SERIAL.print("[HTTP] POST...\n");
        // start connection and send HTTP request
        http.addHeader("Content-Type", "application/json");
        int httpCode = http.POST("{ \"param1\": 1, \"param2\": \"data string\" }");

        // httpCode will be negative on error
        if(httpCode > 0) {
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

