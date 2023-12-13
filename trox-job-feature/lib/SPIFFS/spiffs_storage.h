#ifndef SPIFFS_STORAGE_H
#define SPIFFS_STORAGE_H

#include <ArduinoJson.h>
#include "FS.h"
#include "SPIFFS.h"

extern bool notRead;

/**
 * @brief Start SPIFFS
 * 
 */
void initSPIFFS();
/**
 * @brief Update file
 * 
 * @param path (const char)
 * @param doc (JsonObject)
 * @return int 
 */
int updateFile(const char * path, JsonObject doc);
/**
 * @brief Read json
 * 
 * @param path (const char)
 * @return DynamicJsonDocument 
 */
DynamicJsonDocument readJson(const char * path);
/**
 * @brief Read file
 * 
 * @param path (const char)
 * @return String 
 */
String readFile(const char * path);
/**
 * @brief Update object json
 * 
 * @param oldObject (JsonObject)
 * @param newObject (JsonObject)
 */
void updateObject(JsonObject oldObject, JsonObject newObject);
/**
 * @brief Update array json
 * 
 * @param oldArray (JsonArray)
 * @param newArray (JsonArray)
 */
void updateArray(JsonArray oldArray, JsonArray newArray);
/**
 * @brief Update variant json
 * 
 * @param oldVariant (JsonVariant)
 * @param newVariant (JsonVariant)
 */
void updateVariant(JsonVariant oldVariant, JsonVariant newVariant);
/**
 * @brief List file in directory
 * 
 * @param fs (fs::FS)
 * @param dirname (const char)
 * @param levels (uint8_t)
 */
void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
/**
 * @brief Create a file in SPIFFS
 * 
 * @param path (const char)
 * @param data (const char)
 * @return bool
 */
bool createFile(const char * path, const char * data);
/**
 * @brief Delete a file in SPIFFS
 * 
 * @param path (const char)
 */
void deleteFile(const char * path);

/**
 * @param fs (fs::FS)
 * @param path (const char)
 * @param message (const char)
*/
void appendFile(const char * path, const char * message);
#endif