#include "EEP.h"
#include "HTTP_Server.h"
#include "ripple.h"

static unsigned int EEP_ProfileSize = 0;
unsigned int Global_NumberOfProfiles_InDFLS = 0;
boolean ProfilesAvailable[EEPROM_SUPPORTED_PROFILES];


unsigned int EEPROM_ParseProfiles() {
  ProfileParameters_struct local_Profile;
  unsigned int local_hash;
  unsigned int EEP_ProfileSize = sizeof(local_Profile);
  unsigned int number_of_profiles = 0;

  EEPROM.get(number_of_profiles, local_Profile);
  local_hash = local_Profile.ProfileHash;

  for(int i = 0; i<EEPROM_SUPPORTED_PROFILES; i++) { /* parse all profiles */
    EEPROM.get( (i * EEP_ProfileSize), local_hash);
    if(local_hash == (unsigned int) PROFILE_HASH) { /* is this actually a profile or just junk? */
      ProfilesAvailable[i] = 1U;
      number_of_profiles++;
      DEBUG_MSG_EEP("Found valid profile %d. Profile Hash check PASSED. \n", i+1);
    }
    else{
      DEBUG_MSG_EEP("Found no valid profile %d. Profile Hash check FAILED. \n", i+1);      
    }
  }
  DEBUG_MSG_EEP("Current number of profiles in DFLS: %d \n", number_of_profiles);
  return number_of_profiles;
}

void EEPROM_StoreProfile(unsigned int profileNumber) {
  ProfileParameters_struct local_Profile = {
    .ProfileHash = (unsigned int) PROFILE_HASH,
    .ProfileID = profileNumber,
    .loop_MasterFireRippleEnabled = GlobalParameters.loop_MasterFireRippleEnabled,
    .currentNumberofRipples = MAX_NUMBER_OF_RIPPLES,
    .currentNumberofColors = GlobalParameters.currentNumberofColors,
    .currentBehavior = GlobalParameters.currentBehavior,
    .currentDelayBetweenRipples = GlobalParameters.currentDelayBetweenRipples, /* in milliseconds */
    .currentRainbowDeltaPerTick = GlobalParameters.currentRainbowDeltaPerTick, /* units: hue */
    .currentRippleLifeSpan = GlobalParameters.currentRippleLifeSpan, /* in milliseconds */
    .currentRippleSpeed = GlobalParameters.currentRippleSpeed,
    .currentDecay = GlobalParameters.currentDecay,
  };
  unsigned int EEP_ProfileSize = sizeof(local_Profile);
DEBUG_MSG_EEP("Storing profile %d at address %d \n", profileNumber, (profileNumber * EEP_ProfileSize));
  EEPROM.put((profileNumber * EEP_ProfileSize), local_Profile);
  EEPROM.commit();
  ProfilesAvailable[profileNumber] = 1U;   
  Global_NumberOfProfiles_InDFLS++;
}

void EEPROM_InvalidateProfile(unsigned int profileNumber) {
   ProfileParameters_struct local_Profile = {
    .ProfileHash = (unsigned int) 0U,
   };
   unsigned int EEP_ProfileSize = sizeof(local_Profile);
  EEPROM.put((profileNumber * EEP_ProfileSize), local_Profile);
  EEPROM.commit();
  ProfilesAvailable[profileNumber] = 0U;   
}




boolean EEPROM_RestoreProfile(unsigned int profileNumber) {
  ProfileParameters_struct local_Profile;
  unsigned int local_hash;
  boolean ret_val = 0U;
  unsigned int EEP_ProfileSize = sizeof(local_Profile);
  EEPROM.get((profileNumber * EEP_ProfileSize), local_Profile);
  local_hash = local_Profile.ProfileHash;

DEBUG_MSG_EEP("Restoring profile %d, %d bytes of data from address %d. \n", profileNumber, EEP_ProfileSize, (profileNumber * EEP_ProfileSize));
DEBUG_MSG_EEP("Restored the following data: \n ProfileHash: %d. ProfileID: %d\n", local_Profile.ProfileHash, local_Profile.ProfileID);
DEBUG_MSG_EEP("   currentNumberofRipples: %d. currentNumberofColors: %d, currentBehavior: %d,\n   currentDelayBetweenRipples: %d, currentRainbowDeltaPerTick: %d, etc... \n", local_Profile.currentNumberofRipples, local_Profile.currentNumberofColors, local_Profile.currentBehavior, local_Profile.currentDelayBetweenRipples, local_Profile.currentRainbowDeltaPerTick);
  if(local_hash == (unsigned int) PROFILE_HASH){ /* only apply configuration if we restored a valid hash from EEPROM */
    ret_val = 1U; /* valid profile found */
    GlobalParameters.loop_MasterFireRippleEnabled = local_Profile.loop_MasterFireRippleEnabled;
    GlobalParameters.currentNumberofColors = local_Profile.currentNumberofColors;
    GlobalParameters.currentBehavior  = local_Profile.currentBehavior;
    GlobalParameters.currentDelayBetweenRipples = local_Profile.currentDelayBetweenRipples;
    GlobalParameters.currentRainbowDeltaPerTick = local_Profile.currentRainbowDeltaPerTick;
    GlobalParameters.currentRippleLifeSpan = local_Profile.currentRippleLifeSpan;
    GlobalParameters.currentRippleSpeed = local_Profile.currentRippleSpeed;
    GlobalParameters.currentDecay = local_Profile.currentDecay;
  }
return ret_val;
}
