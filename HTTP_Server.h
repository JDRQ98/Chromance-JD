#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <WiFi.h>

/* Variables used for control over web server */
typedef struct {
  boolean loop_MasterFireRippleEnabled;
  boolean loop_CenterFireRippleEnabled;
  boolean loop_CubeFireRippleEnabled;
  boolean loop_QuadFireRippleEnabled;
  boolean loop_BorderFireRippleEnabled;
  boolean loop_RandomEffectEnabled;
  unsigned char currentNumberofRipples;
  unsigned char currentNumberofColors;
  unsigned char currentBehavior;
  signed char currentDirection;
  short currentDelayBetweenRipples;
  short currentRainbowDeltaPerTick; /* units: hue */
  unsigned long currentRippleLifeSpan;
  float currentRippleSpeed;
  float currentDecay;
} GlobalParameters_struct;


extern GlobalParameters_struct GlobalParameters;

extern boolean manualFireRipple;

void WiFi_MainFunction(void);
void WiFi_init(void);

#endif /* HTTP_SERVER_H */
