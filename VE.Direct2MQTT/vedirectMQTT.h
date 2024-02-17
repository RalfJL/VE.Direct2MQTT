/*
   VE.Direct MQTT code.

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

#ifndef vedirectMQTT_H
#define vedirectMQTT_H

#include <ArduinoJson.h>
#include "PubSubClient.h"

volatile boolean mqtt_param_rec = false;    // we received a parameter via MQTT; remove it or it will be received over and over again

PubSubClient espMQTT(espClient);

boolean reconnect(const char* server, uint16_t port, const char* id, const char* user, const char* pw) {
  espMQTT.setServer(server, port);
  // Loop until we're reconnected
  int j = MQTT_MAX_RETRIES;
  while (!espMQTT.connected() && j-- > 0) {
    log_i("Attempting MQTT connection to server: %s, try: %d", server, j);
    // Attempt to connect
    if (espMQTT.connect(id, user, pw)) {
      log_i("connected");
      log_i("Subscribing to: %s", MQTT_PARAMETER.c_str());
      espMQTT.subscribe(MQTT_PARAMETER.c_str(), 1);
      return true;
    } else {
      log_e("failed, rc= %d", espMQTT.state());
      log_e(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  return false;
}

//
// Callback function; Receiver
// currently only as dummy
//
void callback(const char* topic, byte * payload, unsigned int length) {
  mqtt_param_rec = false;    // did we receive a parameter?
  char s[length + 1]; strncpy(s, (char *)payload, length); s[length] = '\0';
  log_d("MQTT callback received: %s - \"%s\"", topic, s);
  String t = String(topic);
  if ( t.equals(MQTT_PARAMETER)) {
    log_d("MQTT received new parameters");

    const int capacity = JSON_OBJECT_SIZE(10); // no clue how much data is coming in
    StaticJsonDocument<capacity> doc;
    DeserializationError err = deserializeJson(doc, s, length);
    if ( err ) {
      log_d("ERROR: deserializing MQTT message: %s", err.c_str());
      return;
    }
    JsonObject obj = doc.as<JsonObject>();
#ifdef DEBUG
    for ( JsonPair kv : obj) {
      log_d("key: %s, Value %s", kv.key().c_str(), kv.value().as<String>().c_str());
    }
#endif

    JsonVariant sleeptime = obj["VE_WAIT_TIME"];
    if ( sleeptime ) {
      mqtt_param_rec = true; // this will cause the parameter to be erased from queue
      int st = sleeptime.as<int>();
      log_d("Found member VEDIRECT_WAIT_TIME: %d", st);
      if ( VE_WAIT_TIME != st) {
        VE_WAIT_TIME = st;
        // store in EEPROM
        pref.setInt("VE_WAIT_TIME", st);
      }
    }

#ifdef USE_OTA
    JsonVariant ota_sleeptime = obj["OTA_WAIT_TIME"];
    if ( ota_sleeptime ) {
      mqtt_param_rec = true; // this will cause the parameter to be erased from queue
      int st = ota_sleeptime.as<int>();
      log_d("Found member OTA_WAIT_TIME: %d", st);
      if ( OTA_WAIT_TIME != st) {
        OTA_WAIT_TIME = st;
        // store in EEPROM
        pref.setInt("OTA_WAIT_TIME", st);
      }
    }
#endif
#ifdef USE_ONEWIRE
    JsonVariant ow_sleeptime = obj["OW_WAIT_TIME"];
    if ( ow_sleeptime ) {
      mqtt_param_rec = true; // this will cause the parameter to be erased from queue
      int st = ow_sleeptime.as<int>();
      log_d("Found member OW_WAIT_TIME: %d", st);
      if ( OW_WAIT_TIME != st) {
        OW_WAIT_TIME = st;
        // store in EEPROM
        pref.setInt("OW_WAIT_TIME", st);
      }
    }
#endif
  }
}

//
// Startup
//
boolean startMQTT() {
#ifdef USE_SSL
  espClient.setCACert(rootCACertificate);
#endif
  // receive parameter via MQTT
  log_d("MQTT callback setting");
  espMQTT.setCallback(callback);
  for ( int i = 0; i < mqtt_server_count; i++) {
    if (! espMQTT.connected()) {
      log_d("Connecting to MQTT server: %s port: %d", mqtt_server[i], mqtt_port[i]);
      if ( reconnect(mqtt_server[i], mqtt_port[i], mqtt_clientID[i], mqtt_username[i], mqtt_pw[i])) {
        return true; // done got the server
      }
    }
  }
  return false; // no server available
}

//
// end MQTT
//
boolean endMQTT() {
  espMQTT.loop();
  log_d("MQTT disconnect");
  espMQTT.disconnect();
  return true;
}

//
// Send ASCII data from passive mode to MQTT
//
boolean sendASCII2MQTT(VEDirectBlock_t * block) {
  for (int i = 0; i < block->kvCount; i++) {
    String key = block->b[i].key;
    String value = block->b[i].value;
    String topic = MQTT_PREFIX + "/" + key;
    if ( !espMQTT.connected()) {
      startMQTT();
    }
    espMQTT.loop();
    topic.replace("#", ""); // # in a topic is a no go for MQTT
    value.replace("\r\n", "");
    if ( espMQTT.publish(topic.c_str(), value.c_str())) {
      log_i("MQTT message sent succesfully: %s: \"%s\"", topic.c_str(), value.c_str());
    } else {
      log_e("Sending MQTT message failed: %s: %s", topic.c_str(), value.c_str());
    }

    if ( mqtt_param_rec ) {
      // avoid loops by sending only if we received a valid parameter
      log_i("Removing parameter from Queue: %s", MQTT_PARAMETER.c_str());
      espMQTT.publish(MQTT_PARAMETER.c_str(), "", true);
    }
  }
  return true;
}

//
// send operation info
//
boolean sendOPInfo() {
  if ( !espMQTT.connected()) {
    startMQTT();
  }
  espMQTT.loop();
  String topic = MQTT_PREFIX + "/" + "UTCBootTime";
  if ( espMQTT.publish(topic.c_str(), asctime(localtime(&last_boot)))) {
    log_i("MQTT message sent succesfully: %s: \"%s\"", topic.c_str(), asctime(localtime(&last_boot)));
  } else {
    log_e("Sending MQTT message failed: %s: %s", topic.c_str(), asctime(localtime(&last_boot)));
  }
  return true;
}



#endif
