#include <main.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include "Ethernet_Generic.h"
#include <EthernetWebServer.h>
#include <PubSubClient.h>
#include <utils.h>
#include <ArduinoJson.h>
#include <spiffs_storage.h>
#include <vector>
#include <save-json.hpp>
#include <RTClib.h>
#include <NTPClient.h>
#include <EEPROM.h>
#include <web-config.hpp>
#include <ModbusMaster.h>
#include <HTTPClient.h>

using std::vector;

TaskHandle_t wifiConnectTaskHandle = NULL;
TaskHandle_t mqttTaskHandle = NULL;
TaskHandle_t simulateTaskHandle = NULL;
TaskHandle_t drainBufferTaskHandle = NULL;
TaskHandle_t updateEpochTaskHandle = NULL;
TaskHandle_t ethConnectTaskHandle = NULL;
TaskHandle_t sendPayloadToCLPTaskHandle = NULL;
TaskHandle_t ledTaskHandle = NULL;
TaskHandle_t jobTaskHandle = NULL;

uint8_t mac[6];

String deviceId = getDeviceId(true);
String TOPIC_REBOOT = deviceId+"/reboot";
String TOPIC_DATA = "";
String SUB_DATA = "";
String SERVER_MQTT = "broker.hivemq.com";
int PORT_MQTT = 1883;
int SPEED = 115200;
int SLAVE = 1;

int DHCP = 1;

String IP_ADDR = "";
String IP_MASK = "";
String GATEWAY = "";
String DNS = "";

IPAddress myIp;
IPAddress myMask;
IPAddress myDns;
IPAddress myGateway;

bool initialSetup = 0;
bool wifiConnected = false;
bool ethernetConnected = false;
bool mqttConnected = false;
bool bufferAction = false;
bool bufferDrainFlag = false;
bool factoryConfig = true;
bool sendFlag = false;
bool sendLedFlag = false;
bool recvLedFlag = false;
bool errorConnectLedState = false;

unsigned long timeConfig = 0;
unsigned long blinkError = 30000;

int ntpServer = 0;

uint32_t bufferTime = 0;

vector<String> buffer;

SaveJson saveJson;

EthernetWebServer ethernetServer(80);
RTC_DS3231 rtc;
Client * client = new WiFiClient();
PubSubClient * mqttClient = new PubSubClient(*client);
UDP * ntpUDP = new WiFiUDP();
NTPClient * timeClient = new NTPClient(*ntpUDP);
ModbusMaster node;
JsonObject localDoc;

