#include "ripple.h"
#include "HTTP_Server.h" /* for control variables */

int lengths[NUMBER_OF_STRIPS] = {165, 165}; 

//strip(NUMLEDS, DATAPIN, CLOCKPIN, DOTSTART_BRG)
Adafruit_NeoPixel strip0(lengths[0], 15,  NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip1(lengths[1], 2,  NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strips[NUMBER_OF_STRIPS] = {strip0, strip1};

// These ripples are endlessly reused so we don't need to do any memory management
Ripple ripples[NUMBER_OF_RIPPLES] = {
  Ripple(0),
  Ripple(1),
  Ripple(2),
  Ripple(3),
  Ripple(4),
  Ripple(5),
  Ripple(6),
  Ripple(7),
  Ripple(8),
  Ripple(9),
  Ripple(10),
  Ripple(11),
  Ripple(12),
  Ripple(13),
  Ripple(14),
  Ripple(15),
  Ripple(16),
  Ripple(17),
  Ripple(18),
  Ripple(19),
  Ripple(20),
  Ripple(21),
  Ripple(22),
  Ripple(23),
  Ripple(24),
  Ripple(25),
  Ripple(26),
  Ripple(27),
  Ripple(28),
  Ripple(29),
  Ripple(30),
  Ripple(31),
  Ripple(32),
  Ripple(33),
  Ripple(34),
  Ripple(35),
  Ripple(36),
  Ripple(37),
  Ripple(38),
  Ripple(39),
  Ripple(40),
  Ripple(41),
  Ripple(42),
  Ripple(43),
  Ripple(44),
  Ripple(45),
  Ripple(46),
  Ripple(47),
  Ripple(48),
  Ripple(49),
  Ripple(50),
  Ripple(51),
  Ripple(52),
  Ripple(53),
  Ripple(54),
  Ripple(55),
  Ripple(56),
  Ripple(57),
  Ripple(58),
  Ripple(59),
  Ripple(60),
  Ripple(61),
  Ripple(62),
  Ripple(63),
  Ripple(64),
  Ripple(65),
  Ripple(66),
  Ripple(67),
  Ripple(68),
  Ripple(69),
  Ripple(70),
  Ripple(71),
  Ripple(72),
  Ripple(73),
  Ripple(74),
  Ripple(75),
  Ripple(76),
  Ripple(77),
  Ripple(78),
  Ripple(79),
  Ripple(80),
  Ripple(81),
  Ripple(82),
  Ripple(83),
  Ripple(84),
  Ripple(85),
  Ripple(86),
  Ripple(87),
  Ripple(88),
  Ripple(89),
  Ripple(90),
  Ripple(91),
  Ripple(92),
  Ripple(93),
  Ripple(94),
  Ripple(95),
  Ripple(96),
  Ripple(97),
  Ripple(98),
  Ripple(99)
};

float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


void Strips_init(){
   for (int i = 0; i < NUMBER_OF_STRIPS; i++) {
    strips[i].begin();
    strips[i].setBrightness(128U);  // If your PSU sucks, use this to limit the current
    strips[i].show();
  }
}

void Ripple_MainFunction(){
      // Fade all dots to create trails
  for (int segment = 0; segment < NUMBER_OF_SEGMENTS ; segment++){
    for (int led = 0; led < 11; led++) {
       ledHues[segment][led][1] *= GlobalParameters.currentDecay; //fade brightness
    }
  }

  

  //SetPixelColor all leds to ledColors
  for (int segment = 0; segment < NUMBER_OF_SEGMENTS ; segment++){
    for (int fromBottom = 0; fromBottom < 11; fromBottom++) {
      int strip = ledAssignments[segment][0];
      int led = round(fmap(
                        fromBottom,
                        0, 10,
                        ledAssignments[segment][2], ledAssignments[segment][1]));
      unsigned long color = strips[strip].ColorHSV(ledHues[segment][fromBottom][0], 255, ledHues[segment][fromBottom][1]);
      strips[strip].setPixelColor(led, color);
    }
  }

  /* Advance all ripples */
  for (int i = 0; i < GlobalParameters.currentNumberofRipples; i++)
  {
    ripples[i].advance(ledHues);
  }

  /* Show all strips */
  for (int strip = 0; strip < NUMBER_OF_STRIPS ; strip++){
    strips[strip].show();
  }
}



/* elemental FireRipple functions */

bool FireRipple(int* ripple, int dir, int col, int node, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, directionBias bias, unsigned short nodeLimit){
  //int hue = fmap(random(100), 0, 99, 0, 0xFFFF);
  int tempRipple = *ripple;
  int hue = fmap(col, 0, GlobalParameters.currentNumberofColors, 0, 0xFFFF);
  if(ripples[tempRipple].state == dead){ /* if ripple is not currently active */
    if(nodeConnections[node][dir] >= 0){ /* check if dir and node are valid */
      ripples[tempRipple].start(
        node, //starting node
        dir, //direction
        strip0.ColorHSV(hue, 255, 255),
        //float(random(100)) / 100.0 * .2 + .8, //speed
        speed, //speed
        lifespan, //lifespan
        behavior, //behavior, 3 = always turn right
        hue,
        hDelta,
        bias,
        nodeLimit
     );
    *ripple = tempRipple+1; /* increase ripple number */
    return 1; /* ripple was fired */
    } else {
      /* direction is not valid for requested node; ignore request */
      return 0;
    }
  } else{
     /* ripple is currently active; ignore request */
     return 0;
  }
    #ifdef ENABLE_DEBUGGING
      Serial.print("Firing ripple ");
      Serial.print(ripple);
      Serial.print(" from node ");
      Serial.print(node);
      Serial.print(" in direction ");
      Serial.println(dir);
    #endif
};


/* FireDoubleRipple
    Fires two ripples: one biased towards Right and one biased towards left in the same direction
    dependencies: FireRipple
*/
bool FireDoubleRipple(int* firstRipple, int dir, int color, int node, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, unsigned short nodeLimit){
  int currentRipple;
  bool rippleFired = 0;
  currentRipple = *firstRipple;
  if (dir < 0)
  { /* fire in all directions */
    rippleFired |= FireRipple(&currentRipple, 0, color, node, behavior, lifespan, speed, hDelta, preferRightTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 0, color, node, behavior, lifespan, speed, hDelta, preferLeftTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 1, color, node, behavior, lifespan, speed, hDelta, preferRightTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 1, color, node, behavior, lifespan, speed, hDelta, preferLeftTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 2, color, node, behavior, lifespan, speed, hDelta, preferRightTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 2, color, node, behavior, lifespan, speed, hDelta, preferLeftTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 3, color, node, behavior, lifespan, speed, hDelta, preferRightTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 3, color, node, behavior, lifespan, speed, hDelta, preferLeftTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 4, color, node, behavior, lifespan, speed, hDelta, preferRightTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 4, color, node, behavior, lifespan, speed, hDelta, preferLeftTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 5, color, node, behavior, lifespan, speed, hDelta, preferRightTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 5, color, node, behavior, lifespan, speed, hDelta, preferLeftTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
  }
  else
  { /* fire only in one direction */
    rippleFired |= FireRipple(&currentRipple, dir, color, node, behavior, lifespan, speed, hDelta, preferRightTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, dir, color, node, behavior, lifespan, speed, hDelta, preferLeftTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
  }
  *firstRipple = currentRipple;
  return rippleFired;
};


/* FireShard
    Fires two ripples: one biased towards Right and one biased towards left in the next direction
    only makes sense for starburst nodes and quad nodes!
    dependencies: FireRipple
*/
bool FireShard(int *firstRipple, int dir, int color, int node, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, unsigned short nodeLimit)
{
  int currentRipple;
  bool rippleFired = 0;
  currentRipple = *firstRipple;
  if (dir < 0)
  { /* fire in all directions */
    rippleFired |= FireRipple(&currentRipple, 0, color, node, behavior, lifespan, speed, hDelta, preferRightTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 0, color, node, behavior, lifespan, speed, hDelta, preferLeftTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 1, color, node, behavior, lifespan, speed, hDelta, preferRightTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 1, color, node, behavior, lifespan, speed, hDelta, preferLeftTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 2, color, node, behavior, lifespan, speed, hDelta, preferRightTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 2, color, node, behavior, lifespan, speed, hDelta, preferLeftTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 3, color, node, behavior, lifespan, speed, hDelta, preferRightTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 3, color, node, behavior, lifespan, speed, hDelta, preferLeftTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 4, color, node, behavior, lifespan, speed, hDelta, preferRightTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 4, color, node, behavior, lifespan, speed, hDelta, preferLeftTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 5, color, node, behavior, lifespan, speed, hDelta, preferRightTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, 5, color, node, behavior, lifespan, speed, hDelta, preferLeftTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
  }
  else
  { /* fire only in one direction */
    rippleFired |= FireRipple(&currentRipple, dir, color, node, behavior, lifespan, speed, hDelta, preferRightTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
    rippleFired |= FireRipple(&currentRipple, ((dir+1)%6), color, node, behavior, lifespan, speed, hDelta, preferLeftTwice, nodeLimit);
    currentRipple = (currentRipple) % GlobalParameters.currentNumberofRipples;
  }
  *firstRipple = currentRipple;
  return rippleFired;
}

