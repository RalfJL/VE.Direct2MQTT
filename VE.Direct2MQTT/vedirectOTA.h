/*
   VE.Direct Over The Air update code.

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

#ifndef VEDIRECTOTA_H
#define VEDIRECTOTA_H

#include <Update.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>



//// Utility to extract header value from headers
//String getHeaderValue(String header, String headerName) {
//  return header.substring(strlen(headerName.c_str()));
//}

//void printSSLError() {
//  char s[100];
//  int e = espClient.lastError(s, 100);
//  D("SSL Error: " + String(e) + " "); Dln(s);
//  //  int err;
//  //  br_ssl_client_context sc;
//  //  err = br_ssl_engine_last_error(&sc.eng);
//}

boolean splitURL(String * result, String url) {
  int col_pos = url.indexOf(':');
  if ( col_pos < 0 ) {
    return false;
  }
  result[0] = url.substring(0, col_pos);
  //  Dln("Proto: " + result[0]);
  if ( url.charAt(col_pos + 1) != '/' || url.charAt(col_pos + 2) != '/') {
    return false;
  }
  String rest = url.substring(col_pos + 3 ); // strip // immediately
  //  Dln("Rest:" + rest);
  // check if we have a user name and password
  int at_pos = rest.indexOf('@');
  if ( at_pos > 0 ) {
    // process user:password
    result[3] = rest.substring(0, at_pos);
    rest = rest.substring(at_pos + 1);
  }
  int slash_pos = rest.indexOf('/');
  if ( slash_pos < 0 ) {
    return false;
  }
  result[1] = rest.substring(0, slash_pos);
  result[2] = rest.substring(slash_pos);

  return true;
}



//
// make sure that there is plenty of memory, otherwise SSL will fail
// because bin files contain passwords SSL is needed
// e.g. stop BLE and release the memory
//
bool startOTA(String sketch_name) {
  log_d("Free Mem: %d", heap_caps_get_free_size(MALLOC_CAP_8BIT));
  log_d("Starting OTA with sketch: %s", sketch_name.c_str());

  // The line below is optional. It can be used to blink the LED on the board during flashing
  // The LED will be on during download of one buffer of data from the network. The LED will
  // be off during writing that buffer to flash
  // On a good connection the LED should flash regularly. On a bad connection the LED will be
  // on much longer than it will be off. Other pins than LED_BUILTIN may be used. The second
  // value is used to put the LED on. If the LED is on with HIGH, that value should be passed
  // httpUpdate.setLedPin(LED_BUILTIN, HIGH);

  for (int i = 0; i < ota_server_count; i++) {
    t_httpUpdate_return ret = httpUpdate.update(espClient, String(ota_server_string[i]) + sketch_name);
    // Or:
    //t_httpUpdate_return ret = httpUpdate.update(client, "server", 443, "file.bin");

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        log_d("HTTP_UPDATE_FAILED Error %d - %s", httpUpdate.getLastError(),  httpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        log_d("HTTP_UPDATE_NO_UPDATES");
        return true;
        break;

      case HTTP_UPDATE_OK:
        log_d("HTTP_UPDATE_OK");
        return true;
        break;
    }
  }

  return true;
}


#endif