void setup() {
  Serial.begin(115200);
  EEPROM.begin(100);
  initSPIFFS();
  rtc.begin();

  if(!SPIFFS.exists("/buffer.json")){
    SPIFFS.mkdir("/buffer.json");
  }

  if(EEPROM.read(0) != 96){
    EEPROM.write(0, 96);
    EEPROM.commit();
  }else{
    timeClient->setEpochTime(EEPROM.readULong(EPOCH_ADDR) - TIME_FUSE);
  }

  if(EEPROM.read(CONFIG) < 1){
    DHCP = 1;
    saveJson.write("Dhcp", "1");
    EEPROM.write(CONFIG, 1);
    EEPROM.commit();
  } else {
    EEPROM.write(CONFIG, 0);
    EEPROM.commit();
  }

  if(EEPROM.read(FIRST_SETUP) != 77){
    saveJson.write("Server", "broker.hivemq.com");
    saveJson.write("Port", "1883");
    saveJson.write("Topic", "innotech/"+getDeviceId(false)+"/message");
    saveJson.write("Sub", "innotech/"+getDeviceId(false)+"/response");
    saveJson.write("Speed", "115200");
    saveJson.write("Slave", "1");
    saveJson.write("Dhcp", "1");
    saveJson.write("Time", "1");
    saveJson.write("Ip", "");
    saveJson.write("Mask", "");
    saveJson.write("Dns", "");
    saveJson.write("Gateway", "");
    saveJson.write("Registers", "40");
    EEPROM.write(FIRST_SETUP, 77);
    EEPROM.commit();
  }

  pinMode(WTD_PIN, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  pinMode(LED_ON, OUTPUT);
  pinMode(LED_RECV_SEND, OUTPUT);
  pinMode(LED_CONCT, OUTPUT);
  pinMode(LED_ERROR, OUTPUT);

  digitalWrite(WTD_PIN, HIGH);
  digitalWrite(LED_ON, HIGH);
  digitalWrite(LED_CONCT, LOW);
  digitalWrite(LED_ERROR, LOW);

  Serial.println(deviceId);

  SERVER_MQTT = saveJson.read("Server");
  PORT_MQTT = saveJson.read("Port").toInt();
  TOPIC_DATA = saveJson.read("Topic");
  SUB_DATA = saveJson.read("Sub");
  SPEED = saveJson.read("Speed").toInt();
  SLAVE = saveJson.read("Slave").toInt();
  DHCP = saveJson.read("Dhcp").toInt();
  bufferTime = (saveJson.read("Time").toInt() < 1 || saveJson.read("Time").toInt() > 60 ? 1 : saveJson.read("Time").toInt())*60000;

  if(!DHCP){
    Serial.printf("\nIP:\t%s\nMASK:\t%s\nDNS:\t%s\nGateway:\t%s\n",
      saveJson.read("Ip").c_str(), saveJson.read("Mask").c_str(), saveJson.read("Dns").c_str(), saveJson.read("Gateway").c_str());
    myIp.fromString(saveJson.read("Ip").c_str());
    myMask.fromString(saveJson.read("Mask").c_str());
    myDns.fromString(saveJson.read("Dns").c_str());
    myGateway.fromString(saveJson.read("Gateway").c_str());
  }

  Serial1.begin(SPEED);
  Serial1.begin(SPEED, SERIAL_8N1, RXD2, TXD2);

  node.begin(SLAVE, Serial1);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  mqttClient->setBufferSize(2048);

  Ethernet.init(5);
  WizReset();

  ethernetServer.on("/", menuPage);
  ethernetServer.on("/wifi", searchPage);
  ethernetServer.on("/sendWiFi", copyPage);
  ethernetServer.on("/config", configPage);
  ethernetServer.on("/reset", resetPage);
  ethernetServer.on("/config-slave", configSlavePage);
  ethernetServer.on("/resets", resetSlavePage);
  ethernetServer.on("/network", configNetworkPage);
  ethernetServer.on("/setup", receiveSetup);

  xTaskCreate(wifiConnectTask, "connect to wifi", 4096, NULL, 1, &wifiConnectTaskHandle);
  delay(100);
  xTaskCreate(mqttTask, "mqtt task", 10000, NULL, 1, &mqttTaskHandle);
  delay(100);
  xTaskCreate(ethConnectTask, "connect eth", 4096, NULL, 1, &ethConnectTaskHandle);
  delay(100);
  xTaskCreate(simulateTask, "simulate task", 80000, NULL, 1, &simulateTaskHandle);
  delay(100);
  xTaskCreate(drainBuffer, "drain buffer", 10000, NULL, 1, &drainBufferTaskHandle);
  delay(100);
  xTaskCreate(updateEpoch, "update time", 4096, NULL, 1, &updateEpochTaskHandle);
  delay(100);
  xTaskCreate(sendPayloadToCLPTask, "sendPayloadToCLPTask", 8000, NULL, 1, &sendPayloadToCLPTaskHandle);
  delay(100);
  xTaskCreate(ledTask, "ledTask", 4096, NULL, 1, &ledTaskHandle);
  delay(100);
  xTaskCreate(jobTask, "jobTask", 5*1024, NULL, 1, &jobTaskHandle);

  // setupNTP();

  Serial.println(SERVER_MQTT);
  Serial.println(PORT_MQTT);
  Serial.println(TOPIC_DATA);
  Serial.println(SUB_DATA);
  Serial.println(SPEED);
  Serial.println(SLAVE);

  Serial.print("WIFI:\t");
  Serial.println(WiFi.SSID());
  Serial.print("PASS:\t");
  Serial.println(WiFi.psk());

  timeConfig = millis();
}

void loop() {
  ethernetServer.handleClient();
  if(factoryConfig && (millis() - timeConfig >= 10000)){
      factoryConfig = false;
      EEPROM.write(CONFIG, 1);
      EEPROM.commit();
      Serial.println("DHCP factory timeout");
  }
  if(!ethernetConnected && !wifiConnected && millis() > 30000 && (millis() - blinkError) >= 500){
    blinkError = millis();
    errorConnectLedState = !errorConnectLedState;
    vTaskResume(ledTaskHandle);
  }
  if(errorConnectLedState && (ethernetConnected || wifiConnected)){
    errorConnectLedState = false;
    vTaskResume(ledTaskHandle);
  }
  delay(1);
}

void wifiConnectTask(void * pvP){
  unsigned long timeAttempt = 0;
  vTaskSuspend(NULL);
  while(1){
    if(WiFi.status() != WL_CONNECTED){
      if(DEBUG_WIFI){
        Serial.println("Tentando se Conectar a rede Wireless");
      }
      timeAttempt = millis();
      if(DHCP){
        WiFi.begin("Ctba-Ribeiro", "Amandinha123");
      } else {
        WiFi.config(myIp, myGateway, myMask, myDns);
        WiFi.begin();
      }
      while(WiFi.status() != WL_CONNECTED && (millis() - timeAttempt) < 10000){
        if(DEBUG_WIFI){
          Serial.print(".");
        }
        delay(1000);
      }
      if(WiFi.status() == WL_CONNECTED){
        wifiConnected = true;
        client = new WiFiClient();
        timeClient->end();
        ntpUDP = new WiFiUDP();
        timeClient = new NTPClient(*ntpUDP);
        setupNTP();
        if(!initialSetup){
          mqttClient = new PubSubClient(*client);
          vTaskResume(updateEpochTaskHandle);
          initialSetup = true;
        }
        if(DEBUG_WIFI){
          Serial.println("OK!");
        }

      }else{
        wifiConnected = false;
        WiFi.disconnect(true);
        if(DEBUG_WIFI){
          Serial.println(" Falha ao Tentantar se conectar!");
        }
      }
    }
    delay(10000);
  }
}

void ethConnectTask(void * pvP){
  WiFi.macAddress(mac);
  while(1){
    if(!ethernetConnected){
      Serial.printf("Try connect! DHCP: %i\n", DHCP);
      if(DHCP){
        if(Ethernet.begin(mac)){
          if(wifiConnectTaskHandle){
            vTaskSuspend(wifiConnectTaskHandle);
            WiFi.disconnect(true);
            Serial.println("Suspend Wi-Fi");
          }
          Serial.println("OK!");
          ethernetServer.begin();
          Serial.print("AUTO IP: ");
          Serial.println(Ethernet.localIP());
          ethernetConnected = true;
          timeClient->end();
          client = new EthernetClient();
          ntpUDP = new EthernetUDP();
          timeClient = new NTPClient(*ntpUDP);
          setupNTP();
          if(!initialSetup){
            mqttClient = new PubSubClient(*client);
            vTaskResume(updateEpochTaskHandle);
            initialSetup = true;
          }
        }else{
          ethernetConnected = false;
          Serial.println("FAIL!");
          WizReset();
        }
      } else {
        Ethernet.begin(mac, myIp, myDns, myGateway, myMask);
        if(Ethernet.linkStatus() == EthernetLinkStatus::LinkON){
          if(wifiConnectTaskHandle){
            vTaskSuspend(wifiConnectTaskHandle);
            WiFi.disconnect(true);
            Serial.println("Suspend Wi-Fi");
          }
          Serial.println("OK!");
          ethernetServer.begin();
          Serial.print("PERSONAL IP: ");
          Serial.println(Ethernet.localIP());
          ethernetConnected = true;
          timeClient->end();
          client = new EthernetClient();
          ntpUDP = new EthernetUDP();
          timeClient = new NTPClient(*ntpUDP);
          setupNTP();
          if(!initialSetup){
            mqttClient = new PubSubClient(*client);
            vTaskResume(updateEpochTaskHandle);
            initialSetup = true;
          }
        }else{
          ethernetConnected = false;
          Serial.println("FAIL!");
          WizReset();
        }
      }
    }
    delay(3000);
  }
}

void mqttTask(void * pvP){
  bool initialLink = false;
  while(1){
    if(ethernetConnected || wifiConnected){
      if(!mqttConnected){
        mqttClient->setClient(*client);
        mqttClient->setServer(SERVER_MQTT.c_str(), PORT_MQTT);
        mqttClient->setCallback(mqttCallback);
      }
      if(!mqttClient->connected()){
        mqttConnected = false;
        vTaskResume(ledTaskHandle);
        reconnectMqtt();
      }else{
        mqttConnected = true;
        vTaskResume(ledTaskHandle);
        mqttClient->loop();
      }
      delay(10);
    }else{
      delay(5000);
    }
    if(Ethernet.linkStatus() != EthernetLinkStatus::LinkON && WiFi.status() != WL_CONNECTED){
      delay(10);
      if(initialLink){
        if(wifiConnectTaskHandle){
          initialLink = false;
          ethernetConnected = false;
          vTaskResume(wifiConnectTaskHandle);
          Serial.println("Resume Wi-Fi");
        }
      }else{
        initialLink = true;
        Serial.println("LinkOFF detected!");
      }
    }else{
      initialLink = false;
    }
    delay(10);
  }
}

void simulateTask(void * pvP){
  DynamicJsonDocument data(2048);
  uint8_t result;
  int registers = saveJson.read("Registers").toInt();
  String payload = "";
  bool drainFlag = false;
  int drainTime = 1;
  while(1){
    delay(drainFlag ? bufferTime - drainTime : bufferTime);
    if(drainFlag) drainFlag = false;
    data.clear();
    payload.clear();

    DateTime now = rtc.now();

    data["timestamp"] = now.unixtime();

    result = node.readHoldingRegisters(0x00, registers);
    Serial.print("result: ");Serial.println(result);
    if(result == node.ku8MBSuccess){
      for(int i = 0; i < registers; i++)
        data[("p" + String(i))] = node.getResponseBuffer(i)/10.0;
    }


    serializeJson(data, payload);

    if(bufferDrainFlag){
      drainTime = buffer.size()*50;
      delay(drainTime);
      drainFlag = true;
    }
    if(mqttConnected){
      Serial.print("Send\t");
      Serial.println(payload);
      bool isSended = mqttClient->publish(TOPIC_DATA.c_str(), payload.c_str());
      sendLedFlag = true;
      vTaskResume(ledTaskHandle);
      Serial.println(isSended ? "Success!" : "Fail!");
    }
    if(!mqttConnected){
      if(buffer.size() < BUFFER_SIZE){
        bufferAction = true;
        buffer.push_back(payload);
      }else{
        buffer.erase(buffer.begin());
        buffer.push_back(payload);
      }
      appendFile("/buffer.json", payload.c_str());
    }
  }
}

void drainBuffer(void * pvP){
  while(1){
    if(mqttConnected && bufferAction){
      bufferDrainFlag = true;
      File file = SPIFFS.open("/buffer.json");
      bool error = false;
      while (file.available()) {
        if(mqttConnected){
          Serial.println(mqttClient->publish(TOPIC_DATA.c_str(), file.readStringUntil('\n').c_str()) ? "Success!" : "Fail!");
          sendLedFlag = true;
          vTaskResume(ledTaskHandle);
        } else {
          error = true;
          break;
        }
        delay(50);
      }
    
      file.close();
      if (!error) {
        deleteFile("/buffer.json");
        SPIFFS.mkdir("/buffer.json");
      }
      buffer.clear();
      bufferAction = false;
      bufferDrainFlag = false;
    }else{
      delay(100);
    }
    delay(50);
  }
}

void mqttCallback(char * topic, byte * payload, unsigned int length){
  String topicMqtt = topic;
  char rcvdPayload[4096];
  strncpy(rcvdPayload,(char *)payload,length);

  DynamicJsonDocument doc(4096);
  deserializeJson(doc, rcvdPayload);

  serializeJsonPretty(doc, Serial);

  if(DEBUG_WIFI_MQTT){
    Serial.print("\n*** Nova Mensagem ***\n");
    Serial.print("Topic: ");
    Serial.println(topicMqtt);
  }
  recvLedFlag = true;
  vTaskResume(ledTaskHandle);
  if(topicMqtt == TOPIC_REBOOT){
    if(DEBUG_WIFI_MQTT){
      Serial.println("Reiniciando...");
    }
    delay(2000);
    ESP.restart();
  }else if(topicMqtt == SUB_DATA){
    if(doc.containsKey("ini")){
      if(doc.containsKey("reg")){
        Serial.println("oi");
        localDoc = doc.as<JsonObject>();
        if(!sendFlag)
          vTaskResume(sendPayloadToCLPTaskHandle);
      }
    }
  }
}

void reconnectMqtt(){
  if(DEBUG_WIFI_MQTT){
    Serial.println("Tentando se Conectar ao Broker mqtt...");
    Serial.print("Domain:\t");
    Serial.println(SERVER_MQTT);
  }
  if(mqttClient->connect(deviceId.c_str())){
    if(DEBUG_WIFI_MQTT){
      Serial.println("Conectado ao Broker!");
    }
    mqttClient->subscribe(TOPIC_REBOOT.c_str());
    mqttClient->subscribe(SUB_DATA.c_str());
  }
  delay(500);
}

void updateEpoch(void *pvParameters){
  int minutes = 0;
  vTaskSuspend(NULL);
  while (1){
    if(wifiConnected || ethernetConnected){
      while(!timeClient->update()){
        timeClient->forceUpdate();
        delay(1);
      }
      rtc.adjust(DateTime(timeClient->getEpochTime()));
    }
    if (minutes >= 6){
      minutes = 0;
      EEPROM.writeULong(EPOCH_ADDR, timeClient->getEpochTime());
      EEPROM.commit();
    }
    minutes++;
    Serial.println(timeClient->getFormattedDate());
    vTaskDelay(60000 / portTICK_RATE_MS);
  }
}

void sendPayloadToCLPTask(void * pvP){
  vTaskSuspend(NULL);
  while(1){
    sendFlag = true;
    uint cont = localDoc["ini"].as<int>();
    for(JsonVariant reg : localDoc["reg"].as<JsonArray>()){
      Serial.printf("register -> %i, value: -> %i\n", cont, reg.as<uint16_t>());
      node.writeSingleCoil(cont++, reg.as<uint16_t>());
      delay(50);
    }
    localDoc.clear();
    Serial.println("paused Task Send");
    sendFlag = false;
    vTaskSuspend(NULL);
  }
}

void ledTask(void * pvP){
  vTaskSuspend(NULL);
  while(1){
    if(sendLedFlag){
      digitalWrite(LED_RECV_SEND, HIGH);
      delay(40);
      digitalWrite(LED_RECV_SEND, LOW);
      sendLedFlag = false;
    }
    if(recvLedFlag){
      for(int i = 0; i < 4; i++){
        digitalWrite(LED_RECV_SEND, !(i % 2));
        delay(40);
      }
      recvLedFlag = false;
    }
    digitalWrite(LED_CONCT, mqttConnected);
    digitalWrite(LED_ERROR, errorConnectLedState);
    vTaskSuspend(NULL);
  }
}

void jobTask(void * pvP){
  StaticJsonDocument<512> doc;
  while(1){
    if(millis() > 20000 && (wifiConnected || ethernetConnected)){
      Serial.println("oi");
      deserializeJson(doc, getJSON(wifiConnected));
      Serial.println("oi2");
      if(doc.containsKey("version") && doc["version"].as<String>() != EEPROM.readString(FW_VERSION_ADDR)){
        if(execOTA("fw-innotech.s3.amazonaws.com", "/CLP-Slave/firmware.bin", doc["version"].as<String>())){
          delay(3000);
          ESP.restart();
        }
      }
      doc.clear();
      delay(50000);
    }
    delay(100);
  }
}

void setupNTP(){
  Serial.println("SETUP-NTP");
  timeClient->begin();
  timeClient->setUpdateInterval(60000);
  Serial.println(timeClient->getFormattedDate());
}

void WizReset(){
  pinMode(RESET_P, OUTPUT);
  digitalWrite(RESET_P, HIGH);
  vTaskDelay(250 / portTICK_PERIOD_MS);
  digitalWrite(RESET_P, LOW);
  vTaskDelay(50 / portTICK_PERIOD_MS);
  digitalWrite(RESET_P, HIGH);
  vTaskDelay(350 / portTICK_PERIOD_MS);
}

void menuPage(){
  String page = FPSTR(OPEN_HEADER);
  page += FPSTR(STYLE);
  page += FPSTR(SCRIPT);
  page += FPSTR(BODY_START);
  page += "<h3>"+getDeviceId(true)+"</h3>";
  page += FPSTR(MENU);
  page += FPSTR(BODY_END);
  ethernetServer.send(200, "text/html", page);
}

void searchPage(){
  String page = FPSTR(OPEN_HEADER);
  page += FPSTR(STYLE);
  page += FPSTR(SCRIPT);
  page += FPSTR(BODY_START);
  page += "<h3>"+getDeviceId(true)+"</h3>";
  page += FPSTR(SEARCH_PAGE);
  page += "<script>document.getElementById(\"ssid\").value = '"+WiFi.SSID()+"';document.getElementById(\"pass\").value='"+WiFi.psk()+"';</script>";
  page += FPSTR(BODY_END);
  ethernetServer.send(200, "text/html", page);
}

void copyPage(){
  String page = FPSTR(OPEN_HEADER);
  page += FPSTR(STYLE);
  page += FPSTR(SCRIPT);
  page += FPSTR(BODY_START);
  page += "<h3>"+getDeviceId(true)+"</h3>";
  page += FPSTR(COPY_PAGE);
  page += FPSTR(BODY_END);

  String ssid = "";
  String pass = "";

  ethernetServer.send(200, "text/html", page);
  delay(2000);
  ssid = ethernetServer.arg(0);
  pass = ethernetServer.arg(1);

  Serial.printf("\nSSID: %s\tPASS: %s\n\n", ssid.c_str(), pass.c_str());

  int count = 0;

  WiFi.begin(ssid.c_str(), pass.c_str());

  while(WiFi.status() != WL_CONNECTED){
    WiFi.disconnect();
    WiFi.begin(ssid.c_str(), pass.c_str());
    count++;
    vTaskDelay(1000);
    if(count >= 10){
      break;
    }
  }
  if(count >= 10){
    Serial.printf("FAIL:\tReset...");
    ESP.restart();
  }else{
    Serial.printf("SUCCESS:\tReset...");
    ESP.restart();
  }
}

void configPage(){
  String page = FPSTR(OPEN_HEADER);
  page += FPSTR(STYLE);
  page += FPSTR(SCRIPT);
  page += FPSTR(BODY_START);
  page += "<h3>"+getDeviceId(true)+"</h3>";
  page += FPSTR(CONFIG_PAGE);
  page += "<script>document.getElementById(\"server\").value = '"+saveJson.read("Server")+"';document.getElementById(\"port\").value = '"+saveJson.read("Port")+"';document.getElementById(\"topic\").value = '"+saveJson.read("Topic")+"';document.getElementById('sub').value='"+saveJson.read("Sub")+"';document.getElementById('time').value='"+(saveJson.read("Time").toInt() < 1 || saveJson.read("Time").toInt() > 60 ? "" : String(saveJson.read("Time").toInt()))+"';</script>";
  page += FPSTR(BODY_END);
  ethernetServer.send(200, "text/html", page);
}

void configNetworkPage(){
  String page = FPSTR(OPEN_HEADER);
  page += FPSTR(STYLE);
  page += FPSTR(SCRIPT);
  page += FPSTR(BODY_START);
  page += "<h3>"+getDeviceId(true)+"</h3>";
  page += FPSTR(CONFIG_NETWORK);
  page += "<script>document.getElementById(\"dhcp\").value='"+saveJson.read("Dhcp")+"';document.getElementById(\"ip\").value='"+saveJson.read("Ip")+"';document.getElementById(\"mask\").value='"+saveJson.read("Mask")+"';document.getElementById(\"dns\").value='"+saveJson.read("Dns")+"';document.getElementById(\"gateway\").value='"+saveJson.read("Gateway")+"';</script>";
  page += FPSTR(BODY_END);
  ethernetServer.send(200, "text/html", page);
}

void configSlavePage(){
  String page = FPSTR(OPEN_HEADER);
  page += FPSTR(STYLE);
  page += FPSTR(SCRIPT);
  page += FPSTR(BODY_START);
  page += "<h3>"+getDeviceId(true)+"</h3>";
  page += FPSTR(CONFIG_SLAVE);
  page += "<script> document.getElementById(\"speed\").value = '"+saveJson.read("Speed")+"'; document.getElementById(\"nodeid\").value = '"+saveJson.read("Slave")+"'; document.getElementById(\"registers\").value = '"+saveJson.read("Registers")+"';</script>";
  page += FPSTR(BODY_END);
  ethernetServer.send(200, "text/html", page);
}

void resetPage(){
  String page = FPSTR(OPEN_HEADER);
  page += FPSTR(STYLE);
  page += FPSTR(SCRIPT);
  page += FPSTR(BODY_START);
  page += "<h3>"+getDeviceId(true)+"</h3>";
  page += FPSTR(RESET_PAGE);
  page += FPSTR(BODY_END);
  String serverStr = "";
  String port = "";
  String topic = "";
  String sub = "";
  String time = "";
  ethernetServer.send(200, "text/html", page);
  delay(2000);
  serverStr = ethernetServer.arg(0);
  port = ethernetServer.arg(1);
  topic = ethernetServer.arg(2);
  sub = ethernetServer.arg(3);
  time = ethernetServer.arg(4);
  if(serverStr.length())
    saveJson.write("Server", serverStr);
  if(port.length())
    saveJson.write("Port", port);
  if(topic.length())
    saveJson.write("Topic", topic);
  if(sub.length())
    saveJson.write("Sub", sub);
  if(time.length())
    saveJson.write("Time", time);
  delay(1000);
  ESP.restart();
}

void resetSlavePage(){
  String page = FPSTR(OPEN_HEADER);
  page += FPSTR(STYLE);
  page += FPSTR(SCRIPT);
  page += FPSTR(BODY_START);
  page += "<h3>"+getDeviceId(true)+"</h3>";
  page += FPSTR(RESET_PAGE);
  page += FPSTR(BODY_END);
  String speed = "";
  String slave = "";
  String registers = "";
  ethernetServer.send(200, "text/html", page);
  delay(2000);
  speed = ethernetServer.arg(0);
  slave = ethernetServer.arg(1);
  registers = ethernetServer.arg(2);
  if(speed.length())
    saveJson.write("Speed", speed);
  if(slave.length())
    saveJson.write("Slave", slave);
  if(registers.length())
    saveJson.write("Registers", registers);
  delay(1000);
  ESP.restart();
}

void receiveSetup(){
  String page = FPSTR(OPEN_HEADER);
  page += FPSTR(STYLE);
  page += FPSTR(SCRIPT);
  page += FPSTR(BODY_START);
  page += "<h3>"+getDeviceId(true)+"</h3>";
  page += FPSTR(RESET_PAGE);
  page += FPSTR(BODY_END);
  String dhcp = "";
  String ip = "";
  String mask = "";
  String dns = "";
  String gateway = "";
  ethernetServer.send(200, "text/html", page);
  delay(2000);
  dhcp = ethernetServer.arg(0);
  ip = ethernetServer.arg(1);
  mask = ethernetServer.arg(2);
  dns = ethernetServer.arg(3);
  gateway = ethernetServer.arg(4);
  Serial.println(dhcp);
  Serial.println(ip);
  Serial.println(mask);
  Serial.println(dns);
  Serial.println(gateway);
  if(dhcp.length())
    saveJson.write("Dhcp", dhcp);
  if(ip.length())
    saveJson.write("Ip", ip);
  if(mask.length())
    saveJson.write("Mask", mask);
  if(dns.length())
    saveJson.write("Dns", dns);
  if(gateway.length())
    saveJson.write("Gateway", gateway);
  delay(1000);
  ESP.restart();
}

String getJSON(bool isWifi){
  Serial.println("oi3");
  String payload = "{}";
  HTTPClient http;
  EthernetClient http2;
  String serverPath = "https://fw-innotech.s3.amazonaws.com/CLP-Slave/CLP.json";
  Serial.println("oi4");
  if(isWifi){
    http.begin(serverPath.c_str());
    Serial.println("oi5");
    int response = http.GET();
    Serial.println("oi6");
    if(response > 0){
      Serial.printf("HTTP CODE: %i\n", response);
      payload = http.getString();
    }else{
      Serial.printf("ERROR CODE: %i\n", response);
    }
    http.end();
  }else{
    if(http2.connect(serverPath.c_str(), 80)){
      Serial.println("Ethernet HTTP connected!");
      http2.println("GET /get HTTP/1.1");
      http2.println("Host: " + serverPath);
      http2.println("Connection: close");
    }
    int len = http2.available();
     if(len > 0){
      payload = http2.readString();
     }
  }
  Serial.println(payload.c_str());
  return payload;
}