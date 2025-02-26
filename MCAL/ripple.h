/*
   A dot animation that travels along rails
   (C) Voidstar Lab LLC 2021
*/

#ifndef RIPPLE_H_
#define RIPPLE_H_

// WARNING: These slow things down enough to affect performance. Don't turn on unless you need them!
//#define DEBUG_ADVANCEMENT  // Print debug messages about ripples' movement
//#define DEBUG_RENDERING  // Print debug messages about translating logical to actual position
//#define DEBUG_PRESSURE

#include <Adafruit_NeoPixel.h>
#include "mapping.h"

#define MAX_NUMBER_OF_RIPPLES 100 /* for memory management: max number of ripples */

#define ALL_DIRECTIONS -1 /* used as paramater 'dir' for Bulk FireRipple API's */
#define NO_NODE_LIMIT 0xFFFF /* used as parameter 'nodeLimit' for FireRipple API's*/

enum rippleState {
  dead,
  withinNode,  // Ripple isn't drawn as it passes through a node to keep the speed consistent
  travelingUpwards,
  travelingDownwards
};

enum rippleBehavior {
  weaksauce = 0,
  feisty = 1,
  angry = 2,
  alwaysTurnsRight = 3,
  alwaysTurnsLeft = 4
};

enum directionBias {
  noPreference = 0,
  preferLeft = 1,
  preferLeftOnce = 2,
  preferLeftTwice = 3,
  preferRight = 4,
  preferRightOnce = 5,
  preferRightTwice = 6
};


/* public functions */
float fmap(float x, float in_min, float in_max, float out_min, float out_max);
void SetPixelColor_AllLEDs();
void Strips_init();
void Ripple_MainFunction();
void Ripple_KillAllRipples();

/* functions used by Application Software */
extern bool FireRipple(int* ripple, int dir, int col, int node, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, directionBias bias, unsigned short nodeLimit);
extern bool FireDoubleRipple(int* firstRipple, int dir, int color, int node, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, unsigned short nodeLimit);
extern bool FireShard(int *firstRipple, int dir, int color, int node, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, unsigned short nodeLimit);
extern void setSegmentColor(int segment, int col);

/* extern the strips for finer control if neede */
extern Adafruit_NeoPixel strips[NUMBER_OF_STRIPS];

class Ripple {
  public:
    Ripple(int id) : rippleId(id) {
      Serial.print("Instanced ripple #");
      Serial.println(rippleId);
    }

    rippleState state = dead;
    unsigned long color;
    unsigned short hue;
    unsigned short hueDeltaPerTick; /* hue is incremented by this amount every time advance is called; set to 0 for solid color */
    unsigned short nodeLimit = NO_NODE_LIMIT; /* hard-limit on the number of nodes to be reached by ripple */
    directionBias rippleBias = noPreference; /* only relevant for ripples with behavior != always right or always left */
    int currentLed = 0;

    /*
       If within a node: 0 is node, 1 is direction
       If traveling, 0 is segment, 1 is LED position from bottom
    */
    int position[2];

    // Place the Ripple in a node
    void start(byte n, byte d, unsigned long c, float s, unsigned long l, byte b, unsigned short h, unsigned short hDelta, directionBias bias, unsigned short nLimit) {
      position[0] = n; /* starting node */
      position[1] = d; /* direction */
      color = c;
      hue = h;
      speed = s;
      lifespan = l;
      behavior = b;
      hueDeltaPerTick = hDelta;
      rippleBias = bias;
      nodeLimit = nLimit;

      birthday = millis();
      pressure = 0;
      state = withinNode;

      

      justStarted = true;

#ifdef DEBUG_ADVANCEMENT
      Serial.print("Ripple ");
      Serial.print(rippleId);
      Serial.print(" starting at node ");
      Serial.print(position[0]);
      Serial.print(", direction ");
      Serial.print(position[1]);
      Serial.print(", color (RGB) 0x");
      Serial.print(((color >> 8) & 0xFF), HEX); //red
      Serial.print(((color >> 16) & 0xFF), HEX); //green
      Serial.print((color & 0xFF), HEX); //blue
      Serial.print(", hue: ");
      Serial.println(hue);
#endif
    }

