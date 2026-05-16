#include "EEP.h"
#include "HTTP_Server.h"
#include "ripple.h"
#include "Preferences.h"

#define EEPROM_DEBUGGING true
#define EEPROM_KEY "GlobalConfig"

Preferences preferencesObject;

#define RW_MODE false
#define RO_MODE true

static bool EEPROM_Initialized = false;
static bool EEPROM_Dirty = false;
static unsigned long EEPROM_DirtyTimestamp = 0;

void EEPROM_Init(void)
{
  unsigned int bytesStored, bytesRetrieved = 0U;

  if(EEPROM_Initialized == false)
  {
    preferencesObject.begin("chromance", RO_MODE); // Open our namespace (or create it
                                         //  if it doesn't exist) in RO mode.
  
    bool tpInit = preferencesObject.isKey("nvsInit"); // Test for the existence
                                             // of the "already initialized" key.
  
    if (tpInit == false)
    {
#if EEPROM_DEBUGGING
      udp_printf("First-time run detected. Initializing EEPROM with factory default values.");
#endif
      // If tpInit is 'false', the key "nvsInit" does not yet exist therefore this
      //  must be our first-time run. We need to set up our Preferences namespace keys. So...
      preferencesObject.end();                      // close the namespace in RO mode and...
      preferencesObject.begin("chromance", RW_MODE); //  reopen it in RW mode.
  
      // Store the factory default values into the namespace
      GlobalParameters.MasterFireRippleEnabled = 1; // Enable master automatic ripples by default
      GlobalParameters.Decay = DECAY_DEFAULT; // Set default decay value
      GlobalParameters.Brightness = BRIGHTNESS_DEFAULT; // Set default brightness
      GlobalParameters.NumberOfActiveProfiles = 4; // Start with 4 preset profiles

      // Sequencer defaults
      GlobalParameters.SequencerEnabled = false;
      GlobalParameters.SequencerMode = 0; // sequential
      GlobalParameters.SequencerDwellTime_s = SEQUENCER_DWELL_DEFAULT;
      GlobalParameters.SequencerCurrentProfile = 0;
      GlobalParameters.SequencerLastSwitch_ms = 0;

      // BPM default (UI reference only)
      GlobalParameters.GlobalBPM = BPM_DEFAULT;

      // Stable color mode defaults
      GlobalParameters.StableColorMode = false;
      GlobalParameters.StableColorHue  = 43690;  /* blue ~240° */
      GlobalParameters.StableColorSat  = 255;
      GlobalParameters.PulseFrequency  = 0.3f;
      GlobalParameters.PulseDepth      = 0.4f;
      memset(GlobalParameters.StableColorSegments, true, NUMBER_OF_SEGMENTS);

      /* Stable color sequencer defaults — tuned so the |/ /\ rotating-line
         animation presets work as intended when the user enables the sequencer.
         Pulse-synced timing means each frame advances on the global pulse;
         fade with inner-segments-only at 250 ms gives the line a smooth
         rotation feel. SCSeqEnabled stays false so the device boots quietly. */
      GlobalParameters.SCSeqEnabled           = false;
      GlobalParameters.SCSeqMode              = 0; /* sequential */
      GlobalParameters.SCSeqTimingMode        = 1; /* pulse-synced */
      GlobalParameters.SCSeqDwellTime_s       = SC_DWELL_DEFAULT_S;
      GlobalParameters.SCSeqBeatsPerSwitch    = 1.0f;
      GlobalParameters.SCSeqFPS               = SC_FPS_DEFAULT;
      GlobalParameters.SCSeqCycleColors       = false; /* presets share cyan hue */
      GlobalParameters.SCSeqFadeEnabled       = true;
      GlobalParameters.SCSeqFadeOuter         = false;
      GlobalParameters.SCSeqFadeInner         = true;
      GlobalParameters.SCSeqFadeDuration_ms   = 250;
      GlobalParameters.SCSeqCurrentPreset     = 0;
      GlobalParameters.SCSeqLastSwitch_ms     = 0;
      GlobalParameters.NumberOfSCPresets      = 4;
      /* runtime fade state - zeroed */
      GlobalParameters.SCSeqFadeMultiplier    = 1.0f;
      GlobalParameters.SCSeqFadePhase         = 0;
      GlobalParameters.SCSeqFadePhaseStart_ms = 0;
      GlobalParameters.SCSeqFadeTargetPreset  = 0;

      /* Preset 0: "Solid" — all 30 segments, blue. Default "just be one color" preset. */
      GlobalParameters.SCPresets[0].Active = true;
      strncpy(GlobalParameters.SCPresets[0].PresetName, "Solid", 32);
      GlobalParameters.SCPresets[0].Hue = 43690; /* blue ~240° */
      memset(GlobalParameters.SCPresets[0].Segments, true, NUMBER_OF_SEGMENTS);

      /* Presets 1-3: rotating-line animation frames captured from the live device.
         Cycle through them with the SC sequencer (pulse-synced timing, fade enabled,
         fade-inner only, fade ~250 ms) for a barber-pole / rotating-beam effect. */

      /* Preset 1: "|" — vertical bar frame */
      GlobalParameters.SCPresets[1].Active = true;
      strncpy(GlobalParameters.SCPresets[1].PresetName, "|", 32);
      GlobalParameters.SCPresets[1].Hue = 32768; /* cyan ~180° */
      {
        const bool segs[NUMBER_OF_SEGMENTS] = {1,1,1,0,1,1,1,0,0,0,1,1,1,1,0,1,1,1,0,1,0,0,1,0,1,1,1,1,0,1};
        memcpy(GlobalParameters.SCPresets[1].Segments, segs, NUMBER_OF_SEGMENTS);
      }

      /* Preset 2: "/" — forward-slash frame */
      GlobalParameters.SCPresets[2].Active = true;
      strncpy(GlobalParameters.SCPresets[2].PresetName, "/", 32);
      GlobalParameters.SCPresets[2].Hue = 32768; /* cyan ~180° */
      {
        const bool segs[NUMBER_OF_SEGMENTS] = {1,1,1,1,1,0,0,1,1,1,1,1,1,1,0,1,1,1,0,0,1,0,0,1,1,1,0,1,0,0};
        memcpy(GlobalParameters.SCPresets[2].Segments, segs, NUMBER_OF_SEGMENTS);
      }

      /* Preset 3: "\" — backslash frame */
      GlobalParameters.SCPresets[3].Active = true;
      strncpy(GlobalParameters.SCPresets[3].PresetName, "\\", 32);
      GlobalParameters.SCPresets[3].Hue = 32768; /* cyan ~180° */
      {
        const bool segs[NUMBER_OF_SEGMENTS] = {1,1,1,0,0,1,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,0,0,1,0};
        memcpy(GlobalParameters.SCPresets[3].Segments, segs, NUMBER_OF_SEGMENTS);
      }

      /* Presets 4–7: inactive placeholders */
      for(int i = 4; i < NUMBER_OF_SC_PRESETS; i++){
        GlobalParameters.SCPresets[i].Active = false;
        strncpy(GlobalParameters.SCPresets[i].PresetName, "Preset", 32);
        GlobalParameters.SCPresets[i].Hue = 0;
        memset(GlobalParameters.SCPresets[i].Segments, false, NUMBER_OF_SEGMENTS);
      }

      // Profile 0: "Rainbow 7" — rainbow colors from center node
      setupDefaultProfileParameters(&GlobalParameters.RippleProfiles[0]);
      strncpy(GlobalParameters.RippleProfiles[0].ProfileName, "Rainbow 7", MAX_PROFILE_NAME_LEN);
      GlobalParameters.RippleProfiles[0].ProfilePeriod_ms = 3000;
      // Events[0] already set by setupDefaultProfileParameters (center node, feisty, speed 0.5, rainbow delta 200)

      /* Profile 1: "Cyclone" — counter-rotating spirals from cube nodes.
         Period = 4 s (two bars at 120 BPM). Two simultaneous events: pair-cube
         ripples spiral clockwise (alwaysTurnsRight), odd-cube ripples spiral
         counter-clockwise (alwaysTurnsLeft). Slow drift through cool palette. */
      setupDefaultProfileParameters(&GlobalParameters.RippleProfiles[1]);
      strncpy(GlobalParameters.RippleProfiles[1].ProfileName, "Cyclone", MAX_PROFILE_NAME_LEN);
      GlobalParameters.RippleProfiles[1].ProfilePeriod_ms = 4000;
      GlobalParameters.RippleProfiles[1].NumberOfColors = 5;
      GlobalParameters.RippleProfiles[1].Colors[0] = 43690; /* blue (240°) */
      GlobalParameters.RippleProfiles[1].Colors[1] = 32768; /* cyan (180°) */
      GlobalParameters.RippleProfiles[1].Colors[2] = 48497; /* indigo (~266°) */
      GlobalParameters.RippleProfiles[1].Colors[3] = 36044; /* teal (~198°) */
      GlobalParameters.RippleProfiles[1].Colors[4] = 51425; /* violet (~282°) */
      /* Event 0: pair-cube nodes spiral right (clockwise) */
      memset(GlobalParameters.RippleProfiles[1].Events[0].ActiveNodes, 0, sizeof(GlobalParameters.RippleProfiles[1].Events[0].ActiveNodes));
      for(int i = 0; i < numberOfCubePairNodes; i++) GlobalParameters.RippleProfiles[1].Events[0].ActiveNodes[cubePairNodes[i]] = 1;
      GlobalParameters.RippleProfiles[1].Events[0].TimeOffset_ms      = 0;
      GlobalParameters.RippleProfiles[1].Events[0].RippleType         = RIPPLETYPE_SINGLE;
      GlobalParameters.RippleProfiles[1].Events[0].Direction          = 0;
      GlobalParameters.RippleProfiles[1].Events[0].RippleSpeed        = 0.4f;
      GlobalParameters.RippleProfiles[1].Events[0].RippleLifeSpan     = 3500;
      GlobalParameters.RippleProfiles[1].Events[0].Behavior           = alwaysTurnsRight;
      GlobalParameters.RippleProfiles[1].Events[0].RainbowDeltaPerTick = 80;
      /* Event 1: odd-cube nodes spiral left (counter-clockwise) */
      GlobalParameters.RippleProfiles[1].Events[1].Enabled            = true;
      memset(GlobalParameters.RippleProfiles[1].Events[1].ActiveNodes, 0, sizeof(GlobalParameters.RippleProfiles[1].Events[1].ActiveNodes));
      for(int i = 0; i < numberOfCubeOddNodes; i++) GlobalParameters.RippleProfiles[1].Events[1].ActiveNodes[cubeOddNodes[i]] = 1;
      GlobalParameters.RippleProfiles[1].Events[1].TimeOffset_ms      = 0;
      GlobalParameters.RippleProfiles[1].Events[1].RippleType         = RIPPLETYPE_SINGLE;
      GlobalParameters.RippleProfiles[1].Events[1].Direction          = 3;
      GlobalParameters.RippleProfiles[1].Events[1].RippleSpeed        = 0.4f;
      GlobalParameters.RippleProfiles[1].Events[1].RippleLifeSpan     = 3500;
      GlobalParameters.RippleProfiles[1].Events[1].Behavior           = alwaysTurnsLeft;
      GlobalParameters.RippleProfiles[1].Events[1].RainbowDeltaPerTick = 80;

      /* Profile 2: "Bloom" — symmetric flower-burst from center.
         Period = 2 s (one bar at 120 BPM). Three DOUBLE ripples per period
         rotated 120° apart, creating an opening-flower pattern. */
      setupDefaultProfileParameters(&GlobalParameters.RippleProfiles[2]);
      strncpy(GlobalParameters.RippleProfiles[2].ProfileName, "Bloom", MAX_PROFILE_NAME_LEN);
      GlobalParameters.RippleProfiles[2].ProfilePeriod_ms = 2000;
      GlobalParameters.RippleProfiles[2].NumberOfColors = 4;
      GlobalParameters.RippleProfiles[2].Colors[0] = 60074; /* hot pink (~330°) */
      GlobalParameters.RippleProfiles[2].Colors[1] = 2912;  /* coral (~16°) */
      GlobalParameters.RippleProfiles[2].Colors[2] = 54613; /* magenta (~300°) */
      GlobalParameters.RippleProfiles[2].Colors[3] = 0;     /* red (0°) */
      /* All three events share center node, DOUBLE ripple, feisty behavior, fast-ish */
      const unsigned char bloomDirs[3] = {0, 2, 4};
      const unsigned short bloomOffsets[3] = {0, 666, 1333};
      for(int e = 0; e < 3; e++){
        TimeEvent_struct* ev = &GlobalParameters.RippleProfiles[2].Events[e];
        ev->Enabled            = true;
        ev->TimeOffset_ms      = bloomOffsets[e];
        ev->RippleLifeSpan     = 1500;
        ev->RippleType         = RIPPLETYPE_DOUBLE;
        ev->Behavior           = feisty;
        ev->RippleSpeed        = 0.7f;
        ev->RainbowDeltaPerTick = 0;
        ev->Direction          = bloomDirs[e];
        memset(ev->ActiveNodes, 0, sizeof(ev->ActiveNodes));
        ev->ActiveNodes[starburstNode] = 1;
      }

      /* Profile 3: "Heartbeat" — lub-DUB pulse from the perimeter.
         Period = 2 s (one heartbeat per bar). Two SHARD bursts close together
         followed by a long quiet gap so the residual decay reads as the rest. */
      setupDefaultProfileParameters(&GlobalParameters.RippleProfiles[3]);
      strncpy(GlobalParameters.RippleProfiles[3].ProfileName, "Heartbeat", MAX_PROFILE_NAME_LEN);
      GlobalParameters.RippleProfiles[3].ProfilePeriod_ms = 2000;
      GlobalParameters.RippleProfiles[3].NumberOfColors = 4;
      GlobalParameters.RippleProfiles[3].Colors[0] = 0;     /* red (0°) */
      GlobalParameters.RippleProfiles[3].Colors[1] = 63715; /* crimson (~350°) */
      GlobalParameters.RippleProfiles[3].Colors[2] = 60074; /* magenta-red (~330°) */
      GlobalParameters.RippleProfiles[3].Colors[3] = 3640;  /* orange-red (~20°) */
      /* Event 0: "lub" — short shard burst from border nodes */
      memset(GlobalParameters.RippleProfiles[3].Events[0].ActiveNodes, 0, sizeof(GlobalParameters.RippleProfiles[3].Events[0].ActiveNodes));
      for(int i = 0; i < numberOfBorderNodes; i++) GlobalParameters.RippleProfiles[3].Events[0].ActiveNodes[borderNodes[i]] = 1;
      GlobalParameters.RippleProfiles[3].Events[0].TimeOffset_ms      = 0;
      GlobalParameters.RippleProfiles[3].Events[0].RippleType         = RIPPLETYPE_SHARD;
      GlobalParameters.RippleProfiles[3].Events[0].Direction          = ALL_DIRECTIONS;
      GlobalParameters.RippleProfiles[3].Events[0].RippleSpeed        = 1.2f;
      GlobalParameters.RippleProfiles[3].Events[0].RippleLifeSpan     = 600;
      GlobalParameters.RippleProfiles[3].Events[0].Behavior           = weaksauce;
      GlobalParameters.RippleProfiles[3].Events[0].RainbowDeltaPerTick = 0;
      /* Event 1: "DUB" — slightly larger shard 250ms later */
      GlobalParameters.RippleProfiles[3].Events[1].Enabled            = true;
      memset(GlobalParameters.RippleProfiles[3].Events[1].ActiveNodes, 0, sizeof(GlobalParameters.RippleProfiles[3].Events[1].ActiveNodes));
      for(int i = 0; i < numberOfBorderNodes; i++) GlobalParameters.RippleProfiles[3].Events[1].ActiveNodes[borderNodes[i]] = 1;
      GlobalParameters.RippleProfiles[3].Events[1].TimeOffset_ms      = 250;
      GlobalParameters.RippleProfiles[3].Events[1].RippleType         = RIPPLETYPE_SHARD;
      GlobalParameters.RippleProfiles[3].Events[1].Direction          = ALL_DIRECTIONS;
      GlobalParameters.RippleProfiles[3].Events[1].RippleSpeed        = 1.4f;
      GlobalParameters.RippleProfiles[3].Events[1].RippleLifeSpan     = 800;
      GlobalParameters.RippleProfiles[3].Events[1].Behavior           = weaksauce;
      GlobalParameters.RippleProfiles[3].Events[1].RainbowDeltaPerTick = 0;

      // Only profile 0 ("Rainbow 7") is active by default; others exist but are inactive
      GlobalParameters.RippleProfiles[1].Active = 0;
      GlobalParameters.RippleProfiles[2].Active = 0;
      GlobalParameters.RippleProfiles[3].Active = 0;

#if EEPROM_DEBUGGING
      udp_printf("Default global parameters setup by EEPROM_INIT");
      udp_printf(" - MasterFireRippleEnabled: %d", GlobalParameters.MasterFireRippleEnabled);
      udp_printf(" - NumberOfActiveProfiles: %d", GlobalParameters.NumberOfActiveProfiles);
      udp_printf(" - SequencerEnabled: %d, Mode: %d, Dwell: %ds", GlobalParameters.SequencerEnabled, GlobalParameters.SequencerMode, GlobalParameters.SequencerDwellTime_s);
      for(int p = 0; p < GlobalParameters.NumberOfActiveProfiles; p++){
        udp_printf(" - Profile %d Name: \"%s\", Active: %d, Colors: %d, Period: %lu ms", p,
          GlobalParameters.RippleProfiles[p].ProfileName,
          GlobalParameters.RippleProfiles[p].Active,
          GlobalParameters.RippleProfiles[p].NumberOfColors,
          GlobalParameters.RippleProfiles[p].ProfilePeriod_ms);
      }
#endif
      // Store the run-time variables into the Preferences namespace
      
      bytesStored = preferencesObject.putBytes(EEPROM_KEY, &GlobalParameters, sizeof(GlobalParameters));
#if EEPROM_DEBUGGING
      udp_printf("GlobalParameters stored %u bytes", bytesStored);
#endif
      if(bytesStored != sizeof(GlobalParameters))
      {
#if EEPROM_DEBUGGING
        udp_printf("WARNING: GlobalParameters not fully stored into EEPROM! Expected %u bytes, actually stored %u bytes.", sizeof(GlobalParameters), bytesStored);
#endif
      }
      // The "factory defaults" are created and stored so...
      preferencesObject.putBool("nvsInit", true); //  create the "nvsInit" key and set it to 'true'
      preferencesObject.end();                      // Close the namespace in RW mode and...
      preferencesObject.begin("chromance", RO_MODE); //  reopen it in RO mode so the setup code
                                           //  outside this first-time run 'if' block
                                           //  can retrieve the run-time values
                                           //  from the "chromance" namespace.
    }else
    {
#if EEPROM_DEBUGGING
      udp_printf("Not first-time run. Retrieving stored values from EEPROM.");
#endif
      // This is not our first-time run so just retrieve the stored values from
      //  the "preferencesObject" namespace into the run-time variables.
      bytesRetrieved = preferencesObject.getBytes(EEPROM_KEY, &GlobalParameters, sizeof(GlobalParameters));
#if EEPROM_DEBUGGING
      udp_printf("GlobalParameters retrieved %u bytes", bytesRetrieved);
#endif
      if(bytesRetrieved != sizeof(GlobalParameters))
      {
#if EEPROM_DEBUGGING
        udp_printf("WARNING: GlobalParameters not fully retrieved from EEPROM! Expected %u bytes, actually retrieved %u bytes.", sizeof(GlobalParameters), bytesRetrieved);
        udp_printf("This is likely due to a struct change. Forcing re-initialization.");
#endif
        preferencesObject.end();
        EEPROM_Clear();
        ESP.restart(); // Restart to re-run EEPROM_Init with a clean slate.
      }
#if EEPROM_DEBUGGING
      udp_printf("Global parameters retrieved by EEPROM_INIT:");
      udp_printf(" - MasterFireRippleEnabled: %d", GlobalParameters.MasterFireRippleEnabled);
      udp_printf(" - NumberOfActiveProfiles: %d", GlobalParameters.NumberOfActiveProfiles);
      udp_printf(" - Profile 0 Name: \"%s\"", GlobalParameters.RippleProfiles[0].ProfileName);
      udp_printf(" - Profile 0 Active: %d", GlobalParameters.RippleProfiles[0].Active);
      udp_printf(" - Profile 0 ProfilePeriod_ms: %lu", GlobalParameters.RippleProfiles[0].ProfilePeriod_ms);
      udp_printf(" - Profile 0 NumberOfColors: %d", GlobalParameters.RippleProfiles[0].NumberOfColors);
      udp_printf(" - Profile 0 CurrentColor: %d", GlobalParameters.RippleProfiles[0].CurrentColor);
      udp_printf(" - Profile 0 Event[0] Enabled: %d, Type: %d, Speed: %f",
        GlobalParameters.RippleProfiles[0].Events[0].Enabled,
        GlobalParameters.RippleProfiles[0].Events[0].RippleType,
        GlobalParameters.RippleProfiles[0].Events[0].RippleSpeed);
#endif
    }
  
    // Retrieve the operational parameters from the namespace
    //  and save them into their run-time variables.
  
    // All done. Last run state (or the factory default) is now restored.
    preferencesObject.end(); // Close our preferences namespace.
  }
  EEPROM_Initialized = true;
  return;
}

