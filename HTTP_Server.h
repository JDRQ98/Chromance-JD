#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <WiFi.h>
#include <ESPmDNS.h>
#include "WiFi_utilities.h"
#include "LittleFS.h"
#include "MCAL/mapping.h"
#include "MCAL/ripple.h"

/* DEFINES for variable management */
#define NUMBER_OF_PROFILES 10U
#define NUMBER_OF_SC_PRESETS 8U
#define MAX_PROFILE_NAME_LEN 32U

#define BEHAVIOR_DEFAULT feisty

#define DIRECTION_DEFAULT ALL_DIRECTIONS

#define DELAYBETWEENRIPPLES_MIN       1U
#define DELAYBETWEENRIPPLES_DEFAULT   3000U
#define DELAYBETWEENRIPPLES_MAX       20000U

#define RIPPLELIFESPAN_MIN       1U
#define RIPPLELIFESPAN_DEFAULT   5000U
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

#define BRIGHTNESS_MIN       0U
#define BRIGHTNESS_DEFAULT   128U
#define BRIGHTNESS_MAX       255U

#define SEQUENCER_DWELL_MIN      10U
#define SEQUENCER_DWELL_DEFAULT  30U
#define SEQUENCER_DWELL_MAX      120U

#define SC_DWELL_MIN_S     0.5f
#define SC_DWELL_DEFAULT_S 30.0f
#define SC_DWELL_MAX_S     120.0f
#define SC_FPS_DEFAULT     12U
#define SC_FADE_MIN_MS     5U
#define SC_FADE_DEFAULT_MS 300U
#define SC_FADE_MAX_MS     2000U

#define BPM_MIN       40.0f
#define BPM_DEFAULT   120.0f
#define BPM_MAX       300.0f

#define MAX_EVENTS_PER_PROFILE 5U

#define PROFILEPERIOD_MIN       500U
#define PROFILEPERIOD_DEFAULT   5000U
#define PROFILEPERIOD_MAX       60000U

#define RIPPLETYPE_SINGLE  0U
#define RIPPLETYPE_DOUBLE  1U
#define RIPPLETYPE_SHARD   2U

typedef struct {
  boolean Enabled;                        /* Is this event active? */
  unsigned short TimeOffset_ms;           /* When to fire (ms from period start; t0 = 0) */
  unsigned long RippleLifeSpan;           /* Per-event lifespan (ms) */
  unsigned char RippleType;               /* 0=single, 1=double, 2=shard */
  rippleBehavior Behavior;                /* 0-4 */
  float RippleSpeed;                      /* LEDs per tick */
  short RainbowDeltaPerTick;              /* Hue delta per advance */
  signed char Direction;                  /* -1=all, 0-5 */
  boolean ActiveNodes[NUMBER_OF_NODES];   /* Per-event node selection (19 booleans) */
} TimeEvent_struct;

typedef struct {
  boolean Active; /* is this profile active? */
  char ProfileName[MAX_PROFILE_NAME_LEN]; /* name of this profile */
  unsigned int Colors[16]; /* array to store up to 16 colors per profile (shared across events) */
  unsigned int NumberOfColors;
  unsigned int CurrentColor; /* Runtime: index cycling through colors */
  unsigned long ProfilePeriod_ms; /* Total period duration */
  TimeEvent_struct Events[MAX_EVENTS_PER_PROFILE]; /* t0-t4 */
  unsigned long PeriodStartTime_ms; /* Runtime: when current period started */
  boolean EventFired[MAX_EVENTS_PER_PROFILE]; /* Runtime: tracks which events fired this period */
} RippleProfile_struct;


/* Variables used for control over web server */
typedef struct {
  boolean Active;
  char PresetName[32];
  unsigned int Hue;                         /* 16-bit HSV hue, applied when SCSeqCycleColors=true */
  boolean Segments[NUMBER_OF_SEGMENTS];     /* which of the 30 segments to illuminate */
} StableColorPreset_struct;

