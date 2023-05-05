/*
   Chromance wall hexagon source (emotion controlled w/ EmotiBit)
   Partially cribbed from the DotStar example
   I smooshed in the ESP32 BasicOTA sketch, too

   (C) Voidstar Lab 2021
*/

#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include <ArduinoOSCWiFi.h>

#include "ripple.h"
#include "HTTP_Server.h"

//#define ENABLE_DEBUGGING
#define NUMBER_OF_DIRECTIONS 3
#define NUMBER_OF_STARTING_NODES 3

/* Global Variables used by application */
int directions[NUMBER_OF_DIRECTIONS] = {1,3,5};
int starting_nodes[NUMBER_OF_STARTING_NODES] = {4, 11, 12};
int nextRipple = 0;
int nextDirection = 0;
int nextColor = 0;
int rippleFired_withinDelayWindow = 0;
int nextNode = 0;
unsigned long lastRippleTime = 0;

/* control variables */
extern int loopFireRippleEnabled;
extern int manualFireRipple;

/* parameters coming from HTTP_Server */
extern int currentNumberofRipples;
extern int currentNumberofColors;
extern short currentDelayBetweenRipples;
extern unsigned long currentRippleLifeSpan;
extern float currentDecay;


void setup() {
  Serial.begin(115200);

  WiFi_init();
  Strips_init();
 
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
}


void loop(){
  unsigned long benchmark = millis();

  OscWiFi.parse();
  ArduinoOTA.handle();            // Handle OTA updates
  WiFi_MainFunction();

  Ripple_MainFunction(); /* advance all ripples, show all strips, fade all leds, setPixelColor all leds */

  if((benchmark-lastRippleTime) > currentDelayBetweenRipples){
    rippleFired_withinDelayWindow = 0; /* Delay has passed; we may now begin a new burst*/
  }
  
  if(!rippleFired_withinDelayWindow && loopFireRippleEnabled){ /* fire cyclic burst */
    lastRippleTime = millis(); /* update lastRippleTime to reset delay window counter */
    rippleFired_withinDelayWindow = 1;
    /* void FireRipple(int ripple, int dir, int col, int node, byte behavior, unsigned long lifespan) */
    for (int dir = 0; dir < NUMBER_OF_DIRECTIONS; dir++){
      for (int node = 0; node < NUMBER_OF_STARTING_NODES; node++){  
        FireRipple(nextRipple++, directions[dir], nextColor, starting_nodes[node], feisty, currentRippleLifeSpan);
        if (nextRipple >= currentNumberofRipples) nextRipple = 0;
      }
    }
    nextColor++;
    nextColor = (nextColor)%currentNumberofColors;
  }

  if(manualFireRipple){ /* fire manual burst */
    manualFireRipple = 0;

    for (int dir = 0; dir < NUMBER_OF_DIRECTIONS; dir++){
      for (int node = 0; node < NUMBER_OF_STARTING_NODES; node++){  
        FireRipple(nextRipple++, directions[dir], nextColor, starting_nodes[node], alwaysTurnsRight, currentRippleLifeSpan);
        if (nextRipple >= currentNumberofRipples) nextRipple = 0;
      }
    }
    
    nextColor++;
    nextColor = (nextColor)%currentNumberofColors;
    
  }

  
  #ifdef ENABLE_DEBUGGING
    Serial.print("Time spent executing one loop() in milliseconds: ");
    Serial.println(millis() - benchmark);
  #endif

}
