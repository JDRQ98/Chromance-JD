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
      setupDefaultProfileParameters(&GlobalParameters.RippleProfiles[0]); // Set up the default profile parameters in the GlobalParameters run-time structure.
      GlobalParameters.MasterFireRippleEnabled = 1; // Enable master automatic ripples by default
      GlobalParameters.Decay = DECAY_DEFAULT; // Set default decay value
      GlobalParameters.NumberOfActiveProfiles = 1; // Start with one active profile by default
      strncpy(GlobalParameters.RippleProfiles[0].ProfileName, "Rainbow 7", MAX_PROFILE_NAME_LEN);
#if EEPROM_DEBUGGING
      udp_printf("Default global parameters setup by EEPROM_INIT");
      udp_printf(" - MasterFireRippleEnabled: %d", GlobalParameters.MasterFireRippleEnabled);
      udp_printf(" - NumberOfActiveProfiles: %d", GlobalParameters.NumberOfActiveProfiles);
      udp_printf(" - Profile 0 Name: \"%s\"", GlobalParameters.RippleProfiles[0].ProfileName);
      udp_printf(" - Profile 0 Active: %d", GlobalParameters.RippleProfiles[0].Active);
      udp_printf(" - Profile 0 DelayBetweenRipples_ms: %d", GlobalParameters.RippleProfiles[0].DelayBetweenRipples_ms);
      udp_printf(" - Profile 0 RippleLifeSpan: %d", GlobalParameters.RippleProfiles[0].RippleLifeSpan);
      udp_printf(" - Profile 0 RippleSpeed: %f", GlobalParameters.RippleProfiles[0].RippleSpeed);
      udp_printf(" - Profile 0 NumberOfColors: %d", GlobalParameters.RippleProfiles[0].NumberOfColors);
      udp_printf(" - Profile 0 CurrentColor: %d", GlobalParameters.RippleProfiles[0].CurrentColor);
      udp_printf(" - Profile 0 Behavior: %d", GlobalParameters.RippleProfiles[0].Behavior);
      udp_printf(" - Profile 0 RainbowDeltaPerTick: %d", GlobalParameters.RippleProfiles[0].RainbowDeltaPerTick);
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
      udp_printf(" - Profile 0 DelayBetweenRipples_ms: %d", GlobalParameters.RippleProfiles[0].DelayBetweenRipples_ms);
      udp_printf(" - Profile 0 RippleLifeSpan: %d", GlobalParameters.RippleProfiles[0].RippleLifeSpan);
      udp_printf(" - Profile 0 RippleSpeed: %f", GlobalParameters.RippleProfiles[0].RippleSpeed);
      udp_printf(" - Profile 0 NumberOfColors: %d", GlobalParameters.RippleProfiles[0].NumberOfColors);
      udp_printf(" - Profile 0 CurrentColor: %d", GlobalParameters.RippleProfiles[0].CurrentColor);
      udp_printf(" - Profile 0 Behavior: %d", GlobalParameters.RippleProfiles[0].Behavior);
      udp_printf(" - Profile 0 RainbowDeltaPerTick: %d", GlobalParameters.RippleProfiles[0].RainbowDeltaPerTick);
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
