#include "ASW.h"

bool OTAinProgress = 0;

bool FireEffect_Random(int* firstRipple, int color, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, unsigned short nodeLimit){
  int currentRipple;
  bool rippleFired = 0;
  int dir;
  currentRipple = *firstRipple;

  unsigned int effectSelection = random(NUMBER_OF_EFFECTS);
  switch(effectSelection) {
  case 0: /* fire star */
    rippleFired |= FireEffect_Star(&currentRipple, color, behavior, lifespan, speed, hDelta);
    break;
  case 1: /* fire QuadShard in random direction */
    rippleFired |= FireEffect_CenterNode_QuadShard(&currentRipple, random(6), color, behavior, lifespan, speed, hDelta, nodeLimit);
    break;
  case 2: /* fire Doubleripple in all directions from center node */
    rippleFired |= FireDoubleRipple(&currentRipple, 0, color, starburstNode, behavior, lifespan, speed, hDelta, nodeLimit);
    rippleFired |= FireDoubleRipple(&currentRipple, 1, color, starburstNode, behavior, lifespan, speed, hDelta, nodeLimit);
    rippleFired |= FireDoubleRipple(&currentRipple, 2, color, starburstNode, behavior, lifespan, speed, hDelta, nodeLimit);
    rippleFired |= FireDoubleRipple(&currentRipple, 3, color, starburstNode, behavior, lifespan, speed, hDelta, nodeLimit);
    rippleFired |= FireDoubleRipple(&currentRipple, 4, color, starburstNode, behavior, lifespan, speed, hDelta, nodeLimit);
    rippleFired |= FireDoubleRipple(&currentRipple, 5, color, starburstNode, behavior, lifespan, speed, hDelta, nodeLimit);
    break;
  case 3: /* fire shard in random direction and also in its opposite direction */
    dir = random(6); /* pick random direction */
    rippleFired |= FireShard(&currentRipple, dir, color, starburstNode, behavior, lifespan, speed, hDelta, nodeLimit);
    rippleFired |= FireShard(&currentRipple, (dir+3)%6, color, starburstNode, behavior, lifespan, speed, hDelta, nodeLimit);
    break;
  default:
    // code block
    break;
  }

  *firstRipple = currentRipple;
  return rippleFired;
}


/* bulk FireRipple functions */

