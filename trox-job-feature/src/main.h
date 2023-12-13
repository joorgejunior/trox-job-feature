#pragma once
#include <Arduino.h>

#define DEBUG_WIFI 1
#define DEBUG_WIFI_MQTT 1

#define BUFFER_TIME 30000
#define BUFFER_SIZE 40

#define WTD_PIN 12
#define RESET_P 2
#define LED_ON 27
#define LED_RECV_SEND 26
#define LED_CONCT 25
#define LED_ERROR 33

#define TIME_FUSE -10800

#define EPOCH_ADDR 1
#define SPEED_ADDR 20
#define CONFIG 50
#define FIRST_SETUP 77

void mqttTask(void * pvP);
void simulateTask(void * pvP);
void drainBuffer(void * pvP);
void bufferPrint(void * pvP);
void updateEpoch(void *pvParameters);
void wifiConnectTask(void * pvP);
void ethConnectTask(void * pvP);
void sendPayloadToCLPTask(void * pvP);
void ledTask(void * pvP);
void jobTask(void * pvP);

void mqttCallback(char * topic, byte * payload, unsigned int length);
void reconnectMqtt();
void setupNTP();
void WizReset();

void searchPage();
void copyPage();
void configPage();
void resetPage();
void configSlavePage();
void resetSlavePage();
void menuPage();
void refreshNetworks(String * page);
void configNetworkPage();
void receiveSetup();
String getJSON(bool isWifi);