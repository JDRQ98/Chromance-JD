#ifndef EEP_H_
#define EEP_H_

#include "EEPROM.h"

#define EEPROM_SIZE 512
#define PROFILE_HASH 0xAAAAAAAA
#define EEPROM_DEBUGGING TRUE
#define EEPROM_SUPPORTED_PROFILES 5

/* public variables */
extern unsigned int Global_NumberOfProfiles_InDFLS;
extern boolean ProfilesAvailable[EEPROM_SUPPORTED_PROFILES];

typedef struct {
  unsigned int ProfileHash; /* hash isn't the most appropriate name for this, this is simply a byte used to identy in DFLS that this is the start of a profile */
  unsigned int ProfileID;
  boolean loop_MasterFireRippleEnabled;
  boolean loop_CenterFireRippleEnabled;
  boolean loop_RandomEffectEnabled;
  unsigned char currentNumberofRipples;
  unsigned char currentNumberofColors;
  unsigned char currentBehavior;
  short currentDelayBetweenRipples;
  short currentRainbowDeltaPerTick; /* units: hue */
  unsigned long currentRippleLifeSpan;
  float currentRippleSpeed;
  float currentDecay;
} ProfileParameters_struct;

extern void EEPROM_Write_GlobalParameters();
extern void EEPROM_Read_GlobalParameters();

unsigned int EEPROM_ParseProfiles();
extern boolean EEPROM_RestoreProfile(unsigned int profileNumber); /* returns TRUE if restore successful, FALSE if restore unsuccessful*/
extern void EEPROM_StoreProfile(unsigned int profileNumber);

#endif
