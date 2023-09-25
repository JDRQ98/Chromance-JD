#include "EEP.h"
#include "HTTP_Server.h"

static unsigned int EEP_ProfileSize = 0;

void EEPROM_Write_GlobalParameters() {
#if (EEPROM_DEBUGGING == TRUE)
  Serial.println("starting EEPROM write");
  Serial.println("Storing the following values:");
  Serial.print(" -Current Number of Ripples: ");
  Serial.println(GlobalParameters.currentNumberofRipples);
  Serial.print(" -Current Number of colors: ");
  Serial.println(GlobalParameters.currentNumberofColors);
  Serial.println("ETC...");
#endif
  EEPROM.put(0U, GlobalParameters);
}

void EEPROM_Read_GlobalParameters() {
  Serial.println("starting EEPROM read");
  EEPROM.get(0U, GlobalParameters);
#if (EEPROM_DEBUGGING == TRUE)
  Serial.println("Read the following values:");
  Serial.print(" -Current Number of Ripples: ");
  Serial.println(GlobalParameters.currentNumberofRipples);
  Serial.print(" -Current Number of colors: ");
  Serial.println(GlobalParameters.currentNumberofColors);
  Serial.println("ETC...");
#endif
}

unsigned int EEPROM_ParseProfiles() {
  ProfileParameters_struct local_Profile;
  unsigned int local_hash;
  unsigned int EEP_ProfileSize = sizeof(local_Profile);
  unsigned int number_of_profiles = 0;

  EEPROM.get(number_of_profiles, local_Profile);
  local_hash = local_Profile.ProfileHash;
#if (EEPROM_DEBUGGING == TRUE)
  Serial.print("Parsing profiles in DFLS - ");
  Serial.println("retored the following data:");
  Serial.print("ProfileHash: ");
  Serial.println(local_Profile.ProfileHash);
  Serial.print("ProfileID: ");
  Serial.println(local_Profile.ProfileID);
#endif
  while (local_hash == (unsigned int) PROFILE_HASH) { /* is this actually a profile or just junk? */
    number_of_profiles++;
    if ( (number_of_profiles * EEP_ProfileSize) < EEPROM_SIZE) break; /* be careful not to read from non-existing address */
    EEPROM.get( (number_of_profiles * EEP_ProfileSize), local_hash);
#if (EEPROM_DEBUGGING == TRUE)
    Serial.print("Profile ");
    Serial.println(number_of_profiles);
    Serial.print("hash: ");
    Serial.println(local_hash);
#endif
  }

#if (EEPROM_DEBUGGING == TRUE)
  Serial.print("Current number of profiles in DFLS: ");
  Serial.println(number_of_profiles);
  return number_of_profiles;
#endif
}

void EEPROM_StoreProfile(unsigned int profileNumber) {
  ProfileParameters_struct local_Profile = {
    .ProfileHash = (unsigned int) PROFILE_HASH,
    .ProfileID = profileNumber,
    .loop_MasterFireRippleEnabled = GlobalParameters.loop_MasterFireRippleEnabled,
    .loop_CenterFireRippleEnabled = GlobalParameters.loop_CenterFireRippleEnabled,
    .loop_RandomEffectEnabled = GlobalParameters.loop_RandomEffectEnabled,
    .currentNumberofRipples = GlobalParameters.currentNumberofRipples,
    .currentNumberofColors = GlobalParameters.currentNumberofColors,
    .currentBehavior = GlobalParameters.currentBehavior,
    .currentDelayBetweenRipples = GlobalParameters.currentDelayBetweenRipples, /* in milliseconds */
    .currentRainbowDeltaPerTick = GlobalParameters.currentRainbowDeltaPerTick, /* units: hue */
    .currentRippleLifeSpan = GlobalParameters.currentRippleLifeSpan, /* in milliseconds */
    .currentRippleSpeed = GlobalParameters.currentRippleSpeed,
    .currentDecay = GlobalParameters.currentDecay,
  };
  unsigned int EEP_ProfileSize = sizeof(local_Profile);
#if (EEPROM_DEBUGGING == TRUE)
  Serial.print("Storing profile ");
  Serial.print(profileNumber);
  Serial.print(", ");
  Serial.print(EEP_ProfileSize);
  Serial.print(" bytes of data at address ");
  Serial.println((profileNumber * EEP_ProfileSize));
#endif
  EEPROM.put((profileNumber * EEP_ProfileSize), local_Profile);
  EEPROM.commit();
}




void EEPROM_RestoreProfile(unsigned int profileNumber) {
  ProfileParameters_struct local_Profile;
  unsigned int EEP_ProfileSize = sizeof(local_Profile);
  EEPROM.get((profileNumber * EEP_ProfileSize), local_Profile);
#if (EEPROM_DEBUGGING == TRUE)
  Serial.print("Retoring profile ");
  Serial.print(profileNumber);
  Serial.print(", ");
  Serial.print(EEP_ProfileSize);
  Serial.print(" bytes of data from address ");
  Serial.println((profileNumber * EEP_ProfileSize));
  Serial.println("Retored the following data:");
  Serial.print("ProfileHash: ");
  Serial.println(local_Profile.ProfileHash);
  Serial.print("ProfileID: ");
  Serial.println(local_Profile.ProfileID);
#endif
  GlobalParameters.loop_MasterFireRippleEnabled = local_Profile.loop_MasterFireRippleEnabled;
  GlobalParameters.loop_CenterFireRippleEnabled = local_Profile.loop_CenterFireRippleEnabled;
  GlobalParameters.loop_RandomEffectEnabled  = local_Profile.loop_RandomEffectEnabled;
  GlobalParameters.currentNumberofRipples = local_Profile.currentNumberofRipples;
  GlobalParameters.currentNumberofColors = local_Profile.currentNumberofColors;
  GlobalParameters.currentBehavior  = local_Profile.currentBehavior;
  GlobalParameters.currentDelayBetweenRipples = local_Profile.currentDelayBetweenRipples;
  GlobalParameters.currentRainbowDeltaPerTick = local_Profile.currentRainbowDeltaPerTick;
  GlobalParameters.currentRippleLifeSpan = local_Profile.currentRippleLifeSpan;
  GlobalParameters.currentRippleSpeed = local_Profile.currentRippleSpeed;
  GlobalParameters.currentDecay = local_Profile.currentDecay;
}
