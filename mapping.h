/*
 * Maps hex topology onto LED's
 * (C) Voidstar Lab LLC 2021
 */

#ifndef MAPPING_H_
#define MAPPING_H_

#define NUMBER_OF_STRIPS 2
#define NUMBER_OF_SEGMENTS 30
#define NUMBER_OF_NODES 19


//byte ledColors[NUMBER_OF_SEGMENTS][11][3]; /* used for RGB-based control */
extern short ledHues[NUMBER_OF_SEGMENTS][11][2]; /* used for hue-based control */

#define beginningof(S) ((S) * 11)
#define endof(S) (beginningof(S) + 10)

#define beginningof_2ndstrip(S) ((S-15) * 11)
#define endof_2ndstrip(S) (beginningof_2ndstrip(S) + 10)

/* public variables */
extern int nodeConnections[NUMBER_OF_NODES][6];
extern int segmentConnections[NUMBER_OF_SEGMENTS][2];
extern int ledAssignments[NUMBER_OF_SEGMENTS][3];

extern int numberOfBorderNodes;
extern int borderNodes[6];
extern int numberOfQuadNodes;
extern int QuadNodes[6];
extern int numberOfCubeNodes;
extern int cubeNodes[6];
extern int starburstNode;

#endif
