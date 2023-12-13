#include <Arduino.h>
#include <spiffs_storage.h>
#include <ArduinoJson.h>
// #include "FS.h"
#include "SPIFFS.h"

bool notRead = false;

void initSPIFFS(){
  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS failed!");
  }
}

void updateObject(JsonObject oldObject, JsonObject newObject) {
  for (JsonPair kv : newObject){
      if(oldObject.containsKey(kv.key().c_str()))
        updateVariant(oldObject[kv.key().c_str()], kv.value());
      else
        oldObject[kv.key().c_str()] = kv.value();
  }
}

void updateArray(JsonArray oldArray, JsonArray newArray) {
  int index = 0;  
  
  if(newArray.size() == 0){
    //do nothing
  } else {
    for (auto value : newArray) {
      if(oldArray.size() > index)                                          
        updateVariant(oldArray[index], newArray[index]);
      else
        oldArray.add(newArray[index]);
      index++;                                                            
    }

    if(oldArray.size() > newArray.size()){
      index = oldArray.size() -1;
      for(index; index >= newArray.size(); index--)
        oldArray.remove(index);
    }
  }
}

void updateVariant(JsonVariant oldVariant, JsonVariant newVariant) {
    if (newVariant.is<JsonObject>())
        updateObject(oldVariant, newVariant);
    else if (newVariant.is<JsonArray>())
        updateArray(oldVariant, newVariant);
    else 
      oldVariant.set(newVariant);   
}

int updateFile(const char * path, JsonObject data){

  DynamicJsonDocument doc(4096);

  File file = SPIFFS.open(path, "r");
  if(!file || file.isDirectory()){
    if (!createFile(path, "{}")){
      Serial.println("- failed to open file for reading");
      return 0;
    }
    file = SPIFFS.open(path, "r");
    if(!file || file.isDirectory()){
      Serial.println("- failed to open file for reading");
      return 0;
    }
  }
  deserializeJson(doc, file);
  file.close();

  if (!doc.isNull()){
    JsonObject obj = doc.as<JsonObject>();
    updateObject(obj,data);

    file = SPIFFS.open(path, "w");
    serializeJson(obj, file);
    file.close();
  }
  return 1;
}

DynamicJsonDocument readJson(const char * path){
    DynamicJsonDocument doc(4096);

    File file = SPIFFS.open(path, "r");
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        notRead = true;
        return doc;
    }
    deserializeJson(doc, file);
    file.close();
    return doc;
}

String readFile(const char * path){
    // Serial.printf("Reading file: %s\r\n", path);

    File file = SPIFFS.open(path);
    if(!file || file.isDirectory()){
        // Serial.println("- failed to open file for reading");
    }
    String textFile;

    while(file.available()){
        textFile += file.readString();
    }
    file.close();
    return textFile;
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

bool createFile(const char * path, const char * data){
    File file = SPIFFS.open(path, "w");
    if(!file){
        Serial.println("- failed to open file for writing");
        return false;
    }
    if(file.print(data)){
        Serial.println("SUCCESS");
    } else {
        Serial.println("FAILED");
        return false;
    }
    file.close();
    return true;
}

void deleteFile(const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(SPIFFS.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}

void appendFile(const char * path, const char * message){
    Serial.printf("Appending to file: %s\r\n", path);

    File file = SPIFFS.open(path, FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
    if(file.println(message)){
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
    file.close();
}