typedef struct {
  boolean MasterFireRippleEnabled;
  float Decay; /* decay per tick, global for now TODO: make decay a ripple/profile property */
  unsigned char Brightness; /* LED brightness 0-255 */
  RippleProfile_struct RippleProfiles[NUMBER_OF_PROFILES]; /* ripple profiles stored by ESP32 */
  unsigned int NumberOfActiveProfiles;
  boolean SequencerEnabled;              /* Is sequencer running? */
  unsigned char SequencerMode;           /* 0 = sequential, 1 = random */
  unsigned short SequencerDwellTime_s;   /* Seconds per profile (10-120) */
  unsigned char SequencerCurrentProfile; /* Which profile index is playing now */
  unsigned long SequencerLastSwitch_ms;  /* millis() when last switch happened (runtime) */
  float GlobalBPM;                       /* BPM for UI reference (default 120.0) */

  /* Stable Color Mode — gentle pulsing solid color, bypasses ripple engine */
  boolean StableColorMode;              /* true = stable color, false = ripple */
  unsigned int StableColorHue;          /* 16-bit HSV hue (0–65535) */
  unsigned char StableColorSat;         /* HSV saturation (0–255), default 255 */
  float PulseFrequency;                 /* Pulse Hz (0.1–5.0), default 0.3 */
  float PulseDepth;                     /* Pulse amplitude (0.0–1.0), default 0.4 */
  boolean StableColorSegments[NUMBER_OF_SEGMENTS]; /* Which of the 30 segments to illuminate */

  /* Stable Color Sequencer */
  boolean SCSeqEnabled;                     /* Is SC sequencer running? */
  unsigned char SCSeqMode;                  /* 0=sequential, 1=random */
  unsigned char SCSeqTimingMode;            /* 0=time, 1=beat, 2=fps */
  float SCSeqDwellTime_s;                   /* time mode: seconds per preset (0.5-120) */
  float SCSeqBeatsPerSwitch;                /* beat mode: beats per switch (0.25/0.5/1/2/4/8/16) */
  unsigned char SCSeqFPS;                   /* fps mode: frames per second (1-24) */
  boolean SCSeqCycleColors;                 /* true = apply preset Hue on switch */
  boolean SCSeqFadeEnabled;                 /* true = fade transition between presets */
  boolean SCSeqFadeOuter;                   /* true = fade the 12 outer-ring segments */
  boolean SCSeqFadeInner;                   /* true = fade the 18 inner segments */
  unsigned short SCSeqFadeDuration_ms;      /* per-half fade duration (50-2000 ms) */
  unsigned char SCSeqCurrentPreset;         /* which preset index is playing now */
  unsigned long SCSeqLastSwitch_ms;         /* runtime: millis() of last switch */
  unsigned char NumberOfSCPresets;          /* count of active presets (1-8) */
  /* runtime fade state (reset on boot) */
  float SCSeqFadeMultiplier;                /* 0.0-1.0 brightness scale during fade */
  unsigned char SCSeqFadePhase;             /* 0=none, 1=fading out, 2=fading in */
  unsigned long SCSeqFadePhaseStart_ms;     /* millis() when current fade phase started */
  unsigned char SCSeqFadeTargetPreset;      /* preset to switch to when fade-out completes */
  StableColorPreset_struct SCPresets[NUMBER_OF_SC_PRESETS];
} GlobalParameters_struct;


extern GlobalParameters_struct GlobalParameters;

extern boolean manualFireRipple;
extern int nextRipple;

/* FreeRTOS mutex that guards all reads/writes to GlobalParameters across the
   HTTP task (Core 0) and the main loop (Core 1).  Take before touching any
   field of GlobalParameters; give immediately after. */
extern SemaphoreHandle_t gParamsMutex;

/* When the HTTP handler needs to kill a profile's ripples it sets this to the
   profile index instead of calling Ripple_KillProfileRipples() directly.
   The main loop executes the kill at a safe point (never mid-Ripple_MainFunction). */
extern volatile int pendingKillProfileIndex;

void HTTP_backend_init(void);

void setupDefaultProfileParameters(RippleProfile_struct* RippleProfile);

#endif /* HTTP_SERVER_H */
