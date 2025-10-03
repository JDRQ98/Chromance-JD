#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <WiFi.h>
#include <ESPmDNS.h>
#include "WiFi_utilities.h"
#include "SPIFFS.h"
#include "MCAL/mapping.h"
#include "MCAL/ripple.h"

PROGMEM const char FAUXMO_TCP_STATE_RESPONSE[] = "["
    "{\"success\":{\"/lights/%d/state/on\":%s}},"
    "{\"success\":{\"/lights/%d/state/bri\":%d}}"   // not needed?
"]";

/* DEFINES for variable management */
#define NUMBER_OF_PROFILES 10U
#define MAX_PROFILE_NAME_LEN 32U

#define BEHAVIOR_DEFAULT feisty

#define DIRECTION_DEFAULT ALL_DIRECTIONS

#define DELAYBETWEENRIPPLES_MIN       1U
#define DELAYBETWEENRIPPLES_DEFAULT   3000U
#define DELAYBETWEENRIPPLES_MAX       20000U

#define RIPPLELIFESPAN_MIN       1U
#define RIPPLELIFESPAN_DEFAULT   3000U
#define RIPPLELIFESPAN_MAX       20000U

#define RIPPLESPEED_MIN       0.01
#define RIPPLESPEED_DEFAULT   0.5
#define RIPPLESPEED_MAX       10

#define DECAY_MIN       0.9
#define DECAY_DEFAULT   0.985
#define DECAY_MAX       1

#define COLOR_DEFAULT           0xFF0000

#define NUMBEROFCOLORS_MIN            1U
#define NUMBEROFCOLORS_DEFAULT        7U
#define NUMBEROFCOLORS_MAX            16U

#define RAINBOWDELTAPERTICK_MIN       0U
#define RAINBOWDELTAPERTICK_DEFAULT   200U
#define RAINBOWDELTAPERTICK_MAX       2000U

#define RAINBOWDELTAPERPERIOD_MIN       0U
#define RAINBOWDELTAPERPERIOD_DEFAULT   0U
#define RAINBOWDELTAPERPERIOD_MAX       60000U


typedef struct {
  boolean Active; /* is this profile active? */
  char ProfileName[MAX_PROFILE_NAME_LEN]; /* name of this profile */
  boolean ActiveNodes[NUMBER_OF_NODES]; /* for each node, is this profile active on that node? */
  rippleBehavior Behavior;
  signed char Direction;
  unsigned long RippleLifeSpan;
  float RippleSpeed;
  short RainbowDeltaPerTick; /* units: hue */
  unsigned int Colors[16]; /* array to store up to 16 colors per profile */
  unsigned int NumberOfColors;
  unsigned int CurrentColor;
  short DelayBetweenRipples_ms;
  unsigned long TimeLastRippleFired_ms;
} RippleProfile_struct;


/* Variables used for control over web server */
typedef struct {
  boolean MasterFireRippleEnabled;
  float Decay; /* decay per tick, global for now TODO: make decay a ripple/profile property */
  RippleProfile_struct RippleProfiles[NUMBER_OF_PROFILES]; /* ripple profiles stored by ESP32 */
  unsigned int NumberOfActiveProfiles;
} GlobalParameters_struct;


extern GlobalParameters_struct GlobalParameters;

extern boolean manualFireRipple;

void HTTP_backend_init(void);

void setupDefaultProfileParameters(RippleProfile_struct* RippleProfile);

#endif /* HTTP_SERVER_H */
