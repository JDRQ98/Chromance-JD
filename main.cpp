/*
   Chromance wall hexagon source
   Partially cribbed from the DotStar example
   I smooshed in the ESP32 BasicOTA sketch, too

   (C) Voidstar Lab 2021
*/

#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include "Arduino.h"

#include "MCAL/ripple.h"
#include "HTTP_Server.h"
#include "ASW.h"
#include "MCAL/EEP.h"
#include "Wifi_utilities.h"

#define NUMBER_OF_DIRECTIONS 3
#define NUMBER_OF_STARTING_NODES 3
#define LED 2 /*onboard LED*/

/* Global Variables used by application */
int nextRipple = 0;
int nextDirection = 0;
int nextColor = 0;
bool DelayPeriodActive = 0;
int nextNode = 0;
unsigned long lastRippleTime = 0;

/*Alexa callback*/
void handle_SetState(unsigned char id, bool state, unsigned char bri, short ct, unsigned int hue, unsigned char sat, char mode)
{
  Serial.printf_P("\nhandle_SetState id: %d, state: %s, bri: %d, ct: %d, hue: %d, sat: %d, mode: %s\n", 
    id, state ? "true": "false", bri, ct, hue, sat, mode == 'h' ? "hs" : mode == 'c' ? "ct" : "xy");

  if ( state == 1 ){
    digitalWrite(LED,HIGH);
    Serial.println("Alexa set ON");
  }else{
    digitalWrite(LED,LOW);
    Serial.println("Alexa set OFF");
  }
}

void setup() {
  Serial.begin(115200);

  //EEPROM.begin(EEPROM_SIZE);
  //Global_NumberOfProfiles_InDFLS = EEPROM_ParseProfiles();
  //EEPROM_Read_GlobalParameters();

  pinMode(LED, OUTPUT);
  Strips_init();
  WiFi_Utilities_init();
  udp_println("Chromance device is ONLINE!");

}


void loop(){
  unsigned long benchmark = millis();
  int rippleFired_return = 0;

  WiFi_Utilities_loop();
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
