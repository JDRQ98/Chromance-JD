/*
   Chromance wall hexagon source
   Partially cribbed from the DotStar example
   I smooshed in the ESP32 BasicOTA sketch, too

   (C) Voidstar Lab 2021
*/

#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <ArduinoOTA.h>
#include <ArduinoOSCWiFi.h>

#include "ripple.h"
#include "HTTP_Server.h"
#include "ASW.h"
#include "EEP.h"
#include "SimpleJson.h"
#include "HueBridge.h"

#define NUMBER_OF_DIRECTIONS 3
#define NUMBER_OF_STARTING_NODES 3

/* Global Variables used by application */
int nextRipple = 0;
int nextDirection = 0;
int nextColor = 0;
bool DelayPeriodActive = 0;
int nextNode = 0;
unsigned long lastRippleTime = 0;

HueBridge hueBridge;

void handle_SetState(unsigned char id, bool state, unsigned char bri, short ct, unsigned int hue, unsigned char sat, char mode)
{
  Serial.printf_P("\nhandle_SetState id: %d, state: %s, bri: %d, ct: %d, hue: %d, sat: %d, mode: %s\n", 
    id, state ? "true": "false", bri, ct, hue, sat, mode == 'h' ? "hs" : mode == 'c' ? "ct" : "xy");

  //hueBridge.setState(id, state, bri, ct, hue, sat, mode); // set internal variables of the hueBridge class
  GlobalParameters.loop_MasterFireRippleEnabled = state;

  if ( ct == 383 ){
    Serial.println("Warm white");
  }

  if ( hue == 0 && sat == 254 ){
    Serial.println("Red");
  }

  if ( hue == 21845 && sat == 254 ){
    Serial.println("Green");
  }

  if ( hue == 43690 && sat == 254 ){
    Serial.println("Blue");
  }
}



void setup() {
  Serial.begin(115200);

  EEPROM.begin(EEPROM_SIZE);
  Global_NumberOfProfiles_InDFLS = EEPROM_ParseProfiles();
  Strips_init();
  WiFi_init();

 
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


  // setup device name for Amazon Echo
  hueBridge.addDevice("hexagono");
  hueBridge.onSetState(handle_SetState);
  hueBridge.start();


  //EEPROM_Read_GlobalParameters();
}


void loop(){
  unsigned long benchmark = millis();
  int rippleFired_return = 0;

  OscWiFi.parse();
  ArduinoOTA.handle();            // Handle OTA updates
  //WiFi_MainFunction();
  hueBridge.handle();

  Ripple_MainFunction(); /* advance all ripples, show all strips, fade all leds, setPixelColor all leds */

  if((benchmark-lastRippleTime) > GlobalParameters.currentDelayBetweenRipples){
    DelayPeriodActive = 0; /* Delay has passed; we may now begin a new burst*/
  }
  
  if(!DelayPeriodActive && GlobalParameters.loop_MasterFireRippleEnabled){ /* fire cyclic burst */
    lastRippleTime = millis(); /* update lastRippleTime to reset delay window counter */
    DelayPeriodActive = 1;
      rippleFired_return |= FireRipple_CenterNode(&nextRipple, GlobalParameters.currentDirection, nextColor, GlobalParameters.currentBehavior, GlobalParameters.currentRippleLifeSpan, GlobalParameters.currentRippleSpeed, GlobalParameters.currentRainbowDeltaPerTick, noPreference, NO_NODE_LIMIT);
  }


  if(manualFireRipple){ /* fire manual burst */
    manualFireRipple = 0;
    rippleFired_return = 0;
    
    rippleFired_return |= FireEffect_Random(&nextRipple, nextColor, GlobalParameters.currentBehavior, GlobalParameters.currentRippleLifeSpan, GlobalParameters.currentRippleSpeed, GlobalParameters.currentRainbowDeltaPerTick, NO_NODE_LIMIT);
    
  }

  if (rippleFired_return)
  { /* ripples were fired during this window */
    nextColor++;
    nextColor = (nextColor) % GlobalParameters.currentNumberofColors;
    rippleFired_return = 0;
  }
}
