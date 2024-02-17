/*
   VE.Direct to MQTT Gateway using a ESP32 Board
   Collect Data from VE.Direct device like Victron MPPT 75/15 and send it
   to a MQTT gateway. From there you can integrate the data into any
   Home Automation Software like ioBroker annd make graphs.

   The ESP32 will read data from the VE.Direct interface and transmit the
   data via WiFi to a MQTT broker

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


/*
   All configuration comes from config.h
   So please see there for WiFi, MQTT and OTA configuration
*/
#include "config.h"

time_t last_boot;

#include "TimeLib.h"
#include "vedirectEEPROM.h"
mEEPROM pref;
#include "VEDirect.h"
#include "vedirectWiFi.h"
#include "vedirectMQTT.h"
#ifdef USE_ONEWIRE
#include "vedirectONEWIRE.h"
#endif
#ifdef USE_OTA
#include "vedirectOTA.h"
#endif



VEDirect ve;
time_t last_vedirect;


void setup() {
  Serial.begin(115200);
  // fetch previously stored parameters from EEPROM
  VE_WAIT_TIME = pref.getInt("VE_WAIT_TIME", VE_WAIT_TIME);
#ifdef USE_OTA
  OTA_WAIT_TIME = pref.getInt("OTA_WAIT_TIME", OTA_WAIT_TIME);
#endif
#ifdef USE_ONEWIRE
  OW_WAIT_TIME = pref.getInt("OW_WAIT_TIME", OW_WAIT_TIME);
#endif
  if ( startWiFi()) {
    setClock();
    last_boot = time(nullptr);
    if ( startMQTT()) {
      ve.begin();
      // looking good; moving to loop
      return;
    }
  }
  // oh oh, we did not get WiFi or MQTT, that is bad; we can't continue
  // wait a while and reboot to try again
  delay(20000);
  ESP.restart();
}

void loop() {
  VEDirectBlock_t block;
  time_t t = time(nullptr);
#ifdef USE_ONEWIRE
  if ( abs(t - last_ow) >= OW_WAIT_TIME) {
    if ( checkWiFi()) {
      sendOneWireMQTT();
      last_ow = t;
      sendOPInfo();
    }
  }
#endif
#ifdef USE_OTA
  if ( abs(t - last_ota) >= OTA_WAIT_TIME) {
    if ( checkWiFi()) {
      startOTA(SKETCH_NAME);
      last_ota = t;
    }
  }
#endif
  if ( abs( t - last_vedirect) >= VE_WAIT_TIME) {
    if ( ve.getNewestBlock(&block)) {
      last_vedirect = t;
      log_i("New block arrived; Value count: %d, serial %d", block.kvCount, block.serial);
      if ( checkWiFi()) {
        sendASCII2MQTT(&block);
        // sendOPInfo();
      }
    }
  }
  if ( ! espMQTT.loop()){
    log_i("MQTT connection lost restart");
    startMQTT();
  }
}
