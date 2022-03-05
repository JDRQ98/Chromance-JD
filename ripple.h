/*
   A dot animation that travels along rails
   (C) Voidstar Lab LLC 2021
*/

#ifndef RIPPLE_H_
#define RIPPLE_H_

// WARNING: These slow things down enough to affect performance. Don't turn on unless you need them!
//#define DEBUG_ADVANCEMENT  // Print debug messages about ripples' movement
//#define DEBUG_RENDERING  // Print debug messages about translating logical to actual position

#include <Adafruit_NeoPixel.h>
#include "mapping.h"

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

float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

class Ripple {
  public:
    Ripple(int id) : rippleId(id) {
      Serial.print("Instanced ripple #");
      Serial.println(rippleId);
    }

    rippleState state = dead;
    unsigned long color;
    int currentLed = 0;

    /*
       If within a node: 0 is node, 1 is direction
       If traveling, 0 is segment, 1 is LED position from bottom
    */
    int position[2];

    // Place the Ripple in a node
    void start(byte n, byte d, unsigned long c, float s, unsigned long l, byte b) {
      color = c;
      speed = s;
      lifespan = l;
      behavior = b;

      birthday = millis();
      pressure = 0;
      state = withinNode;

      position[0] = n;
      position[1] = d;

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
      Serial.println((color & 0xFF), HEX); //blue
#endif
    }

    void advance(byte ledColors[1][11][3]) {
      unsigned long age = millis() - birthday;

      if (state == dead)
        return;
#ifdef DEBUG_ADVANCEMENT
        Serial.print("  Pressure calculation. Starting pressure: ");
        Serial.print(pressure);
        Serial.print(", age: ");
        Serial.print(age);
        Serial.print(", lifespan: ");
        Serial.println(lifespan);
        
#endif
      pressure += fmap(float(age), 0.0, float(lifespan), speed, speed/2);  // Ripple slows down as it ages
      // TODO: Motion of ripple is severely affected by loop speed. Make it time invariant
#ifdef DEBUG_ADVANCEMENT
        Serial.print("  Pressure calculation. Ending pressure: ");
        Serial.println(pressure);
#endif
      if (pressure < 1 && (state == travelingUpwards || state == travelingDownwards)) {
        // Ripple is visible but hasn't moved - render it to avoid flickering
        Serial.println("  Calling renderLed() because pressure is less than 1");
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
              }

              position[0] = nodeConnections[position[0]][position[1]];  // Look up which segment we're on

#ifdef DEBUG_ADVANCEMENT
              Serial.print("  and entering segment ");
              Serial.println(position[0]);
#endif

              if (currentLed == 5 || currentLed == 0 || currentLed == 1) {  // Top half of the node
#ifdef DEBUG_ADVANCEMENT
                Serial.println("  (starting at bottom)");
#endif
                state = travelingUpwards;
                currentLed = 0;  // Starting at bottom of segment
              }
              else {
#ifdef DEBUG_ADVANCEMENT
                Serial.println("  (starting at top)");
#endif
                state = travelingDownwards;
                currentLed = 10; // Starting at top of 11-LED-long strip
              }
              break;
            }

          case travelingUpwards: {
              currentLed++;

              if (currentLed >= 11) {
                // We've reached the top!
#ifdef DEBUG_ADVANCEMENT
                Serial.print("  Reached top of seg. ");
                Serial.println(position[0]);
#endif
                state = dead;
              }
              else {
#ifdef DEBUG_ADVANCEMENT
                Serial.print("  Moved up to seg. ");
                Serial.print(position[0]);
                Serial.print(" LED ");
                Serial.println(currentLed);
#endif
              }
              break;
            }
          case travelingDownwards: {
              currentLed--;
              if (position[1] < 0) {
                // We've reached the bottom!
#ifdef DEBUG_ADVANCEMENT
                Serial.print("  Reached bottom of seg. ");
                Serial.println(position[0]);
#endif
                state = dead;
              }
              else {
#ifdef DEBUG_ADVANCEMENT
                Serial.print("  Moved down to seg. ");
                Serial.print(position[0]);
                Serial.print(" LED ");
                Serial.println(currentLed);
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
          Serial.println("  Calling renderLed() inside while loop");
          renderLed(ledColors, age);
        }
      }

#ifdef DEBUG_ADVANCEMENT
      Serial.print("  Age is now ");
      Serial.print(age);
      Serial.print('/');
      Serial.println(lifespan);
      Serial.print("  Pressure is now ");
      Serial.println(pressure);
#endif

      if (lifespan && age >= lifespan) {
        // We dead
#ifdef DEBUG_ADVANCEMENT
        Serial.println("  Lifespan is up! Ripple is dead.");
#endif
        state = dead;
        position[0] = position[1] = currentLed = pressure = age = 0;
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

    void renderLed(byte ledColors[1][11][3], unsigned long age) {
      int strip = ledAssignments[position[0]][0];
      int led = ledAssignments[position[0]][1] + currentLed;

      int red = ledColors[position[0]][led][0];
      int green = ledColors[position[0]][led][1];
      int blue = ledColors[position[0]][led][2];
#ifdef DEBUG_RENDERING
      Serial.print("Rendering ripple position (");
      Serial.print(position[0]);
      Serial.print(',');
      Serial.print(position[1]);
      Serial.print(") at Strip ");
      Serial.print(strip);
      Serial.print(", LED ");
      Serial.println(led);
      Serial.print(". ledColors address: 0x");
      Serial.print((unsigned long) &ledColors[0][0][0], HEX);
      Serial.print(", old color (RGB): 0x");
      for (int i = 0; i < 3; i++) {
        if (ledColors[position[0]][led][i] <= 0x0F)
          Serial.print('0');
        Serial.print(ledColors[position[0]][led][i], HEX);
      }
#endif

      ledColors[position[0]][led][0] = byte(min(255, max(0, int(fmap(float(age+lifespan), 0.0, float(2*lifespan), 0.0, (color >> 8) & 0xFF)))));
      ledColors[position[0]][led][1] = byte(min(255, max(0, int(fmap(float(age+lifespan), 0.0, float(2*lifespan), 0.0, (color >> 16) & 0xFF)))));
      ledColors[position[0]][led][2] = byte(min(255, max(0, int(fmap(float(age+lifespan), 0.0, float(2*lifespan), 0.0, color & 0xFF)))));
      

      //ledColors[position[0]][led][0] = byte(min(255, max(0, int(fmap(float(age), 0.0, float(lifespan), (color >> 8) & 0xFF, 0.0)) + red)));
      //ledColors[position[0]][led][1] = byte(min(255, max(0, int(fmap(float(age), 0.0, float(lifespan), (color >> 16) & 0xFF, 0.0)) + green)));
      //ledColors[position[0]][led][2] = byte(min(255, max(0, int(fmap(float(age), 0.0, float(lifespan), color & 0xFF, 0.0)) + blue)));
      
#ifdef DEBUG_RENDERING
      Serial.print(", new color (RGB): 0x");
      for (int i = 0; i < 3; i++) {
        if (ledColors[position[0]][led][i] <= 0x0F)
          Serial.print('0');
        Serial.print(ledColors[position[0]][led][i], HEX);
      }
      Serial.println();
#endif
    }
};

#endif
