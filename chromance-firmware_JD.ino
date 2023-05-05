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


/* functions used by Application Software */
bool FireRipple_AllOddCubeNodes(int* firstRipple, int dir, int color, byte behavior, unsigned long lifespan){
  int currentRipple;
  bool rippleFired = 0;
  currentRipple = *firstRipple;
  for(int i = 0; i < numberOfCubeOddNodes; i++){
    if( dir < 0 ){ /* fire in all directions */
      rippleFired |= FireRipple(&currentRipple, 1, color, cubeOddNodes[i], behavior, lifespan);
      currentRipple = (currentRipple)%currentNumberofRipples;
      rippleFired |= FireRipple(&currentRipple, 3, color, cubeOddNodes[i], behavior, lifespan);
      currentRipple = (currentRipple)%currentNumberofRipples;
      rippleFired |= FireRipple(&currentRipple, 5, color, cubeOddNodes[i], behavior, lifespan);
      currentRipple = (currentRipple)%currentNumberofRipples;
    } else { /* fire only in one direction */
      rippleFired |= FireRipple(&currentRipple, dir, color, cubeOddNodes[i], behavior, lifespan);
      currentRipple = (currentRipple)%currentNumberofRipples;
    }
  }
  *firstRipple = currentRipple;
  return rippleFired;
}

bool FireRipple_AllPairCubeNodes(int* firstRipple, int dir, int color, byte behavior, unsigned long lifespan){
  int currentRipple;
  bool rippleFired = 0;
  currentRipple = *firstRipple;
  for(int i = 0; i < numberOfCubePairNodes; i++){
    if( dir < 0 ){ /* fire in all directions */
      rippleFired |= FireRipple(&currentRipple, 0, color, cubePairNodes[i], behavior, lifespan);
      currentRipple = (currentRipple)%currentNumberofRipples;
      rippleFired |= FireRipple(&currentRipple, 2, color, cubePairNodes[i], behavior, lifespan);
      currentRipple = (currentRipple)%currentNumberofRipples;
      rippleFired |= FireRipple(&currentRipple, 4, color, cubePairNodes[i], behavior, lifespan);
      currentRipple = (currentRipple)%currentNumberofRipples;
    } else { /* fire only in one direction */
      rippleFired |= FireRipple(&currentRipple, dir, color, cubePairNodes[i], behavior, lifespan);
      currentRipple = (currentRipple)%currentNumberofRipples;
    }
  }
  *firstRipple = currentRipple;
  return rippleFired;
}

bool FireRipple_AllCubeNodes(int* firstRipple, int dir, int color, byte behavior, unsigned long lifespan){
  int currentRipple;
  bool rippleFired = 0;
  currentRipple = *firstRipple;
  if( dir < 0 ){ /* fire in all directions */
    rippleFired = FireRipple_AllPairCubeNodes(&currentRipple, -1, color, behavior, lifespan);
    rippleFired = FireRipple_AllOddCubeNodes(&currentRipple, -1, color, behavior, lifespan);
  } else { /* fire only in one direction */
    rippleFired = FireRipple_AllPairCubeNodes(&currentRipple, dir, color, behavior, lifespan);
    rippleFired = FireRipple_AllOddCubeNodes(&currentRipple, dir, color, behavior, lifespan);
  }
  *firstRipple = currentRipple;
  return rippleFired;
}

bool FireRipple_AllQuadNodes(int* firstRipple, int dir, int color, byte behavior, unsigned long lifespan); /* to be implemented; must use nodeConnections */
bool FireRipple_AllBorderNodes(int* firstRipple, int dir, int color, byte behavior, unsigned long lifespan); /* to be implemented; must use nodeConnections */
bool FireRipple_CenterNode(int* firstRipple, int dir, int color, byte behavior, unsigned long lifespan){
  int currentRipple;
  bool rippleFired = 0;
  currentRipple = *firstRipple;
  if( dir < 0 ){ /* fire in all directions */
    rippleFired |= FireRipple(&currentRipple, 0, color, starburstNode, behavior, lifespan);
    currentRipple = (currentRipple)%currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 1, color, starburstNode, behavior, lifespan);
    currentRipple = (currentRipple)%currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 2, color, starburstNode, behavior, lifespan);
    currentRipple = (currentRipple)%currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 3, color, starburstNode, behavior, lifespan);
    currentRipple = (currentRipple)%currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 4, color, starburstNode, behavior, lifespan);
    currentRipple = (currentRipple)%currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 5, color, starburstNode, behavior, lifespan);
    currentRipple = (currentRipple)%currentNumberofRipples;
  } else { /* fire only in one direction */
    rippleFired |= FireRipple(&currentRipple, dir, color, starburstNode, behavior, lifespan);
    currentRipple = (currentRipple)%currentNumberofRipples;
  }
  *firstRipple = currentRipple;
  return rippleFired;
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
  
  if(!DelayPeriodActive && loopFireRippleEnabled){ /* fire cyclic burst */
    lastRippleTime = millis(); /* update lastRippleTime to reset delay window counter */
    DelayPeriodActive = 1;
    /* void rippleFired |= FireRipple(int ripple, int dir, int col, int node, byte behavior, unsigned long lifespan) */
    /* int FireRipple_AllCubeNodes(int firstRipple, int dir, int color, byte behavior, unsigned long lifespan) */
    rippleFired_return |= FireRipple_AllCubeNodes(&nextRipple, ALL_DIRECTIONS, nextColor, alwaysTurnsRight, currentRippleLifeSpan);
    rippleFired_return |= FireRipple_CenterNode(&nextRipple, ALL_DIRECTIONS, nextColor, alwaysTurnsRight, currentRippleLifeSpan);
    
    if (rippleFired_return){ /* ripples were fired during this window */
      nextColor++; 
      nextColor = (nextColor)%currentNumberofColors;
      rippleFired_return = 0;
    }

  }

  if(manualFireRipple){ /* fire manual burst */
    manualFireRipple = 0;
    rippleFired_return = 0;
    
    rippleFired_return |= FireRipple_AllCubeNodes(&nextRipple, ALL_DIRECTIONS, nextColor, alwaysTurnsRight, currentRippleLifeSpan);
    rippleFired_return |= FireRipple_CenterNode(&nextRipple, ALL_DIRECTIONS, nextColor, alwaysTurnsRight, currentRippleLifeSpan);
    
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
