#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <WiFi.h>

/* Variables used for control over web server */
extern int loopFireRippleEnabled;
extern int manualFireRipple;
extern int currentNumberofRipples;
extern int currentNumberofColors;
extern short currentDelayBetweenRipples;
extern unsigned long currentRippleLifeSpan;
extern float currentDecay;
     
void WiFi_MainFunction(void);      
void WiFi_init(void);           

#endif /* HTTP_SERVER_H */
