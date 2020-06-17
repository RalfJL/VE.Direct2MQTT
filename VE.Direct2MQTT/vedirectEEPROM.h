/*
   Store alarms and preferences in NVRAM
   for Bewässerung

   Ralf Lehmann
   12.2019
*/

#ifndef VEDIRECTEEPROM_H
#define VEDIRECTEEPROM_H


// Constants
// no key larger than 15 chars



#include <Preferences.h>
#define PREF_NAME_SPACE  "VE2MQTT" //

class mEEPROM {
  public:
    mEEPROM();
    String getString(String key, String devault_value);
    String getString(int key, String devault_value);
    boolean setString(String key, String value);
    boolean setString(int key, String value);
    int32_t getInt(int key, int default_value);
    int32_t getInt(String key, int default_value);
    boolean setInt(int key, int32_t value);
    boolean setInt(String key, int32_t value);


  private:
    Preferences _preferences;
};

mEEPROM::mEEPROM() {
  // Open Preferences with my-app namespace. Each application module, library, etc
  // has to use a namespace name to prevent key name collisions. We will open storage in
  // RW-mode (second parameter has to be false).
  // Note: Namespace name is limited to 15 chars.

}

int32_t mEEPROM::getInt(String key, int default_value = 0) {
  _preferences.begin(PREF_NAME_SPACE, false);
  int32_t ret = _preferences.getInt(key.c_str(), default_value);
  log_d("PrefGetInt; \'%s\' = \'%d\'", key.c_str(),  ret);
  _preferences.end();
  return ret;
}

boolean mEEPROM::setInt(String key, int32_t value) {
  _preferences.begin(PREF_NAME_SPACE, false);
  _preferences.putInt(key.c_str(), value);
  log_d("PrefPutInt; \'%s\' = \'%d\'", key.c_str(),  value);
  _preferences.end();
  return true;
}

int32_t mEEPROM::getInt(int key, int default_value = 0) {
  return mEEPROM::getInt(String(key), default_value);
}

boolean mEEPROM::setInt(int key, int32_t value = 0) {
  return mEEPROM::setInt(String(key), value);
}

String mEEPROM::getString(int key, String default_value = ""){
  return getString(String(key), default_value);
}

String mEEPROM::getString(String key, String default_value = "") {
  _preferences.begin(PREF_NAME_SPACE, false);
  String ret = _preferences.getString(key.c_str(), default_value.c_str());
  log_d("PrefGetStr: \'%s\' = \'%s\'", key.c_str(),  ret.c_str());
  _preferences.end();
  return ret;
}

boolean mEEPROM::setString(int key, String value){
  return setString(String(key), value);
}

boolean mEEPROM::setString(String key, String value) {
  _preferences.begin(PREF_NAME_SPACE, false);
  _preferences.putString(key.c_str(), value.c_str());
  log_d("PrefSetStr: \'%s\' = \'%s\'", key.c_str(), value.c_str());
  _preferences.end();
  return true;
}


#endif
