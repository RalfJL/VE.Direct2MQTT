/*
   VE.Direct OneWire temperature sensors code.

   GITHUB Link

   MIT License

   Copyright (c) 2020 Ralf Lehmann


   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#ifndef VEDIRECTONEWIRE_H
#define VEDIRECTONEWIRE_H

#include <OneWire.h>
#include <DallasTemperature.h>

#define MAX_DS18SENSORS 3
#define MAX_MEASSUREMENTS 3

OneWire oneWire(ONEWIRE_PIN);
DallasTemperature sensors(&oneWire);
int deviceCount = 0;
boolean onewire_good_values = false;

String addr2String(DeviceAddress addr) {
  String s;
  for (int i = 0; i < 7; i++) {
    s += String(addr[i]) + ":";
  }
  return s += addr[7];
}

boolean meassureOneWire() {
  log_d("Start");
  int m_count = MAX_MEASSUREMENTS;
  do {
    m_count --;
    onewire_good_values = true;
    sensors.begin();
    sensors.setWaitForConversion(true);
    delay(1000);
    sensors.requestTemperatures();
    delay(1000);
    deviceCount = sensors.getDeviceCount();
    log_d("Found %d devices", deviceCount);
    for (int i = 0; i < deviceCount; i++) {
      float temp = sensors.getTempCByIndex(i);
      if ( temp > 200 || temp < -80 ) {
        onewire_good_values = false;
      }
    }
  } while (m_count >= 0 && !onewire_good_values);
  log_d("End");
  return true;
}

boolean sendOneWireMQTT() {
  log_d("Start");
  meassureOneWire();

  log_d("Found %d One wire temp sensors", deviceCount);
  float c[deviceCount];

  if ( !onewire_good_values || deviceCount <= 0) {
    log_d("No ONE Wire temp sensors found; END");
    return false;
  }

  for (int i = 0; i < deviceCount; i++) {
    DeviceAddress addr;
    sensors.getAddress((uint8_t *)&addr, (uint8_t) i);
    c[i] = sensors.getTempCByIndex(i);
    log_d("DS18: %s, Temp: %f", addr2String(addr).c_str(), c[i]);
  }

  if ( !espMQTT.connected()) {
    startMQTT();
  }

  StaticJsonDocument<300> doc;
  // Add values in the document
  //
  int count = deviceCount > MAX_DS18SENSORS ? MAX_DS18SENSORS : deviceCount;
  log_d("Sending: %d Devices", count);
  for (int i = 0; i < count; i++) {
    DeviceAddress addr;
    sensors.getAddress((uint8_t *)&addr, (uint8_t) i);
    String s = addr2String(addr);
    doc[s] = c[i];
  }  char s[300];
  serializeJson(doc, s);
  if ( espMQTT.publish(MQTT_ONEWIRE.c_str(), s)) {
    log_d("Sending OneWire Data: %s - OK", s);
  } else {
    log_d("Sending OneWire %s Data: %s - ERROR", MQTT_ONEWIRE.c_str(), s);
  }
  espMQTT.loop();
  if ( mqtt_param_rec ) {
    // avoid loops by sending only if we received a valid parameter
    log_i("Removing parameter from Queue: %s", MQTT_PARAMETER.c_str());
    espMQTT.publish(MQTT_PARAMETER.c_str(), "", true);
  }
  log_d("End");
  return true;
}


#endif
