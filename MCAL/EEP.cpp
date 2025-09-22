#include "EEP.h"
#include "HTTP_Server.h"
#include "ripple.h"
#include "Preferences.h"

Preferences preferencesObject;

#define RW_MODE false
#define RO_MODE true

static bool EEPROM_Initialized = false;

void EEPROM_Init(void)
{
  if(EEPROM_Initialized == false)
  {
    preferencesObject.begin("chromance", RO_MODE); // Open our namespace (or create it
                                         //  if it doesn't exist) in RO mode.
  
    bool tpInit = preferencesObject.isKey("nvsInit"); // Test for the existence
                                             // of the "already initialized" key.
  
    if (tpInit == false)
    {
      udp_printf("First-time run detected. Initializing EEPROM with factory default values.");
      // If tpInit is 'false', the key "nvsInit" does not yet exist therefore this
      //  must be our first-time run. We need to set up our Preferences namespace keys. So...
      preferencesObject.end();                      // close the namespace in RO mode and...
      preferencesObject.begin("chromance", RW_MODE); //  reopen it in RW mode.
  
      // Store the factory default values into the namespace
      setupDefaultProfileParameters(); // Set up the default profile parameters in the GlobalParameters run-time structure.
      preferencesObject.putBytes("GlobalParameters", &GlobalParameters, sizeof(GlobalParameters));
      // The "factory defaults" are created and stored so...
      preferencesObject.end();                      // Close the namespace in RW mode and...
      preferencesObject.begin("chromance", RO_MODE); //  reopen it in RO mode so the setup code
                                           //  outside this first-time run 'if' block
                                           //  can retrieve the run-time values
                                           //  from the "chromance" namespace.
    }else
    {
      udp_printf("Not first-time run. Retrieving stored values from EEPROM.");
      // This is not our first-time run so just retrieve the stored values from
      //  the "preferencesObject" namespace into the run-time variables.
      preferencesObject.getBytes("GlobalParameters", &GlobalParameters, sizeof(GlobalParameters));
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
    udp_printf("EEP_ReadGlobalParameters: EEPROM not initialized. Call EEPROM_Init() first.");
    return;
  }
  preferencesObject.begin("chromance", RO_MODE); // Open our namespace in RO mode.
  preferencesObject.getBytes("GlobalParameters", &GlobalParameters, sizeof(GlobalParameters));
  preferencesObject.end(); // Close our preferences namespace.
  return;
}

void EEPROM_StoreGlobalParameters(void)
{
  if(EEPROM_Initialized == false)
  {
    udp_printf("EEP_StoreGlobalParameters: EEPROM not initialized. Call EEPROM_Init() first.");
    return;
  }
  preferencesObject.begin("chromance", RW_MODE); // Open our namespace in RW mode.
  preferencesObject.putBytes("GlobalParameters", &GlobalParameters, sizeof(GlobalParameters));
  preferencesObject.end(); // Close our preferences namespace.
  return;
}
