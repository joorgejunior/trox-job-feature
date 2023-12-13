#pragma once
#include <Arduino.h>

#define MAX485_DE 4
#define FW_VERSION_ADDR 80

#define RXD2 16 //RXX2 pin
#define TXD2 17 //TX2 pin

String getDeviceId(bool complete);
void preTransmission();
void postTransmission();
bool execOTA(const char* host,const char* bin, String fwVersion);