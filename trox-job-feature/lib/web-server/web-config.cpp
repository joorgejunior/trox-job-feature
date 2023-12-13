#include <web-config.hpp>
// #include "DNSServer.h"
#include <save-json.hpp>
// #include <Ethernet.h>
// #include <EthernetWebServer.h>

// EthernetWebServer server(80);
// WebServer wifiServer(80);
// DNSServer dnsServer;

bool isFinish = false;

bool WebConfig::start(const char * ssid, unsigned int timeout){

  // state = true;

  // Serial.print("WC: ");
  // Serial.println("OPEN AP");
  // Serial.print("WC: ");
  // Serial.print("\t- SSID: ");
  // Serial.println(ssid);
  // server.begin();
  // Serial.print("WC: ");
  // Serial.println("\t- Server initialized!");
  // Serial.print("WC: ");
  // Serial.print("\t- Connect to: ");
  // Serial.print("\t- http://");
  // Serial.println(Ethernet.localIP());

  // server.on("/", menuPage);
  // server.on("/wifi", searchPage);
  // server.on("/sendWiFi", copyPage);
  // server.on("/config", configPage);
  // server.on("/reset", resetPage);

  // unsigned long msTimeout = millis();
  // isFinish = false;

  // while(1){
  //   if(millis() - msTimeout >= timeout || isFinish){
  //     delay(1000);
  //     break;
  //   }
  //   server.handleClient();
  //   delay(1);
  // }
  // Serial.println("WC: FINISH");
  // Serial.println(WiFi.status() == WL_CONNECTED ? "WC: CONNECTED" : "WC: NOT CONNECTED");
  // return WiFi.status() == WL_CONNECTED ? true : false;
  return true;
}

void WebConfig::stop(){
  if(state){
    isFinish = true;
  }
}

// void refreshNetworks(String * page){
//   int networks = WiFi.scanNetworks();
//   String networksSaved[networks];
//   int items = 0;
//   for(int i = 0; i < networks; i++){
//     bool isEqual = false;
//     for(int v = 0; v < networks; v++){
//       if(networksSaved[v] == WiFi.SSID(i)){
//         isEqual = true;
//         break;
//       }
//     }
//     if(!isEqual){
//       items++;
//       networksSaved[i] = WiFi.SSID(i);
//       Serial.print("\nWC: ");
//       Serial.print("\t- SSID: ");
//       Serial.println(WiFi.SSID(i));
//       Serial.print("WC: ");
//       Serial.print("- RSSI: ");
//       Serial.println(WiFi.RSSI(i));
//       Serial.print("WC: ");
//       Serial.print("- Security: ");
//       Serial.println(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Open" : "Close");
//       String wifiItem = "<div onclick=\"c(this)\" class=\"wifi\"> <h3>"+WiFi.SSID(i)+"</h3> "+(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "" : IMG_WIFI)+" </div>";
//       *page += wifiItem;
//     }
//     if(items > 6)
//       break;
//   }
//   WiFi.scanDelete();
// }

// void menuPage(int option){
//   String page = FPSTR(OPEN_HEADER);
//   page += FPSTR(STYLE);
//   page += FPSTR(SCRIPT);
//   page += FPSTR(BODY_START);
//   page += FPSTR(MENU);
//   page += FPSTR(BODY_END);
//   if (option == 1)
//   server.send(200, "text/html", page);
// }

// void searchPage(){
//   String page = FPSTR(OPEN_HEADER);
//   page += FPSTR(STYLE);
//   page += FPSTR(SCRIPT);
//   page += FPSTR(BODY_START);
//   refreshNetworks(&page);
//   page += FPSTR(SEARCH_PAGE);
//   page += FPSTR(BODY_END);
//   server.send(200, "text/html", page);
// }

// void copyPage(){
//   String page = FPSTR(OPEN_HEADER);
//   page += FPSTR(STYLE);
//   page += FPSTR(SCRIPT);
//   page += FPSTR(BODY_START);
//   page += FPSTR(COPY_PAGE);
//   page += FPSTR(BODY_END);
//   server.send(200, "text/html", page);
//   delay(2000);
//   String ssid = server.arg(0);
//   String pass = server.arg(1);
//   int count = 0;
//   while(WiFi.status() != WL_CONNECTED){
//     WiFi.disconnect();
//     WiFi.begin(ssid.c_str(), pass.c_str());
//     count++;
//     vTaskDelay(1000);
//     if(count >= 10){
//       break;
//     }
//   }
//   if(count < 10){
//     WiFi.softAPdisconnect(true);
//     isFinish = true;
//   }
// }

// void configPage(){
//   String page = FPSTR(OPEN_HEADER);
//   page += FPSTR(STYLE);
//   page += FPSTR(SCRIPT);
//   page += FPSTR(BODY_START);
//   page += FPSTR(CONFIG_PAGE);
//   page += FPSTR(BODY_END);
//   server.send(200, "text/html", page);
// }

// void resetPage(){
//   String page = FPSTR(OPEN_HEADER);
//   page += FPSTR(STYLE);
//   page += FPSTR(SCRIPT);
//   page += FPSTR(BODY_START);
//   page += FPSTR(RESET_PAGE);
//   page += FPSTR(BODY_END);
//   server.send(200, "text/html", page);
//   delay(2000);
//   String topic = server.arg(0);
//   String speed = server.arg(1);
//   if(topic.length())
//     saveJson.write("Topic", topic);
//   if(speed.length())
//     saveJson.write("Speed", speed);
//   delay(1000);
//   ESP.restart();
// }