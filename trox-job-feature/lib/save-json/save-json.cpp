#include <save-json.hpp>

void SaveJson::write(String id, String buffer){
  preference.begin(id.c_str());
  preference.putString(id.c_str(), buffer);
  preference.end();
}

String SaveJson::read(String id){
  preference.begin(id.c_str());
  String res = preference.getString(id.c_str());
  preference.end();
  return res;
}
