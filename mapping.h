/*
 * Maps hex topology onto LED's
 * (C) Voidstar Lab LLC 2021
 */

#ifndef MAPPING_H_
#define MAPPING_H_

#define NUMBER_OF_STRIPS 1
#define NUMBER_OF_SEGMENTS 9

byte ledColors[NUMBER_OF_SEGMENTS][11][3];
short ledHues[NUMBER_OF_SEGMENTS][11][2];


// Beam 0 is at 12:00 and advance clockwise
// -1 means nothing connected on that side
int nodeConnections[7][6] = {
  {-1, -1, 2, -1, 3, -1},
  {-1, 3, 7, 4, -1, -1},
  {-1, -1, -1, 1, 8, 2},
  {-1, 8, -1, 6, -1, 7},
  {4, -1, 5, -1, -1, -1},
  {1, -1, -1, -1, 0, -1},
  {6, 0, -1, -1, -1, 5}
};

// First member: Node closer to ceiling
// Second: Node closer to floor
int segmentConnections[NUMBER_OF_SEGMENTS][2] = {
  {5, 6},
  {2, 5},
  {0, 2},
  {0, 1},
  {1, 4},
  {4, 6},
  {3, 6},
  {1, 3},
  {2, 3}
};

// First member: Strip number
// Second: LED index closer to ceiling
// Third: LED index closer to floor
int ledAssignments[NUMBER_OF_SEGMENTS][3] = {
  {0,10,0},
  {0,21,11},
  {0,32,22},
  {0,33,43},
  {0,44,54},
  {0,55,65},
  {0,77,66},
  {0,87,77},
  {0,98,88}
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
