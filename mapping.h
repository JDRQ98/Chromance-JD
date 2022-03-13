/*
 * Maps hex topology onto LED's
 * (C) Voidstar Lab LLC 2021
 */

#ifndef MAPPING_H_
#define MAPPING_H_

byte ledColors[4][11][3];
short ledHues[4][11][2];

#define NUMBER_OF_STRIPS 2
#define NUMBER_OF_SEGMENTS 4

// Beam 0 is at 12:00 and advance clockwise
// -1 means nothing connected on that side
int nodeConnections[4][6] = {
  {-1, 3, -1, -1, -1, 0},
  {-1, 1, 0, -1, -1, -1},
  {-1, -1, 2, -1, 1, -1},
  {-1, -1, -1, -1, 3, 2},
};

// First member: Node closer to ceiling
// Second: Node closer to floor
int segmentConnections[4][2] = {
  {1, 0},
  {2, 1},
  {2, 3},
  {3, 0}
};

// First member: Strip number
// Second: LED index closer to ceiling
// Third: LED index closer to floor
int ledAssignments[4][3] = {
  {0,10,0},
  {0,21,11},
  {1,21,11},
  {1,10,0},
};

// Border nodes are on the very edge of the network.
// Ripples fired here don't look very impressive.
int numberOfBorderNodes = 10;
int borderNodes[] = {0, 1, 2, 3, 6, 10, 13, 19, 21, 24};

// Cube nodes link three equiangular segments
// Firing ripples that always turn in one direction will draw a cube
int numberOfCubeNodes = 8;
int cubeNodes[] = {7, 8, 9, 11, 12, 17, 18};

// Firing ripples that always turn in one direction will draw a starburst
int starburstNode = 15;

#endif
