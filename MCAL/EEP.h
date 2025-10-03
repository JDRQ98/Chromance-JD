#ifndef EEP_H_
#define EEP_H_


void EEPROM_Init(void);

void EEPROM_ReadGlobalParameters(void);
void EEPROM_StoreGlobalParameters(void);

void EEPROM_Clear(void);

#endif
