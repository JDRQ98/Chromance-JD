#ifndef ASW_H_
#define ASW_H_

#include "MCAL/ripple.h"
#include "HTTP_Server.h"

#define NUMBER_OF_EFFECTS 4 /* for random effect function*/


extern bool OTAinProgress;

void onOTAStart();
void onOTAProgress(size_t current, size_t final);
void onOTAEnd(bool success);

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
