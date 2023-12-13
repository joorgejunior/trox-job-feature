#include <utils.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <Update.h>
//#include "Ethernet_Generic.h"

String getDeviceId(bool complete){
  byte mac[6];
  String id = "";
  WiFi.macAddress(mac);

  for (int i = 0; i < 6; i++) {
    String aux = String(mac[i], HEX);
    if (aux.length() < 2)
      id.concat("0");
    id.concat(aux);
  }
  id.toUpperCase();
  
  return complete ? "ID-"+id : id;
}

void preTransmission(){
  digitalWrite(MAX485_DE, 1);
}

void postTransmission(){
  digitalWrite(MAX485_DE, 0);
}

String getHeaderValue(String header, String headerName) {
  return header.substring(strlen(headerName.c_str()));
}


bool execOTA(const char* host,const char* bin, String fwVersion) {
  WiFiClient wifiClientOTA;
  int contentLength = 0;
  bool isValidContentType = false;
  Serial.println("Connecting to: " + String(host));
  if (wifiClientOTA.connect(host, 80)) {
    wifiClientOTA.print(String("GET ") + bin + " HTTP/1.1\r\n" +
                     "Host: " + host + "\r\n" +
                     "Cache-Control: no-cache\r\n" +
                     "Connection: close\r\n\r\n");
    unsigned long timeout = millis();
    while (wifiClientOTA.available() == 0) {
      if (millis() - timeout > 25000) {
        Serial.println("client1 Timeout !");
        wifiClientOTA.stop();
        return false;
      }
    }
    while (wifiClientOTA.available()) {
      String line = wifiClientOTA.readStringUntil('\n');
      line.trim();
      if (!line.length()) {
        break;
      }
      if (line.startsWith("HTTP/1.1")) {
        if (line.indexOf("200") < 0) {
          Serial.println("Got a non 200 status code from server. Exiting OTA Update.");
          break;
        }
      }
      if (line.startsWith("Content-Length: ")) {
        contentLength = atol((getHeaderValue(line, "Content-Length: ")).c_str());
        Serial.println("Got " + String(contentLength) + " bytes from server");
      }
      if (line.startsWith("Content-Type: ")) {
        String contentType = getHeaderValue(line, "Content-Type: ");
        Serial.println("Got " + contentType + " payload.");
        if (contentType == "application/octet-stream") {
          isValidContentType = true;
        }
      }
    }
  } else {
    Serial.println("Connection to " + String(host) + " failed. Please check your setup");
  }
  Serial.println("contentLength : " + String(contentLength) + ", isValidContentType : " + String(isValidContentType));
  if ((contentLength && isValidContentType)) {
    bool canBegin = Update.begin(contentLength);
    if (canBegin) {
      Serial.println("Begin OTA. This may take 2 - 5 mins to complete. Things might be quite for a while.. Patience!");
      size_t written = Update.writeStream(wifiClientOTA);

      if (written == contentLength) {
        Serial.println("Written : " + String(written) + " successfully");
      } else {
        Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?" );
      }

      if (Update.end()) {
        Serial.println("OTA done!");
        if (Update.isFinished()) {
          Serial.println("Update successfully completed. Rebooting.");
          EEPROM.writeString(FW_VERSION_ADDR, fwVersion);
          EEPROM.commit();
          Serial.println("gravou nova versao ##########################");
          return true;
        } else {
          Serial.println("Update not finished? Something went wrong!");
        }
      } else {
        Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      }
    } else {
      Serial.println("Not enough space to begin OTA");
      wifiClientOTA.flush();
      return false;
    }
  } else {
    Serial.println("There was no content/new fw in the response");
    wifiClientOTA.flush();
    return false;
  }
  wifiClientOTA.flush();
  return false;
}

void processJobDocument(DynamicJsonDocument jobReceived){
  Serial.println("Job Execution Started");

  const char* url = jobReceived["url"];
  const char* bin = jobReceived["fileName"];
  String fwVersion = jobReceived["version"].as<String>();

  // Serial.printf("url -> %s\nbin -> %s\n fwV ->%s\n", url, bin, fwVersion.c_str());
  if(execOTA(url, bin, fwVersion)){
    delay(5000);
    EEPROM.write(80, 0);
    EEPROM.commit();
    ESP.restart();
  }
  jobReceived.clear();
  //job.clear();
}