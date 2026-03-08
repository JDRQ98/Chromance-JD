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

      // Profile 0: "Rainbow 7" — rainbow colors from center node
      setupDefaultProfileParameters(&GlobalParameters.RippleProfiles[0]);
      strncpy(GlobalParameters.RippleProfiles[0].ProfileName, "Rainbow 7", MAX_PROFILE_NAME_LEN);
      GlobalParameters.RippleProfiles[0].ProfilePeriod_ms = 3000;
      // Events[0] already set by setupDefaultProfileParameters (center node, feisty, speed 0.5, rainbow delta 200)

      // Profile 1: "Ocean Wave" — blues/cyans, slow, border nodes
      setupDefaultProfileParameters(&GlobalParameters.RippleProfiles[1]);
      strncpy(GlobalParameters.RippleProfiles[1].ProfileName, "Ocean Wave", MAX_PROFILE_NAME_LEN);
      GlobalParameters.RippleProfiles[1].ProfilePeriod_ms = 4000;
      GlobalParameters.RippleProfiles[1].NumberOfColors = 4;
      GlobalParameters.RippleProfiles[1].Colors[0] = 43690; /* blue (hue 240°) */
      GlobalParameters.RippleProfiles[1].Colors[1] = 38229; /* cyan-blue (hue 210°) */
      GlobalParameters.RippleProfiles[1].Colors[2] = 32768; /* cyan (hue 180°) */
      GlobalParameters.RippleProfiles[1].Colors[3] = 36044; /* teal (hue ~198°) */
      // Update event 0 nodes and settings
      memset(GlobalParameters.RippleProfiles[1].Events[0].ActiveNodes, 0, sizeof(GlobalParameters.RippleProfiles[1].Events[0].ActiveNodes));
      for(int i = 0; i < numberOfBorderNodes; i++) GlobalParameters.RippleProfiles[1].Events[0].ActiveNodes[borderNodes[i]] = 1;
      GlobalParameters.RippleProfiles[1].Events[0].RippleSpeed = 0.3;
      GlobalParameters.RippleProfiles[1].Events[0].RippleLifeSpan = 4000;
      GlobalParameters.RippleProfiles[1].Events[0].Behavior = weaksauce;
      GlobalParameters.RippleProfiles[1].Events[0].RainbowDeltaPerTick = 0;

      // Profile 2: "Fire Storm" — reds/oranges/yellows, fast, center node
      setupDefaultProfileParameters(&GlobalParameters.RippleProfiles[2]);
      strncpy(GlobalParameters.RippleProfiles[2].ProfileName, "Fire Storm", MAX_PROFILE_NAME_LEN);
      GlobalParameters.RippleProfiles[2].ProfilePeriod_ms = 1500;
      GlobalParameters.RippleProfiles[2].NumberOfColors = 4;
      GlobalParameters.RippleProfiles[2].Colors[0] = 0;     /* red (hue 0°) */
      GlobalParameters.RippleProfiles[2].Colors[1] = 3640;  /* orange-red (hue ~20°) */
      GlobalParameters.RippleProfiles[2].Colors[2] = 5461;  /* orange (hue ~30°) */
      GlobalParameters.RippleProfiles[2].Colors[3] = 9102;  /* yellow-orange (hue ~50°) */
      // Event 0 already has center node from setupDefaultProfileParameters
      GlobalParameters.RippleProfiles[2].Events[0].RippleSpeed = 1.5;
      GlobalParameters.RippleProfiles[2].Events[0].RippleLifeSpan = 2000;
      GlobalParameters.RippleProfiles[2].Events[0].Behavior = angry;
      GlobalParameters.RippleProfiles[2].Events[0].RainbowDeltaPerTick = 0;

      // Profile 3: "Forest" — greens/yellows, medium speed, quad nodes
      setupDefaultProfileParameters(&GlobalParameters.RippleProfiles[3]);
      strncpy(GlobalParameters.RippleProfiles[3].ProfileName, "Forest", MAX_PROFILE_NAME_LEN);
      GlobalParameters.RippleProfiles[3].ProfilePeriod_ms = 2500;
      GlobalParameters.RippleProfiles[3].NumberOfColors = 4;
      GlobalParameters.RippleProfiles[3].Colors[0] = 21845; /* green (hue 120°) */
      GlobalParameters.RippleProfiles[3].Colors[1] = 18204; /* yellow-green (hue ~100°) */
      GlobalParameters.RippleProfiles[3].Colors[2] = 14563; /* chartreuse (hue ~80°) */
      GlobalParameters.RippleProfiles[3].Colors[3] = 10922; /* yellow (hue ~60°) */
      memset(GlobalParameters.RippleProfiles[3].Events[0].ActiveNodes, 0, sizeof(GlobalParameters.RippleProfiles[3].Events[0].ActiveNodes));
      for(int i = 0; i < numberOfQuadNodes; i++) GlobalParameters.RippleProfiles[3].Events[0].ActiveNodes[QuadNodes[i]] = 1;
      GlobalParameters.RippleProfiles[3].Events[0].RippleSpeed = 0.7;
      GlobalParameters.RippleProfiles[3].Events[0].RippleLifeSpan = 4000;
      GlobalParameters.RippleProfiles[3].Events[0].Behavior = feisty;
      GlobalParameters.RippleProfiles[3].Events[0].RainbowDeltaPerTick = 0;
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