    void advance(int ledColors[NUMBER_OF_SEGMENTS][NUMBER_OF_LEDS_PER_SEGMENT][2]) {
      unsigned long age = millis() - birthday;
      hue += hueDeltaPerTick;
      if (state == dead)
        return;
#ifdef DEBUG_PRESSURE
        Serial.print("  Pressure calculation. Starting pressure: ");
        Serial.print(pressure);
        Serial.print(", age: ");
        Serial.print(age);
        Serial.print(", lifespan: ");
        Serial.println(lifespan);
#endif
      pressure += fmap(float(age), 0.0, float(lifespan), speed, speed/2);  // Ripple slows down as it ages
      // TODO: Motion of ripple is severely affected by loop speed. Make it time invariant
#ifdef DEBUG_PRESSURE
        Serial.print("  Pressure calculation. Ending pressure: ");
        Serial.println(pressure);
#endif
      if (pressure < 1 && (state == travelingUpwards || state == travelingDownwards)) {
        // Ripple is visible but hasn't moved - render it to avoid flickering
#ifdef DEBUG_PRESSURE
        Serial.println("  Calling renderLed() because pressure is less than 1");
#endif
        renderLed(ledColors, age);
      }

      while (pressure >= 1) {
#ifdef DEBUG_ADVANCEMENT
        Serial.print("Ripple ");
        Serial.print(rippleId);
        Serial.println(" advancing:");
#endif

        switch (state) {
          case withinNode: {
              if (justStarted) {
                justStarted = false;
              } else {
#ifdef DEBUG_ADVANCEMENT
                Serial.print("  Picking direction out of node ");
                Serial.print(position[0]);
                Serial.print(" with behavior ");
                Serial.print(behavior);
#endif
                int newDirection = -1;

                int sharpLeft = (position[1] + 1) % 6;
                int wideLeft = (position[1] + 2) % 6;
                int forward = (position[1] + 3) % 6;
                int wideRight = (position[1] + 4) % 6;
                int sharpRight = (position[1] + 5) % 6;

              
              if (behavior <= 2) {  // Semi-random aggressive turn mode
                  // The more aggressive a ripple, the tighter turns it wants to make.
                  // If there aren't any segments it can turn to, we need to adjust its behavior.
                  byte anger = behavior;

                  while (newDirection < 0) {
                    if (anger == 0) {
                      int forwardConnection = nodeConnections[position[0]][forward];

                      if (forwardConnection < 0) {
                        // We can't go straight ahead - we need to take a more aggressive angle
#ifdef DEBUG_ADVANCEMENT
                        Serial.println("  Can't go straight - picking more agr. path");
#endif
                        anger++;
                      }
                      else {
#ifdef DEBUG_ADVANCEMENT
                        Serial.println("  Going forward");
#endif
                        newDirection = forward;
                      }
                    }

                    if (anger == 1) {
                      int leftConnection = nodeConnections[position[0]][wideLeft];
                      int rightConnection = nodeConnections[position[0]][wideRight];

                      if (leftConnection >= 0 && rightConnection >= 0) {
#ifdef DEBUG_ADVANCEMENT
                        Serial.println("  Turning left or right at random");
#endif
                        if (rippleBias == noPreference) newDirection = random(2) ? wideLeft : wideRight;
                        else if(rippleBias >= noPreference && rippleBias <= preferLeftTwice) newDirection = wideLeft;
                        else if(rippleBias >= preferLeftTwice && rippleBias <= preferRightTwice) newDirection = wideRight;
                        
                        if (rippleBias == preferLeftOnce) rippleBias = noPreference;
                        if (rippleBias == preferLeftTwice) rippleBias = preferLeftOnce;
                        if (rippleBias == preferRightOnce) rippleBias = noPreference;
                        if (rippleBias == preferRightTwice) rippleBias = preferRightOnce;
                      }
                      else if (leftConnection >= 0) {
#ifdef DEBUG_ADVANCEMENT
                        Serial.println("  Can only turn left");
#endif
                        newDirection = wideLeft;
                      }
                      else if (rightConnection >= 0) {
#ifdef DEBUG_ADVANCEMENT
                        Serial.println("  Can only turn right");
#endif
                        newDirection = wideRight;
                      }
                      else {
#ifdef DEBUG_ADVANCEMENT
                        Serial.println("  Can't make wide turn - picking more agr. path");
#endif
                        anger++;  // Can't take shallow turn - must become more aggressive
                      }
                    }

                    if (anger == 2) {
                      int leftConnection = nodeConnections[position[0]][sharpLeft];
                      int rightConnection = nodeConnections[position[0]][sharpRight];

                      if (leftConnection >= 0 && rightConnection >= 0) {
#ifdef DEBUG_ADVANCEMENT
                        Serial.println("  Turning left or right at random");
#endif
                        if (rippleBias == noPreference) newDirection = random(2) ? sharpLeft : sharpRight;
                        else if(rippleBias >= noPreference && rippleBias <= preferLeftTwice) newDirection = sharpLeft;
                        else if(rippleBias >= preferLeftTwice && rippleBias <= preferRightTwice) newDirection = sharpRight;
                        
                        if (rippleBias == preferLeftOnce) rippleBias = noPreference;
                        if (rippleBias == preferLeftTwice) rippleBias = preferLeftOnce;
                        if (rippleBias == preferRightOnce) rippleBias = noPreference;
                        if (rippleBias == preferRightTwice) rippleBias = preferRightOnce;
                      }
                      else if (leftConnection >= 0) {
#ifdef DEBUG_ADVANCEMENT
                        Serial.println("  Can only turn left");
#endif
                        newDirection = sharpLeft;
                      }
                      else if (rightConnection >= 0) {
#ifdef DEBUG_ADVANCEMENT
                        Serial.println("  Can only turn right");
#endif
                        newDirection = sharpRight;
                      }
                      else {
#ifdef DEBUG_ADVANCEMENT
                        Serial.println("  Can't make tight turn - picking less agr. path");
#endif
                        anger--;  // Can't take tight turn - must become less aggressive
                      }
                    }

                    // Note that this can't handle some circumstances,
                    // like a node with segments in nothing but the 0 and 3 positions.
                    // Good thing we don't have any of those!
                  }
                }
                else if (behavior == alwaysTurnsRight) {
                  for (int i = 1; i < 6; i++) {
                    int possibleDirection = (position[1] + i) % 6;

                    if (nodeConnections[position[0]][possibleDirection] >= 0) {
                      newDirection = possibleDirection;
                      break;
                    }
                  }

#ifdef DEBUG_ADVANCEMENT
                  Serial.println("  Turning as rightward as possible");
#endif
                }
                else if (behavior == alwaysTurnsLeft) {
                  for (int i = 5; i >= 1; i--) {
                    int possibleDirection = (position[1] + i) % 6;

                    if (nodeConnections[position[0]][possibleDirection] >= 0) {
                      newDirection = possibleDirection;
                      break;
                    }
                  }

#ifdef DEBUG_ADVANCEMENT
                  Serial.println("  Turning as leftward as possible");
#endif
                }


#ifdef DEBUG_ADVANCEMENT
                Serial.print("  Leaving node ");
                Serial.print(position[0]);
                Serial.print(" in direction ");
                Serial.println(newDirection);
#endif

                position[1] = newDirection;
              }
              
              position[0] = nodeConnections[position[0]][position[1]];  // Look up which segment we're on


#ifdef DEBUG_ADVANCEMENT
              Serial.print("  and entering segment ");
              Serial.println(position[0]);
#endif

              if (position[1] == 5 || position[1] == 0 || position[1] == 1) {  //Direction is upwards
#ifdef DEBUG_ADVANCEMENT
                Serial.println("  (starting at bottom)");
#endif
                state = travelingUpwards;
                position[1] = 0;  // Starting at bottom of segment
              }
              else {
#ifdef DEBUG_ADVANCEMENT
                Serial.println("  (starting at top)");
#endif
                state = travelingDownwards;
                position[1] = (NUMBER_OF_LEDS_PER_SEGMENT-1); // Starting at top of LED strip
              }
              break;
            }

          case travelingUpwards: {
              position[1]++;

              if (position[1] >= NUMBER_OF_LEDS_PER_SEGMENT) {
                // We've reached the top!
#ifdef DEBUG_ADVANCEMENT
                Serial.print("  Reached top of seg. ");
                Serial.println(position[0]);
#endif
                // Enter the new node.
                nodeLimit--;
                int segment = position[0];
                position[0] = segmentConnections[position[0]][0]; /* assign position [0] to node we just enterred */
                for (int i = 0; i < 6; i++) {
                  // Figure out from which direction the ripple is entering the node.
                  // Allows us to exit in an appropriately aggressive direction.
                  int incomingConnection = nodeConnections[position[0]][i];
                  if (incomingConnection == segment)
                    position[1] = i;
                }
                state = withinNode;

#ifdef DEBUG_ADVANCEMENT
                Serial.print("  Entering node ");
                Serial.print(position[0]);
                Serial.print(" from direction ");
                Serial.println(position[1]);
#endif
              }
              else {

#ifdef DEBUG_ADVANCEMENT
                Serial.print("  Moved up to seg. ");
                Serial.print(position[0]);
                Serial.print(" LED ");
                Serial.println(position[1]);
#endif
              }
              break;
            }
          case travelingDownwards: {
              position[1]--;
              if (position[1] < 0) {
                // We've reached the bottom!
#ifdef DEBUG_ADVANCEMENT
                Serial.print("  Reached bottom of seg. ");
                Serial.println(position[0]);
#endif
                // Enter the new node.
                nodeLimit--;
                int segment = position[0];
                position[0] = segmentConnections[position[0]][1];
                for (int i = 0; i < 6; i++) {
                  // Figure out from which direction the ripple is entering the node.
                  // Allows us to exit in an appropriately aggressive direction.
                  int incomingConnection = nodeConnections[position[0]][i];
                  if (incomingConnection == segment){
                    position[1] = i;
                  }
                }
              state = withinNode;

#ifdef DEBUG_ADVANCEMENT
                Serial.print("  Entering node ");
                Serial.print(position[0]);
                Serial.print(" from direction ");
                Serial.println(position[1]);
#endif
              }
              else {
#ifdef DEBUG_ADVANCEMENT
                Serial.print("  Moved down to seg. ");
                Serial.print(position[0]);
                Serial.print(" LED ");
                Serial.println(position[1]);
#endif
              }
              break;
            }

          default:
            break;
        }

        pressure -= 1;

        if (state == travelingUpwards || state == travelingDownwards) {
          // Ripple is visible - render it
#ifdef DEBUG_ADVANCEMENT
          Serial.println("  Calling renderLed() inside while loop");
#endif
          renderLed(ledColors, age);
        }
      }

#ifdef DEBUG_PRESSURE
      Serial.print("  Age is now ");
      Serial.print(age);
      Serial.print('/');
      Serial.println(lifespan);
      Serial.print("  Pressure is now ");
      Serial.println(pressure);
#endif

      if ( (lifespan && age >= lifespan) || (nodeLimit == 0) ){
        // We dead
#ifdef DEBUG_ADVANCEMENT
        Serial.println("  Lifespan is up! Ripple is dead.");
#endif
        state = dead;
        position[0] = position[1] = pressure = age = 0;
      }
    }

