#include "ASW.h"

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
