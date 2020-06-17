/*
   VE.Direct Protocol.

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

#ifndef VEDIRECT_H
#define VEDIRECT_H

#include <StringSplitter.h>
#include "vedirectSerial.h"

typedef struct VEDirectKeyValue_t {
  String key;
  String value;
};

typedef struct VEDirectBlock_t {
  VEDirectKeyValue_t b[MAX_KEY_VALUE_COUNT];
  int kvCount;
  int serial;
};




class VEDirect {
  public:
    void begin();
    boolean addToASCIIBlock(String s);
    boolean getNewestBlock(VEDirectBlock_t *b);


  private:

    int _serial = 0; // Serial number of the block
    boolean _endOfASCIIBlock(StringSplitter *s);
    void _increaseNewestBlock();
    int _getNewestBlock();
    VEDirectBlock_t block[MAX_BLOCK_COUNT];
    portMUX_TYPE _new_block_mutex = portMUX_INITIALIZER_UNLOCKED;  // mutex to protect _newest_block
    volatile int _newest_block = -1;     // newest complete block; ready for consumption
    volatile boolean _has_new_block = false;
    int _incoming_block = 0;   // block currently filled; candidate for next newest block
    int _incoming_keyValueCount = 0;
    TaskHandle_t tHandle = NULL;

};

/*
   This task will collect the data from Serial1 and place them in a block buffer
   The task is independant from the main task so it should not loose data because
   of MQTT reconnects or other time consuming duties in the main task
*/
void serialTask(void * pointer) {
  VEDirect *ve = (VEDirect *) pointer;
  String data;
  startVEDirectSerial();
  while ( true ) {
    if ( Serial1.available()) {
      //      char s = Serial1.read();
      //      log_d("Read: %c:%d", s, s);
      //      if ( s == '\n') {
      //        log_d("received start");
      // begin of a datafield or frame
      data = Serial1.readStringUntil('\n'); // read label and value
      //log_d("Received Data: \"%s\"", data.c_str());
      if ( data.length() > 0) {
        data.replace("\r\n", ""); // Strip carriage return newline; not part of the data
        ve->addToASCIIBlock(data);
        log_d("Stack free: %5d", uxTaskGetStackHighWaterMark(NULL));
      }
      //}
    } else {
      // no serial data available; have a nap
      delay(1);
    }
  }
}




void VEDirect::begin() {
  _newest_block = -1;
  _incoming_block = _incoming_keyValueCount = 0;
  // create a task to handle the serial input
  xTaskCreate(
    serialTask,     /* Task function. */
    "serialTask",  /* String with name of task. */
    10000,            /* Stack size in bytes. */
    this,             /* Parameter passed as input of the task */
    1,                /* Priority of the task. */
    &tHandle);        /* Task handle. */
}

void VEDirect::_increaseNewestBlock() {
  taskENTER_CRITICAL(&_new_block_mutex);
  _newest_block++;
  _newest_block %= MAX_BLOCK_COUNT;
  taskEXIT_CRITICAL(&_new_block_mutex);
  _has_new_block = true;
}

int VEDirect::_getNewestBlock() {
  int n;
  taskENTER_CRITICAL(&_new_block_mutex);
  n = _newest_block;
  taskEXIT_CRITICAL(&_new_block_mutex);
  return n;
}

boolean VEDirect::_endOfASCIIBlock(StringSplitter *s) {
  if ( s->getItemAtIndex(0).equals("Checksum")) {
    // To Do: checksum test
    return true;
  }
  return false;
}

boolean VEDirect::addToASCIIBlock(String s) {
  StringSplitter sp = StringSplitter(s, '\t', 2);
  String historical = "";
  if ( sp.getItemCount() == 2) { // sometime checksum has historical data attached
    log_d("Received Key/value: \"%s\":\"%s\"", sp.getItemAtIndex(0).c_str(), sp.getItemAtIndex(1).c_str());
    if ( _incoming_keyValueCount < MAX_KEY_VALUE_COUNT) {
      block[_incoming_block].b[_incoming_keyValueCount].key = sp.getItemAtIndex(0);
      if ( sp.getItemAtIndex(0).equals("Checksum")) {
        char s[10];
        sprintf(s, "%02x", sp.getItemAtIndex(1).charAt(0));
        block[_incoming_block].b[_incoming_keyValueCount].value = String(s);
        historical = sp.getItemAtIndex(1).substring(1);
      } else {
        block[_incoming_block].b[_incoming_keyValueCount].value = sp.getItemAtIndex(1);
      }
      block[_incoming_block].kvCount = ++_incoming_keyValueCount;
      if ( historical.length() > 1) {
        // historical data
        block[_incoming_block].b[_incoming_keyValueCount].key = "Historical";
        block[_incoming_block].b[_incoming_keyValueCount].value = historical;
        log_d("Historical data:\"%s\"", historical.c_str());
        block[_incoming_block].kvCount = ++_incoming_keyValueCount;
      }
    } else {
      // buffer full but not end of frame
      // so this is not a good frame -> delete frame
      _incoming_keyValueCount = 0;
      return false; // buffer full
    }
    if ( _endOfASCIIBlock(&sp)) {
      // good frame; increase newest frame pointer
      block[_incoming_block].serial = _serial++;
      _increaseNewestBlock();
      _incoming_keyValueCount = 0;
      _incoming_block++;
      _incoming_block %= MAX_BLOCK_COUNT;
      return true;
    }
    // not the end of a frame yet; continue
    return false;
  } else {
    log_e("Receviced Data not correct: \"%s\"", s.c_str());
    //delete splitter;
    return false;
  }
}

boolean VEDirect::getNewestBlock(VEDirectBlock_t *b) {
  if ( ! _has_new_block) {
    return false;
  }
  _has_new_block = false;
  int block_num = _getNewestBlock();
  int kv_count = block[block_num].kvCount;
  b->kvCount = kv_count;
  int ser = block[block_num].serial;
  b->serial = ser;
  for (int i = 0; i < kv_count; i++) {
    b->b[i] = block[block_num].b[i];
  }
  return true;
}



#endif
