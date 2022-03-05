/*
   Chromance wall hexagon source (emotion controlled w/ EmotiBit)
   Partially cribbed from the DotStar example
   I smooshed in the ESP32 BasicOTA sketch, too

   (C) Voidstar Lab 2021
*/

#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <ArduinoOSCWiFi.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "mapping.h"
#include "ripple.h"
//#include "HTTP_Server.h"


const char* ssid = "TP-Link-150";
const char* password = "Cenote#150";

// WiFi stuff - CHANGE FOR YOUR OWN NETWORK!
const IPAddress ip(192, 168, 0, 241);  // IP address that THIS DEVICE should request
const IPAddress gateway(192, 168, 0, 1);  // Your router
const IPAddress subnet(255, 255, 255, 0);  // Your subnet mask (find it from your router's admin panel)

//WiFiServer server(80); //Open port number 80 (HTTP)

//strip(NUMLEDS, DATAPIN, CLOCKPIN, DOTSTART_BRG)
Adafruit_NeoPixel strip0(11, 15,  NEO_GRB + NEO_KHZ800);
extern byte ledColors[1][11][3];

float decay = 0.998;  // Multiply all LED's by this amount each tick to create fancy fading tails

// These ripples are endlessly reused so we don't need to do any memory management
#define numberOfRipples 1
Ripple ripples[numberOfRipples] = {
  Ripple(0),
};


void setup() {
  Serial.begin(115200);

  Serial.println("*** LET'S GOOOOO ***");

  strip0.begin();
  strip0.setBrightness(125);  // If your PSU sucks, use this to limit the current
  strip0.show();


  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  Serial.print("WiFi connected! IP = ");
  Serial.println(WiFi.localIP());
  
  //server.begin();

  // Wireless OTA updating? On an ARDUINO?! It's more likely than you think!
  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  })
  .onEnd([]() {
    Serial.println("\nEnd");
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();

  Serial.println("Ready for WiFi OTA updates");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
}

void FireRipple(){
  int hue = fmap(random(100), 0, 99, 0, 0xFFFF);
  ripples[0].start(
    0, //starting node
    4, //direction
    strip0.ColorHSV(hue, 255, 255),
    float(random(100)) / 100.0 * .2 + .8, //speed
    1000, //lifespan
    2 //behavior
  );
}

int loopCounter = 0;

void loop(){
  if(loopCounter == 270){
    loopCounter = 0;
    if(ripples[0].state == dead){
      FireRipple();
    }
  }
  loopCounter++;
  
  OscWiFi.parse();
  ArduinoOTA.handle();						// Handle OTA updates

  /*
  WiFiClient client = server.available();   // Listen for incoming clients
  if (client) {                             // If a new client connects,
    Serial.println("New Client. Handling HTTP request");
    //HandleHTTPRequest(client);
  }
  */
  // Fade all dots to create trails
  for (int led = 0; led < 11; led++) {
    for (int i = 0; i < 3; i++) {
        ledColors[0][led][i] *= decay;
    }
  }
  
  //SetPixelColor all leds to ledColors
  for (int led = 0; led < 11; led++) {
    strip0.setPixelColor(led, ledColors[0][led][0], ledColors[0][led][1], ledColors[0][led][2]);
  }

  for (int i = 0; i < numberOfRipples; i++) {
    ripples[i].advance(ledColors);
  }

  delay(10);
  strip0.show();

}
