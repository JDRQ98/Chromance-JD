#ifndef EEP_H_
#define EEP_H_

#include "EEPROM.h"

#define EEPROM_SIZE 512

extern void EEPROM_Write_GlobalParameters();
extern void EEPROM_Read_GlobalParameters();

#endif
