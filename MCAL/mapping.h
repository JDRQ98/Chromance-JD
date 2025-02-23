/*
 * Maps hex topology onto LED's
 * (C) Voidstar Lab LLC 2021
 */

#ifndef MAPPING_H_
#define MAPPING_H_

#define NUMBER_OF_STRIPS 2
#define NUMBER_OF_SEGMENTS 30
#define NUMBER_OF_NODES 19
#define NUMBER_OF_LEDS_PER_SEGMENT 11


//byte ledColors[NUMBER_OF_SEGMENTS][NUMBER_OF_LEDS_PER_SEGMENT][3]; /* used for RGB-based control */
extern int ledHues[NUMBER_OF_SEGMENTS][NUMBER_OF_LEDS_PER_SEGMENT][2]; /* used for hue-based control */

#define beginningof(S) ((S) * NUMBER_OF_LEDS_PER_SEGMENT)
#define endof(S) (beginningof(S) + (NUMBER_OF_LEDS_PER_SEGMENT-1))

#define beginningof_2ndstrip(S) ((S-15) * NUMBER_OF_LEDS_PER_SEGMENT)
#define endof_2ndstrip(S) (beginningof_2ndstrip(S) + (NUMBER_OF_LEDS_PER_SEGMENT-1))

/* public variables */
extern int nodeConnections[NUMBER_OF_NODES][6];
extern int segmentConnections[NUMBER_OF_SEGMENTS][2];
extern int ledAssignments[NUMBER_OF_SEGMENTS][3];

extern int numberOfBorderNodes;
extern int borderNodes[6];
extern int numberOfQuadNodes;
extern int QuadNodes[6];
extern int numberOfCubePairNodes;
extern int cubePairNodes[3];
extern int numberOfCubeOddNodes;
extern int cubeOddNodes[3];
extern int starburstNode;
extern int numberOfPerimeterSegments;
extern int numberOfPerimeterLEDs;
extern int perimeterSegments[12U];

#endif