void EEPROM_ReadGlobalParameters(void)
{
  if(EEPROM_Initialized == false)
  {
#if EEPROM_DEBUGGING
    udp_printf("EEP_ReadGlobalParameters: EEPROM not initialized. Call EEPROM_Init() first.");
#endif
    return;
  }
  preferencesObject.begin("chromance", RO_MODE); // Open our namespace in RO mode.
  preferencesObject.getBytes(EEPROM_KEY, &GlobalParameters, sizeof(GlobalParameters));
  preferencesObject.end(); // Close our preferences namespace.
  return;
}

void EEPROM_StoreGlobalParameters(void)
{
  if(EEPROM_Initialized == false)
  {
#if EEPROM_DEBUGGING
    udp_printf("EEP_StoreGlobalParameters: EEPROM not initialized. Call EEPROM_Init() first.");
#endif
    return;
  }
  preferencesObject.begin("chromance", RW_MODE); // Open our namespace in RW mode.
  preferencesObject.putBytes(EEPROM_KEY, &GlobalParameters, sizeof(GlobalParameters));
  preferencesObject.end(); // Close our preferences namespace.
  return;
}

void EEPROM_MarkDirty(void)
{
  EEPROM_Dirty = true;
  EEPROM_DirtyTimestamp = millis();
#if EEPROM_DEBUGGING
  udp_printf("EEPROM marked dirty, will save after %u ms cooldown", EEPROM_SAVE_COOLDOWN_MS);
#endif
}

void EEPROM_DebouncedSave(void)
{
  if (EEPROM_Dirty && (millis() - EEPROM_DirtyTimestamp >= EEPROM_SAVE_COOLDOWN_MS))
  {
    EEPROM_Dirty = false;
    EEPROM_StoreGlobalParameters();
#if EEPROM_DEBUGGING
    udp_printf("EEPROM debounced save completed");
#endif
  }
}

void EEPROM_Clear(void)
{
  preferencesObject.begin("chromance", RW_MODE); // Open our namespace in RW mode.
  preferencesObject.clear();                      // Clear all keys in the namespace.
  preferencesObject.end();                        // Close our preferences namespace.
  EEPROM_Initialized = false;                     // Force re-initialization on next call to EEPROM_Init()
  return;
}
