/*
 * Maps hex topology onto LED's
 * (C) Voidstar Lab LLC 2021
 */

#ifndef MAPPING_H_
#define MAPPING_H_

#define NUMBER_OF_STRIPS 2
#define NUMBER_OF_SEGMENTS 30
#define NUMBER_OF_NODES 19


byte ledColors[NUMBER_OF_SEGMENTS][11][3];
short ledHues[NUMBER_OF_SEGMENTS][11][2];

#define beginningof(S) ((S) * 11)
#define endof(S) (beginningof(S) + 10)

#define beginningof_2ndstrip(S) ((S-15) * 11)
#define endof_2ndstrip(S) (beginningof_2ndstrip(S) + 10)


// each 6-member array describes a node's connections. Begin with node closer to ceiling.
// Beam 0 is at 12:00 and advance clockwise, -1 means nothing connected on that side
int nodeConnections[NUMBER_OF_NODES][6] = {
  {-1, -1, 11, -1, 10, -1},     /* node 0 */
  {-1, 10, 9, 26, 25, -1},       /* node 1 */
  {-1, -1, 12, 29, 28, 11},     /* node 2 */
  {-1, 25, -1, 24, -1, -1},        /* node 3 */
  {-1, 28, -1, 8, -1, 9},       /* node 4 */
  {-1, -1, -1, 13, -1, 12},       /* node 5 */
  {26, -1, 19, -1, 18, -1},       /* node 6 */
  {29, -1, 3, -1, 4, -1},        /* node 7 */
  {24, 18, 23, 17, -1, -1},        /* node 8 */
  {8, 4, 5, 20, 27, 19},        /* node 9 */
  {13, -1, -1, 2, 14, 3},       /* node 10 */
  {-1, 27, -1, 22, -1, 23},       /* node 11 */
  {-1, 14, -1, 6, -1, 5},       /* node 12 */
  {17, -1, 16, -1, -1, -1},        /* node 13 */
  {20, -1, 7, -1, 21, -1},        /* node 14 */
  {2, -1, -1, -1, 1, -1},        /* node 15 */
  {22, 21, 15, -1, -1, 16},       /* node 16 */
  {6, 1, -1, -1, 0, 7},       /* node 17 */
  {-1, 0, -1, -1, -1, 15}         /* node 18 */
};

// First member: Node closer to ceiling
// Second: Node closer to floor
int segmentConnections[NUMBER_OF_SEGMENTS][2] = {
  {17, 18},
  {15, 17},
  {10, 15},
  {7, 10},
  {7, 9},
  {9, 12},
  {12, 17},
  {14, 17},
  {4, 9}, /* segment 8 */
  {1, 4},
  {0, 1},
  {0, 2},
  {2, 5},
  {5, 10}, /* segment 13 */
  {10, 12},
  {16, 18}, /* segment 15, start of green strip*/
  {13, 16},
  {8, 13},
  {6, 8},
  {6, 9}, /* segment 19*/
  {9, 14},
  {14, 16},
  {11, 16},
  {8, 11},
  {3, 8},
  {1, 3},
  {1, 6}, /* segment 26*/
  {9, 11},
  {2, 4},
  {2, 7}, /* segment 29*/
};

// First member: Strip number
// Second: LED index closer to ceiling
// Third: LED index closer to floor
int ledAssignments[NUMBER_OF_SEGMENTS][3] = {
  {0, endof(0), beginningof(0)},
  {0, endof(1), beginningof(1)},
  {0, endof(2), beginningof(2)},
  {0, endof(3), beginningof(3)},
  {0, beginningof(4), endof(4)},
  {0, beginningof(5), endof(5)},
  {0, beginningof(6), endof(6)},
  {0, endof(7), beginningof(7)},
  {0, endof(8), beginningof(8)},
  {0, endof(9), beginningof(9)},
  {0, endof(10), beginningof(10)},
  {0, beginningof(11), endof(11)},
  {0, beginningof(12), endof(12)},
  {0, beginningof(13), endof(13)},
  {0, beginningof(14), endof(14)},
  {1, endof_2ndstrip(15), beginningof_2ndstrip(15)},
  {1, endof_2ndstrip(16), beginningof_2ndstrip(16)},
  {1, endof_2ndstrip(17), beginningof_2ndstrip(17)},
  {1, endof_2ndstrip(18), beginningof_2ndstrip(18)},
  {1, beginningof_2ndstrip(19), endof_2ndstrip(19)},
  {1, beginningof_2ndstrip(20), endof_2ndstrip(20)},
  {1, beginningof_2ndstrip(21), endof_2ndstrip(21)},
  {1, endof_2ndstrip(22), beginningof_2ndstrip(22)},
  {1, endof_2ndstrip(23), beginningof_2ndstrip(23)},
  {1, endof_2ndstrip(24), beginningof_2ndstrip(24)},
  {1, endof_2ndstrip(25), beginningof_2ndstrip(25)},
  {1, beginningof_2ndstrip(26), endof_2ndstrip(26)},
  {1, beginningof_2ndstrip(27), endof_2ndstrip(27)},
  {1, endof_2ndstrip(28), beginningof_2ndstrip(28)},
  {1, beginningof_2ndstrip(29), endof_2ndstrip(29)}
};

// Border nodes are on the very edge of the network. These connect 2 segments together.
// Ripples fired here don't look very impressive.
int numberOfBorderNodes = 6;
int borderNodes[] = {0, 3, 5, 13, 15, 18};

// Quad nodes: connect 4 segments together
int numberOfQuadNodes = 6;
int QuadNodes[] = {1, 2, 8, 10, 16, 17};

// Cube nodes link three equiangular segments
// Firing ripples that always turn in one direction will draw a cube
int numberOfCubeNodes = 6;
int cubeNodes[] = {6, 7, 9, 11, 12, 14};

// Firing ripples that always turn in one direction will draw a starburst
int starburstNode = 9;

#endif
