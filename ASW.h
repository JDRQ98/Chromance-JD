#ifndef ASW_H_
#define ASW_H_

#include "ripple.h"
#include "HTTP_Server.h"
#include "mapping.h"

#define NUMBER_OF_EFFECTS 4 /* for random effect function*/

/* bulk fire ripples */
bool FireRipple_CenterNode(int* firstRipple, int dir, int color, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, directionBias bias, unsigned short nodeLimit);
bool FireRipple_AllBorderNodes(int* firstRipple, int dir, int color, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, directionBias bias, unsigned short nodeLimit);
bool FireRipple_AllQuadNodes(int* firstRipple, int dir, int color, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, directionBias bias, unsigned short nodeLimit);
bool FireRipple_AllCubeNodes(int* firstRipple, int dir, int color, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, directionBias bias, unsigned short nodeLimit);
bool FireRipple_AllPairCubeNodes(int* firstRipple, int dir, int color, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, directionBias bias, unsigned short nodeLimit);
bool FireRipple_AllOddCubeNodes(int* firstRipple, int dir, int color, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, directionBias bias, unsigned short nodeLimit);
/* special effects */
bool FireEffect_Star(int* firstRipple, int color, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta);
bool FireEffect_CenterNode_QuadShard(int *firstRipple, int dir, int color, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, unsigned short nodeLimit);
bool FireEffect_Random(int* firstRipple, int color, byte behavior, unsigned long lifespan, float speed, unsigned short hDelta, unsigned short nodeLimit);


#endif
