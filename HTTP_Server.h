#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <WiFi.h>

void HandleHTTPRequest(WiFiClient client);      
void WiFi_MainFunction(void);      
void WiFi_init(void);           

#endif /* HTTP_SERVER_H */
