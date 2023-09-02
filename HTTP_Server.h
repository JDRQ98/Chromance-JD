#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <WiFi.h>

/* Variables used for control over web server */
extern int loop_MasterFireRippleEnabled;
extern int loop_CenterFireRippleEnabled;
extern int loop_CubeFireRippleEnabled;
extern int loop_QuadFireRippleEnabled;
extern int loop_BorderFireRippleEnabled;
extern int loop_RandomEffectEnabled;
extern int manualFireRipple;
extern int currentNumberofRipples;
extern int currentNumberofColors;
extern int currentBehavior;
extern int currentDirection;
extern short currentDelayBetweenRipples;
extern short currentRainbowDeltaPerTick; /* units: hue */
extern unsigned long currentRippleLifeSpan;
extern float currentRippleSpeed;
extern float currentDecay;
     
void WiFi_MainFunction(void);      
void WiFi_init(void);           

#endif /* HTTP_SERVER_H */