bool FireRipple_AllOddCubeNodes(int* firstRipple, int dir, int color, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, directionBias bias, unsigned short nodeLimit){
  int currentRipple;
  bool rippleFired = 0;
  currentRipple = *firstRipple;
  for(int i = 0; i < numberOfCubeOddNodes; i++){
    if( dir < 0 ){ /* fire in all directions */
      rippleFired |= FireRipple(&currentRipple, 1, color, cubeOddNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
      rippleFired |= FireRipple(&currentRipple, 3, color, cubeOddNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
      rippleFired |= FireRipple(&currentRipple, 5, color, cubeOddNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
    } else { /* fire only in one direction */
      rippleFired |= FireRipple(&currentRipple, dir, color, cubeOddNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
    }
  }
  *firstRipple = currentRipple;
  return rippleFired;
}

bool FireRipple_AllPairCubeNodes(int* firstRipple, int dir, int color, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, directionBias bias, unsigned short nodeLimit){
  int currentRipple;
  bool rippleFired = 0;
  currentRipple = *firstRipple;
  for(int i = 0; i < numberOfCubePairNodes; i++){
    if( dir < 0 ){ /* fire in all directions */
      rippleFired |= FireRipple(&currentRipple, 0, color, cubePairNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
      rippleFired |= FireRipple(&currentRipple, 2, color, cubePairNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
      rippleFired |= FireRipple(&currentRipple, 4, color, cubePairNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
    } else { /* fire only in one direction */
      rippleFired |= FireRipple(&currentRipple, dir, color, cubePairNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
    }
  }
  *firstRipple = currentRipple;
  return rippleFired;
}

bool FireRipple_AllCubeNodes(int* firstRipple, int dir, int color, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, directionBias bias, unsigned short nodeLimit){
  int currentRipple;
  bool rippleFired = 0;
  currentRipple = *firstRipple;
  if( dir < 0 ){ /* fire in all directions */
    rippleFired = FireRipple_AllPairCubeNodes(&currentRipple, -1, color, behavior, lifespan, speed, hDelta, bias, nodeLimit);
    rippleFired = FireRipple_AllOddCubeNodes(&currentRipple, -1, color, behavior, lifespan, speed, hDelta, bias, nodeLimit);
  } else { /* fire only in one direction */
    rippleFired = FireRipple_AllPairCubeNodes(&currentRipple, dir, color, behavior, lifespan, speed, hDelta, bias, nodeLimit);
    rippleFired = FireRipple_AllOddCubeNodes(&currentRipple, dir, color, behavior, lifespan, speed, hDelta, bias, nodeLimit);
  }
  *firstRipple = currentRipple;
  return rippleFired;
}

bool FireRipple_AllQuadNodes(int* firstRipple, int dir, int color, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, directionBias bias, unsigned short nodeLimit){
  int currentRipple;
  bool rippleFired = 0;
  currentRipple = *firstRipple;
  for(int i = 0; i < numberOfQuadNodes; i++){
    if( dir < 0 ){ /* fire in all directions */
      rippleFired |= FireRipple(&currentRipple, 0, color, QuadNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
      rippleFired |= FireRipple(&currentRipple, 1, color, QuadNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
      rippleFired |= FireRipple(&currentRipple, 2, color, QuadNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
      rippleFired |= FireRipple(&currentRipple, 3, color, QuadNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
      rippleFired |= FireRipple(&currentRipple, 4, color, QuadNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
      rippleFired |= FireRipple(&currentRipple, 5, color, QuadNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
    } else { /* fire only in one direction */
      rippleFired |= FireRipple(&currentRipple, dir, color, QuadNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
    }
  }
  *firstRipple = currentRipple;
  return rippleFired;
}

bool FireRipple_AllBorderNodes(int* firstRipple, int dir, int color, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, directionBias bias, unsigned short nodeLimit){
  int currentRipple;
  bool rippleFired = 0;
  currentRipple = *firstRipple;
  for(int i = 0; i < numberOfBorderNodes; i++){
    if( dir < 0 ){ /* fire in all directions */
      rippleFired |= FireRipple(&currentRipple, 0, color, borderNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
      rippleFired |= FireRipple(&currentRipple, 1, color, borderNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
      rippleFired |= FireRipple(&currentRipple, 2, color, borderNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
      rippleFired |= FireRipple(&currentRipple, 3, color, borderNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
      rippleFired |= FireRipple(&currentRipple, 4, color, borderNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
      rippleFired |= FireRipple(&currentRipple, 5, color, borderNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
    } else { /* fire only in one direction */
      rippleFired |= FireRipple(&currentRipple, dir, color, borderNodes[i], behavior, lifespan, speed, hDelta, bias, nodeLimit);
      currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
    }
  }
  *firstRipple = currentRipple;
  return rippleFired;
}

bool FireRipple_CenterNode(int* firstRipple, int dir, int color, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, directionBias bias, unsigned short nodeLimit){
  int currentRipple;
  bool rippleFired = 0;
  currentRipple = *firstRipple;
  if( dir < 0 ){ /* fire in all directions */
    rippleFired |= FireRipple(&currentRipple, 0, color, starburstNode, behavior, lifespan, speed, hDelta, bias, nodeLimit);
    currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
    rippleFired |= FireRipple(&currentRipple, 1, color, starburstNode, behavior, lifespan, speed, hDelta, bias, nodeLimit);
    currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
    rippleFired |= FireRipple(&currentRipple, 2, color, starburstNode, behavior, lifespan, speed, hDelta, bias, nodeLimit);
    currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
    rippleFired |= FireRipple(&currentRipple, 3, color, starburstNode, behavior, lifespan, speed, hDelta, bias, nodeLimit);
    currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
    rippleFired |= FireRipple(&currentRipple, 4, color, starburstNode, behavior, lifespan, speed, hDelta, bias, nodeLimit);
    currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
    rippleFired |= FireRipple(&currentRipple, 5, color, starburstNode, behavior, lifespan, speed, hDelta, bias, nodeLimit);
    currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
  } else { /* fire only in one direction */
    rippleFired |= FireRipple(&currentRipple, dir, color, starburstNode, behavior, lifespan, speed, hDelta, bias, nodeLimit);
    currentRipple = (currentRipple)%MAX_NUMBER_OF_RIPPLES;
  }
  *firstRipple = currentRipple;
  return rippleFired;
}



/* special effects */
bool FireEffect_Star(int* firstRipple, int color, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta){
  int currentRipple;
  bool rippleFired = 0;
  currentRipple = *firstRipple;
  rippleFired |= FireRipple_CenterNode(&currentRipple, ALL_DIRECTIONS, color, behavior, lifespan, speed, hDelta, preferLeftTwice, 2);
  rippleFired |= FireRipple_CenterNode(&currentRipple, ALL_DIRECTIONS, color, behavior, lifespan, speed, hDelta, preferRightTwice, 2);
  *firstRipple = currentRipple;
  return rippleFired;
}

bool FireEffect_CenterNode_QuadShard(int *firstRipple, int dir, int color, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, unsigned short nodeLimit){
  int currentRipple;
  bool rippleFired = 0;
  currentRipple = *firstRipple;
  
  rippleFired |= FireShard(&currentRipple, dir, color, starburstNode, behavior, lifespan, speed, hDelta, nodeLimit);
  rippleFired |= FireShard(&currentRipple, (dir+1)%6, color, starburstNode, behavior, lifespan, speed, hDelta, nodeLimit);
  rippleFired |= FireShard(&currentRipple, (dir+3)%6, color, starburstNode, behavior, lifespan, speed, hDelta, nodeLimit);
  rippleFired |= FireShard(&currentRipple, (dir+4)%6, color, starburstNode, behavior, lifespan, speed, hDelta, nodeLimit);
  
  *firstRipple = currentRipple;
  return rippleFired;
}



void onOTAStart()
{
  // Log when OTA has started
  udp_println("OTA update started!");
  OTAinProgress = 1;
  Ripple_KillAllRipples();

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
static int lastModifiedLEDinbex = 0;
static int lastModifiedSegment = 0;

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
        ledHues[lastModifiedSegment][lastModifiedLEDinbex][1] = 255; // Set Brightness to max
    }

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
        if(segmentIndex < 6 || (segmentIndex >= 9))
        {
          /*flip order for first half segments so ripple travels downwards*/
          ledIndex = (NUMBER_OF_LEDS_PER_SEGMENT - 1 - ledIndex);
        }

        ledHues[segment][ledIndex][0] = 0x1AA7EC;                 // Set Hue to 0x1AA7EC (blue)
        ledHues[segment][ledIndex][1] = (short)partialBrightness; // Set the calculated brightness
        lastModifiedLEDinbex = ledIndex;
        lastModifiedSegment = segment;
        // Debugging output
        // String debugMessage = String("[Partial light] segmentIndex: ") + segmentIndex +
        //                       ", segment: " + segment +
        //                       ", ledWithinSegment: " + ledWithinSegment +
        //                       ", ledIndex: " + ledIndex +
        //                       ", ledAssignments[segment][1] (ceiling): " + ledAssignments[segment][1] +
        //                       ", ledAssignments[segment][2] (floor): " + ledAssignments[segment][2] +
        //                       ", fullLEDs: " + fullLEDs;
        // udp_println(debugMessage); // Commented out for performance.
      }
    }
    previousFullLEDs = fullLEDs; // Update the number of LEDs lit for the next call

    // SetPixelColor all leds to ledColors
    SetPixelColor_AllLEDs();
    
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