  private:
    float speed;  // Each loop, ripples move this many LED's.
    unsigned long lifespan;  // The ripple stops after this many milliseconds

    /*
       0: Always goes straight ahead if possible
       1: Can take 60-degree turns
       2: Can take 120-degree turns
    */
    byte behavior;

    bool justStarted = false;

    float pressure;  // When Pressure reaches 1, ripple will move
    unsigned long birthday;  // Used to track age of ripple

    static byte rippleCount;  // Used to give them unique ID's
    byte rippleId;  // Used to identify this ripple in debug output

    void renderLed(int ledColors[NUMBER_OF_SEGMENTS][NUMBER_OF_LEDS_PER_SEGMENT][2], unsigned long age) {
      int strip = ledAssignments[position[0]][0];
      int led = ledAssignments[position[0]][2] + position[1];
      
#ifdef DEBUG_RENDERING
      Serial.print("Rendering ripple position (");
      Serial.print(position[0]);
      Serial.print(',');
      Serial.print(position[1]);
      Serial.print(") at Strip ");
      Serial.print(strip);
      Serial.print(", LED ");
      Serial.print(led);
      /*
      for (int i = 0; i < 3; i++) {
        if (ledColors[position[0]][position[1]][i] <= 0x0F)
          Serial.print('0');
        Serial.print(ledColors[position[0]][position[1]][i], HEX);
      }
      */
#endif

      ledColors[position[0]][position[1]][0] = short(hue);
      ledColors[position[0]][position[1]][1] = 255; //increase brightness
      

#ifdef DEBUG_RENDERING
      Serial.print(", new hue: ");
      Serial.println(hue);

#endif
    }
};


#endif
