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
int nextRipple = 0;
int nextDirection = 0;
int nextColor = 0;
bool DelayPeriodActive = 0;
int nextNode = 0;
unsigned long lastRippleTime = 0;


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
  int rippleFired_return = 0;

  OscWiFi.parse();
  ArduinoOTA.handle();            // Handle OTA updates
  WiFi_MainFunction();

  Ripple_MainFunction(); /* advance all ripples, show all strips, fade all leds, setPixelColor all leds */

  if((benchmark-lastRippleTime) > currentDelayBetweenRipples){
    DelayPeriodActive = 0; /* Delay has passed; we may now begin a new burst*/
  }
  
  if(!DelayPeriodActive && loop_MasterFireRippleEnabled){ /* fire cyclic burst */
    lastRippleTime = millis(); /* update lastRippleTime to reset delay window counter */
    DelayPeriodActive = 1;

    if(loop_CubeFireRippleEnabled){
      rippleFired_return |= FireRipple_AllCubeNodes(&nextRipple, currentDirection, nextColor, currentBehavior, currentRippleLifeSpan, currentRippleSpeed, currentRainbowDeltaPerTick, noPreference, NO_NODE_LIMIT);
    }

    if(loop_CenterFireRippleEnabled){
      rippleFired_return |= FireRipple_CenterNode(&nextRipple, currentDirection, nextColor, currentBehavior, currentRippleLifeSpan, currentRippleSpeed, currentRainbowDeltaPerTick, noPreference, NO_NODE_LIMIT);
    }

    if(loop_QuadFireRippleEnabled){
      rippleFired_return |= FireRipple_AllQuadNodes(&nextRipple, currentDirection, nextColor, currentBehavior, currentRippleLifeSpan, currentRippleSpeed, currentRainbowDeltaPerTick, noPreference, NO_NODE_LIMIT);
    }

    if(loop_BorderFireRippleEnabled){
      rippleFired_return |= FireRipple_AllBorderNodes(&nextRipple, currentDirection, nextColor, currentBehavior, currentRippleLifeSpan, currentRippleSpeed, currentRainbowDeltaPerTick, noPreference, NO_NODE_LIMIT);
    }
    
    if (rippleFired_return){ /* ripples were fired during this window */
      nextColor++; 
      nextColor = (nextColor)%currentNumberofColors;
      rippleFired_return = 0;
    }

  }

  if(manualFireRipple){ /* fire manual burst */
    manualFireRipple = 0;
    rippleFired_return = 0;
    
    rippleFired_return |= FireRipple_CenterNode(&nextRipple, ALL_DIRECTIONS, nextColor, currentBehavior, currentRippleLifeSpan, currentRippleSpeed, currentRainbowDeltaPerTick, preferLeft, 2);
    rippleFired_return |= FireRipple_CenterNode(&nextRipple, ALL_DIRECTIONS, nextColor, currentBehavior, currentRippleLifeSpan, currentRippleSpeed, currentRainbowDeltaPerTick, preferRight, 2);
    
    
    if (rippleFired_return){ /* ripples were fired during this window */
      nextColor++; 
      nextColor = (nextColor)%currentNumberofColors;
      rippleFired_return = 0;
    }
    
  }

  
  #ifdef ENABLE_DEBUGGING
    Serial.print("Time spent executing one loop() in milliseconds: ");
    Serial.println(millis() - benchmark);
  #endif

}
