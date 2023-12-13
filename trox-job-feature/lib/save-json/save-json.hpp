#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>

class SaveJson {
  private:
    Preferences preference;
  public:
    void write(String id, String buffer);
    String read(String id);
};

extern SaveJson saveJson;