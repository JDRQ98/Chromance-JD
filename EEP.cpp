#include "EEP.h"
#include "HTTP_Server.h"

void EEPROM_Write_GlobalParameters(){
  Serial.println("starting EEPROM write");
  Serial.println("Storing the following values:");
  Serial.print(" -Current Number of Ripples: ");
  Serial.println(GlobalParameters.currentNumberofRipples);
  Serial.print(" -Current Number of colors: ");
  Serial.println(GlobalParameters.currentNumberofColors);
  Serial.println("ETC...");
  EEPROM.put(0U, GlobalParameters);
}

void EEPROM_Read_GlobalParameters(){
  Serial.println("starting EEPROM read");
  EEPROM.get(0U, GlobalParameters);
  Serial.println("Read the following values:");
  Serial.print(" -Current Number of Ripples: ");
  Serial.println(GlobalParameters.currentNumberofRipples);
  Serial.print(" -Current Number of colors: ");
  Serial.println(GlobalParameters.currentNumberofColors);
  Serial.println("ETC...");
}
