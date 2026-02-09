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

#define NUMBER_OF_DIRECTIONS 6
#define NUMBER_OF_STARTING_NODES 3
#define LED 2 /*onboard LED*/

/* Global Variables used by application */
int nextRipple = 0;


/*Alexa callback*/
void handle_SetState(unsigned char id, bool state, unsigned char bri, short ct, unsigned int hue, unsigned char sat, char mode)
{
  Serial.printf_P("\nhandle_SetState id: %d, state: %s, bri: %d, ct: %d, hue: %d, sat: %d, mode: %s\n",
                  id, state ? "true" : "false", bri, ct, hue, sat, mode == 'h' ? "hs" : mode == 'c' ? "ct"
                                                                                                    : "xy");

  if (state == 1)
  {
    digitalWrite(LED, HIGH);
    Serial.println("Alexa set ON");
  }
  else
  {
    digitalWrite(LED, LOW);
    Serial.println("Alexa set OFF");
  }
}

void setup()
{
  Serial.begin(115200);

  pinMode(LED, OUTPUT);
  WiFi_Utilities_init();
  Strips_init();
  
  (void) EEPROM_Init();

  udp_printf("GlobalParameters restored from EEPROM:");
  udp_printf("MasterFireRippleEnabled: %d", GlobalParameters.MasterFireRippleEnabled);
  udp_printf("NumberOfActiveProfiles: %d", GlobalParameters.NumberOfActiveProfiles);
  for(int i = 0; i < GlobalParameters.NumberOfActiveProfiles; i++){
    udp_printf(" -Profile %d Name: \"%s\"", i, GlobalParameters.RippleProfiles[i].ProfileName);
    // udp_printf("Profile %d Active: %d", i, GlobalParameters.RippleProfiles[i].Active);
    // udp_printf("Profile %d DelayBetweenRipples_ms: %d", i, GlobalParameters.RippleProfiles[i].DelayBetweenRipples_ms);
    // udp_printf("Profile %d RippleLifeSpan: %d", i, GlobalParameters.RippleProfiles[i].RippleLifeSpan);
    // udp_printf("Profile %d RippleSpeed: %d", i, GlobalParameters.RippleProfiles[i].RippleSpeed);
    // udp_printf("Profile %d NumberOfColors: %d", i, GlobalParameters.RippleProfiles[i].NumberOfColors);
    // udp_printf("Profile %d CurrentColor: %d", i, GlobalParameters.RippleProfiles[i].CurrentColor);
    // udp_printf("Profile %d Behavior: %d", i, GlobalParameters.RippleProfiles[i].Behavior);
    // udp_printf("Profile %d RainbowDeltaPerTick: %d", i, GlobalParameters.RippleProfiles[i].RainbowDeltaPerTick);
  }  
}
  
void loop()
{
  unsigned long currentTime_ms = millis();
  int rippleFired_return = 0;

  WiFi_Utilities_loop();

  if (!OTAinProgress)
  {
    Ripple_MainFunction(); /* advance all ripples, show all strips, fade all leds, setPixelColor all leds */

    /* decide if we need to fire a new ripple */
    if(GlobalParameters.MasterFireRippleEnabled){
      for(int i = 0; i < NUMBER_OF_PROFILES; i++){ //iterate through all profiles
        if(!GlobalParameters.RippleProfiles[i].Active) continue; //skip inactive profiles
        if((currentTime_ms - GlobalParameters.RippleProfiles[i].TimeLastRippleFired_ms) > GlobalParameters.RippleProfiles[i].DelayBetweenRipples_ms){
          /* delay has passed; we may now begin a new burst*/
          GlobalParameters.RippleProfiles[i].TimeLastRippleFired_ms = millis(); //update lastRippleTime to reset delay window counter
          for(int j = 0; j < NUMBER_OF_NODES; j++){ //iterate through all nodes
            if(!GlobalParameters.RippleProfiles[i].ActiveNodes[j]) continue; //skip inactive nodes
            
            int dirStart = 0;
            int dirEnd = NUMBER_OF_DIRECTIONS;
            if(GlobalParameters.RippleProfiles[i].Direction >= 0 && GlobalParameters.RippleProfiles[i].Direction < NUMBER_OF_DIRECTIONS){
              dirStart = GlobalParameters.RippleProfiles[i].Direction;
              dirEnd = dirStart + 1;
            }
            for(int direction = dirStart; direction < dirEnd; direction++){

              rippleFired_return |= FireRipple(&nextRipple,
                direction,
                GlobalParameters.RippleProfiles[i].Colors[GlobalParameters.RippleProfiles[i].CurrentColor],
                j, /* node */
                GlobalParameters.RippleProfiles[i].Behavior,
                GlobalParameters.RippleProfiles[i].RippleLifeSpan,
                GlobalParameters.RippleProfiles[i].RippleSpeed,
                GlobalParameters.RippleProfiles[i].RainbowDeltaPerTick,
                noPreference,
                NO_NODE_LIMIT);
              } /* end direction loop */
          } /* end node loop */

          if(rippleFired_return){
            GlobalParameters.RippleProfiles[i].CurrentColor++; //ripple was fired, advance to profile's next color
            if(GlobalParameters.RippleProfiles[i].CurrentColor >= GlobalParameters.RippleProfiles[i].NumberOfColors) GlobalParameters.RippleProfiles[i].CurrentColor = 0;
            rippleFired_return = 0; //reset rippleFired_return for next profile
          }
        } /* end delay check */
      } /* end profile loop */
    } /* end MasterFireRippleEnabled check */
  } /* end OTAinProgress check */
}
