#ifndef EEP_H_
#define EEP_H_

#define EEPROM_SAVE_COOLDOWN_MS 5000U

void EEPROM_Init(void);

void EEPROM_ReadGlobalParameters(void);
void EEPROM_StoreGlobalParameters(void);
void EEPROM_MarkDirty(void);
void EEPROM_DebouncedSave(void); // call from main loop

void EEPROM_Clear(void);

#endif
