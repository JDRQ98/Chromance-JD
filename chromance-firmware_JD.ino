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
#include "HTTP_Server.h"

#define NUMBER_OF_DIRECTIONS 3

int directions[NUMBER_OF_DIRECTIONS] = {3,5,1};
extern int loopFireRippleEnabled;
extern int manualFireRipple;


// WiFi stuff - CHANGE FOR YOUR OWN NETWORK!
const char* ssid = "TP-Link-150";
const char* password = "Cenote#150";

const IPAddress ip(192, 168, 0, 241);  // IP address that THIS DEVICE should request
const IPAddress gateway(192, 168, 0, 1);  // Your router
const IPAddress subnet(255, 255, 255, 0);  // Your subnet mask (find it from your router's admin panel)

WiFiServer server(80); //Open port number 80 (HTTP)



int lengths[NUMBER_OF_STRIPS] = {99}; 

//strip(NUMLEDS, DATAPIN, CLOCKPIN, DOTSTART_BRG)
Adafruit_NeoPixel strip0(lengths[0], 15,  NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strips[NUMBER_OF_STRIPS] = {strip0};



float decay = 0.985;  // Multiply all LED's by this amount each tick to create fancy fading tails 0.972 good value for rainbow

// These ripples are endlessly reused so we don't need to do any memory management
#define NUMBER_OF_RIPPLES 9
Ripple ripples[NUMBER_OF_RIPPLES] = {
  Ripple(0),
  Ripple(1),
  Ripple(2),
  Ripple(3),
  Ripple(4),
  Ripple(5),
  Ripple(6),
  Ripple(7),
  Ripple(8)
};


void setup() {
  Serial.begin(115200);

  Serial.println("*** LET'S GOOOOO ***");

  for (int i = 0; i < NUMBER_OF_STRIPS; i++) {
    strips[i].begin();
    //    strips[i].setBrightness(125);  // If your PSU sucks, use this to limit the current
    strips[i].show();
  }


  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(50000);
    ESP.restart();
  }

  Serial.print("WiFi connected! IP = ");
  Serial.println(WiFi.localIP());
  
  server.begin();

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

void FireRipple(int ripple, int dir, int col){
  //int hue = fmap(random(100), 0, 99, 0, 0xFFFF);
  int hue = fmap(col, 0, 7, 0, 0xFFFF);
  ripples[ripple].start(
    3, //starting node
    dir, //direction
    strip0.ColorHSV(hue, 255, 255),
    //float(random(100)) / 100.0 * .2 + .8, //speed
    0.15, //speed
    4000, //lifespan
    0, //behavior, 3 = always turn right
    hue
  );
}

int nextRipple = 0;
int nextDirection = 0;
int nextColor = 0;
int rippleFired = 0;
unsigned long lastRippleTime = 0;

void loop(){
  unsigned long benchmark = millis();

  if((benchmark-lastRippleTime) > 500){
    rippleFired = 0;
  }
  
  if(!rippleFired){
    if(ripples[nextRipple].state == dead && loopFireRippleEnabled){
      Serial.print("Firing ripple ");
      Serial.print(nextRipple);
      Serial.print(" in direction ");
      Serial.println(directions[nextDirection]);
      FireRipple(nextRipple++, directions[nextDirection++], nextColor++);
      rippleFired = 1;
      lastRippleTime = millis();
      nextRipple = nextRipple%NUMBER_OF_RIPPLES;
      nextDirection = nextDirection%NUMBER_OF_DIRECTIONS;
      nextColor = (nextColor)%7;
      Serial.print("Next ripple ");
      Serial.print(nextRipple);
      Serial.print(", next direction ");
      Serial.println(directions[nextDirection]);
    }
  }

  if(manualFireRipple && ripples[nextRipple].state == dead){
    manualFireRipple = 0;
    FireRipple(nextRipple++, directions[nextDirection++], nextColor++);
    nextRipple = (nextRipple)%NUMBER_OF_RIPPLES;
    nextDirection = (nextDirection)%NUMBER_OF_DIRECTIONS;
    nextColor = (nextColor)%7;
  }
  
  OscWiFi.parse();
  ArduinoOTA.handle();						// Handle OTA updates

  
  WiFiClient client = server.available();   // Listen for incoming clients
  if (client) {                             // If a new client connects,
    Serial.println("New Client. Handling HTTP request");
    HandleHTTPRequest(client);
  }
  
  // Fade all dots to create trails
  
  for (int segment = 0; segment < NUMBER_OF_SEGMENTS ; segment++){
    for (int led = 0; led < 11; led++) {
       ledHues[segment][led][1] *= decay; //fade brightness
      /*for (int i = 0; i < 3; i++) {
          ledColors[strip][led][i] *= decay;
      }*/
    }
  }
/*
  for (int rip = 0; rip < NUMBER_OF_RIPPLES ; rip++){
    ripples[rip].hue += 50; //rainbow effect
  }*/
  
  //SetPixelColor all leds to ledColors
  for (int segment = 0; segment < NUMBER_OF_SEGMENTS ; segment++){
    for (int fromBottom = 0; fromBottom < 11; fromBottom++) {
      int strip = ledAssignments[segment][0];
      int led = round(fmap(
                        fromBottom,
                        0, 10,
                        ledAssignments[segment][2], ledAssignments[segment][1]));
      /*
      Serial.println("--TESTING LEDCOLORS ASSIGNMENT--");
      Serial.print("strip: ");
      Serial.println(strip);
      Serial.print("segment: ");
      Serial.println(segment);
      Serial.print("fromBottom: ");
      Serial.println(fromBottom);
      Serial.print("ledAssignments[segment][2]: ");
      Serial.println(ledAssignments[segment][1]);
      Serial.print("ledAssignments[segment][1]: ");
      Serial.println(ledAssignments[segment][1]);
      Serial.print("led: ");
      Serial.println(led);
      */
      //strips[strip].setPixelColor(led, ledColors[segment][fromBottom][0], ledColors[segment][fromBottom][1], ledColors[segment][fromBottom][2]);
      unsigned long color = strips[strip].ColorHSV(ledHues[segment][fromBottom][0], 255, ledHues[segment][fromBottom][1]);
      strips[strip].setPixelColor(led, color);
    }
  }

  for (int i = 0; i < NUMBER_OF_RIPPLES; i++) {
    ripples[i].advance(ledHues);
  }

  //delay(10);
  for (int strip = 0; strip < NUMBER_OF_STRIPS ; strip++){
    strips[strip].show();
  }
  //Serial.print("Time between strip.show(): ");
  //Serial.println(millis() - benchmark);

}
