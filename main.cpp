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
bool OTAinProgress = 0;

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

void onOTAStart()
{
  // Log when OTA has started
  udp_println("OTA update started!");
  Ripple_KillAllRipples();
  OTAinProgress = 1;

  // Set ALL LEDs to off once at OTA start
  for (int segment = 0; segment < NUMBER_OF_SEGMENTS; segment++)
  {
    for (int ledWithinSegment = 0; ledWithinSegment < NUMBER_OF_LEDS_PER_SEGMENT; ledWithinSegment++)
    {
      ledHues[segment][ledWithinSegment][0] = 0; // Set Hue to 0
      ledHues[segment][ledWithinSegment][1] = 0; // Set Brightness to 0
    }
  }
}

// Global variable to store the number of LEDs lit in the previous OTA progress update
static int previousFullLEDs = 0;

void onOTAProgress(size_t current, size_t final)
{
  // Calculate the progress percentage
  float progress = (float)current / (float) final;

  // Calculate the number of LEDs to light up completely
  float ledsToLightUp = progress * numberOfPerimeterLEDs;
  int fullLEDs = (int)ledsToLightUp;

  // Calculate the brightness for the partially lit LED
  float partialBrightness = (ledsToLightUp - fullLEDs) * 255.0f;

  // Light up the full LEDs, only if the progress has changed
  if (fullLEDs != previousFullLEDs)
  {
    int ledCount = previousFullLEDs; // Start from where we left off

    // Before starting lighting process, make sure the partially lit LED from previous update
    // is fully lit
    if (previousFullLEDs < numberOfPerimeterLEDs)
    {
      // Calculate the segment and LED within the segment directly
      int segmentIndex = previousFullLEDs / NUMBER_OF_LEDS_PER_SEGMENT;     // which segment is it
      int ledWithinSegment = previousFullLEDs % NUMBER_OF_LEDS_PER_SEGMENT; // which led within the segment it is

      if (segmentIndex < numberOfPerimeterSegments)
      {
        int segment = perimeterSegments[segmentIndex];
        ledHues[segment][ledWithinSegment][1] = 255; // Set Brightness to max
      }
    }

    // for (int segmentIndex = 0; segmentIndex < numberOfPerimeterSegments; segmentIndex++)
    // {
    //   int segment = perimeterSegments[segmentIndex];
    //   for (int ledWithinSegment = 0; ledWithinSegment < NUMBER_OF_LEDS_PER_SEGMENT; ledWithinSegment++)
    //   {
    //     if (ledCount < fullLEDs)
    //     {
    //       ledHues[segment][ledWithinSegment][0] = 0x1AA7EC; // Set Hue to 0x1AA7EC (blue)
    //       ledHues[segment][ledWithinSegment][1] = 255;      // Set Brightness to max
    //       ledCount++;
    //       // Debugging output
    //       String debugMessage = String("[Full light] segmentIndex: ") + segmentIndex +
    //                             ", segment: " + segment +
    //                             ", ledWithinSegment: " + ledWithinSegment +
    //                             ", ledCount: " + ledCount +
    //                             ", fullLEDs: " + fullLEDs;
    //       udp_println(debugMessage); // Commented out for performance.
    //     }
    //     else
    //     {
    //       // No need to set to 0, already set at the beginning
    //       // ledHues[perimeterSegments[segment]][ledWithinSegment][0] = 0;
    //       // ledHues[perimeterSegments[segment]][ledWithinSegment][1] = 0;
    //     }
    //   }
    // }

    }

    // Light up the partially lit LED
    if (fullLEDs < numberOfPerimeterLEDs)
    {
      // Calculate the segment and LED within the segment directly
      int segmentIndex = fullLEDs / NUMBER_OF_LEDS_PER_SEGMENT;     // which segment is it
      int ledWithinSegment = fullLEDs % NUMBER_OF_LEDS_PER_SEGMENT; // which led within the segment it is

      if (segmentIndex < numberOfPerimeterSegments)
      {
        int segment = perimeterSegments[segmentIndex];
        int ledIndex = ledWithinSegment; // initialize it, will change

        int strip = ledAssignments[segment][0];

        if (ledAssignments[segment][2] < ledAssignments[segment][1])
        { // Normal order
          ledIndex = ledWithinSegment;
        }
        else
        { // Reversed order
          ledIndex = (NUMBER_OF_LEDS_PER_SEGMENT - 1 - ledWithinSegment);
        }

        ledHues[segment][ledIndex][0] = 0x1AA7EC;                 // Set Hue to 0x1AA7EC (blue)
        ledHues[segment][ledIndex][1] = (short)partialBrightness; // Set the calculated brightness
        // Debugging output
        String debugMessage = String("[Partial light] segmentIndex: ") + segmentIndex +
                              ", segment: " + segment +
                              ", ledWithinSegment: " + ledWithinSegment +
                              ", ledIndex: " + ledIndex +
                              ", partialBrightness: " + partialBrightness +
                              ", fullLEDs: " + fullLEDs;
        udp_println(debugMessage); // Commented out for performance.
      }
    }
    previousFullLEDs = fullLEDs; // Update the number of LEDs lit for the next call

    // SetPixelColor all leds to ledColors
    for (int segment = 0; segment < NUMBER_OF_SEGMENTS; segment++)
    {
      for (int fromBottom = 0; fromBottom < NUMBER_OF_LEDS_PER_SEGMENT; fromBottom++)
      {
        int strip = ledAssignments[segment][0];
        int led;

        if (ledAssignments[segment][2] < ledAssignments[segment][1])
        { // Normal order
          led = ledAssignments[segment][2] + fromBottom;
        }
        else
        { // Reversed order
          led = ledAssignments[segment][1] + (NUMBER_OF_LEDS_PER_SEGMENT - 1 - fromBottom);
        }
        unsigned long color = strips[strip].ColorHSV(ledHues[segment][fromBottom][0], 255, ledHues[segment][fromBottom][1]);
        strips[strip].setPixelColor(led, color);
      }
    }
    for (int stripIndex = 0; stripIndex < NUMBER_OF_STRIPS; stripIndex++)
    {
      strips[stripIndex].show();
    }

    // Log the progress (debugging)
    // String otaProgressMessage = String("OTA Progress: ") + current + " bytes / " + final + " bytes (" + String(progress * 100, 2) + "%%), Full LEDs: " + fullLEDs + ", Partial Brightness: " + String(partialBrightness, 2);
    // udp_println(otaProgressMessage);
  }

  void onOTAEnd(bool success)
  {
    // Log when OTA has finished
    if (success)
    {
      udp_println("OTA update finished successfully!");
      for (int segment = 0; segment < NUMBER_OF_SEGMENTS; segment++)
      {
        setSegmentColor(segment, 0x74C365); /* 0x74C365 is green for success */
      }
    }
    else
    {
      udp_println("There was an error during OTA update!");
      for (int segment = 0; segment < NUMBER_OF_SEGMENTS; segment++)
      {
        setSegmentColor(segment, 0x0); /* 0x0 is red for failure */
      }
    }
  }

  void setup()
  {
    Serial.begin(115200);

    // EEPROM.begin(EEPROM_SIZE);
    // Global_NumberOfProfiles_InDFLS = EEPROM_ParseProfiles();
    // EEPROM_Read_GlobalParameters();

    pinMode(LED, OUTPUT);
    Strips_init();
    WiFi_Utilities_init();
    ElegantOTA.onStart(onOTAStart);
    ElegantOTA.onProgress(onOTAProgress);
    ElegantOTA.onEnd(onOTAEnd);
    String ipAddress = WiFi.localIP().toString();                       // Get the microcontroller's IP address as a string
    String udpMessage = "Chromance device is ONLINE! IP: " + ipAddress; // Create the UDP message with the IP address
    udp_println(udpMessage);
  }

  void loop()
  {
    unsigned long benchmark = millis();
    int rippleFired_return = 0;

    WiFi_Utilities_loop();

    if (!OTAinProgress)
    {
      Ripple_MainFunction(); /* advance all ripples, show all strips, fade all leds, setPixelColor all leds */

      if ((benchmark - lastRippleTime) > GlobalParameters.currentDelayBetweenRipples)
      {
        DelayPeriodActive = 0; /* Delay has passed; we may now begin a new burst*/
      }

      if (!DelayPeriodActive && GlobalParameters.loop_MasterFireRippleEnabled)
      {                            /* fire cyclic burst */
        lastRippleTime = millis(); /* update lastRippleTime to reset delay window counter */
        DelayPeriodActive = 1;
        rippleFired_return |= FireRipple_CenterNode(&nextRipple, GlobalParameters.currentDirection, nextColor, GlobalParameters.currentBehavior, GlobalParameters.currentRippleLifeSpan, GlobalParameters.currentRippleSpeed, GlobalParameters.currentRainbowDeltaPerTick, noPreference, NO_NODE_LIMIT);
      }

      if (manualFireRipple)
      { /* fire manual burst */
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
  }
