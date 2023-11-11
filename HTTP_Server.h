#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <WiFi.h>
#include <ESPmDNS.h>
#include "HueBridge.h"

PROGMEM const char FAUXMO_TCP_STATE_RESPONSE[] = "["
    "{\"success\":{\"/lights/%d/state/on\":%s}},"
    "{\"success\":{\"/lights/%d/state/bri\":%d}}"   // not needed?
"]";

/* DEFINES for variable management */
#define HTTP_CURRENTNUMBEROFRIPPLES_MIN           1U
#define HTTP_CURRENTNUMBEROFRIPPLES_DEFAULT       100U
#define HTTP_CURRENTNUMBEROFRIPPLES_MAX           100U /* this define should match the MAX_NUMBER_OF_RIPPLES definition in ripple .h [backend limitation] */

#define HTTP_CURRENTNUMBEROFCOLORS_MIN            1U
#define HTTP_CURRENTNUMBEROFCOLORS_DEFAULT        7U
#define HTTP_CURRENTNUMBEROFCOLORS_MAX            25U

#define HTTP_CURRENTBEHAVIOR_DEFAULT feisty

#define HTTP_CURRENTDIRECTION_DEFAULT ALL_DIRECTIONS

#define HTTP_CURRENTDELAYBETWEENRIPPLES_MIN       1U
#define HTTP_CURRENTDELAYBETWEENRIPPLES_DEFAULT   3000U
#define HTTP_CURRENTDELAYBETWEENRIPPLES_MAX       20000U

#define HTTP_CURRENTRAINBOWDELTAPERTICK_MIN       0U
#define HTTP_CURRENTRAINBOWDELTAPERTICK_DEFAULT   200U
#define HTTP_CURRENTRAINBOWDELTAPERTICK_MAX       2000U

#define HTTP_CURRENTRIPPLELIFESPAN_MIN       1U
#define HTTP_CURRENTRIPPLELIFESPAN_DEFAULT   3000U
#define HTTP_CURRENTRIPPLELIFESPAN_MAX       20000U

#define HTTP_CURRENTRIPPLESPEED_MIN       0.01
#define HTTP_CURRENTRIPPLESPEED_DEFAULT   0.5
#define HTTP_CURRENTRIPPLESPEED_MAX       10

#define HTTP_CURRENTDECAY_MIN       0.9
#define HTTP_CURRENTDECAY_DEFAULT   0.985
#define HTTP_CURRENTDECAY_MAX       1




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

typedef enum {
  no_preset = 0,
  default_preset = 1,
  RainbowTrails = 2,
  LongTrails = 3,
} presetType;


extern HueBridge hueBridge; /* used for WebServer - belongs to hueBridge */
extern bool DelayPeriodActive; /* variable used for Ripple_KillAllRipples - belongs to ASW*/

extern GlobalParameters_struct GlobalParameters;

extern boolean manualFireRipple;

void WiFi_MainFunction(void);
void WiFi_init(void);

#endif /* HTTP_SERVER_H */
