/* -*-c++-*-
 * This file is part of formicula2.
 * 
 * vice-mapper is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * vice-mapper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with vice-mapper.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <ESPAsyncWebServer.h>
#include <Update.h>
#include "ui.h"

// to update connect to http://<IP_ADDRESS>:8080/OTA
static AsyncWebServer server(8080);

static String style =
    "<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
    "input{background:#f1f1f1;border:0;padding:0 15px}body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}"
    "#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
    "#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#3498db;width:0%;height:10px}"
    "form{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
    ".btn{background:#3498db;color:#fff;cursor:pointer}</style>";

/* Login page */
static String loginIndex =
    "<form name=loginForm>"
    "<h1>Formicula Control Center Upload Login</h1>"
    "<input name=userid placeholder='User ID'> "
    "<input name=pwd placeholder=Password type=Password> "
    "<input type=submit onclick=check(this.form) class=btn value=Login></form>"
    "<script>"
    "function check(form) {"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{window.open('/serverIndex')}"
    "else"
    "{alert('Error Password or Username')}"
    "}"
    "</script>" +
    style;

/* Server Index Page */

static String serverIndex =
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
    "<h1>Formicula Control Center Select & Upload</h1>"
    "<input type='file' name='update'>"
    "<input type='submit' value='Update'>"
    "</form>"
    "<div id='prg'>progress: 0%</div>"
    "<script>"
    "$('form').submit(function(e){"
    "e.preventDefault();"
    "var form = $('#upload_form')[0];"
    "var data = new FormData(form);"
    " $.ajax({"
    "url: '/update',"
    "type: 'POST',"
    "data: data,"
    "contentType: false,"
    "processData:false,"
    "xhr: function() {"
    "var xhr = new window.XMLHttpRequest();"
    "xhr.upload.addEventListener('progress', function(evt) {"
    "if (evt.lengthComputable) {"
    "var per = evt.loaded / evt.total;"
    "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
    "}"
    "}, false);"
    "return xhr;"
    "},"
    "success:function(d, s) {"
    "console.log('success!')"
    "},"
    "error: function (a, b, c) {"
    "}"
    "});"
    "});"
    "</script>" +
    style;

void setup_OTA(void)
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send(200, "text/plain", "Welcome to the Formicula Control Center");
    });
    server.on(
        "/OTA", HTTP_GET,
        [](AsyncWebServerRequest *req) {
            req->send(200, "text/html", loginIndex);
        });
    server.on(
        "/serverIndex", HTTP_GET,
        [](AsyncWebServerRequest *req) {
            req->send(200, "text/html", serverIndex);
        });
    server.on(
        "/update", HTTP_POST,
        [](AsyncWebServerRequest *req) {
            req->send(200, "text/plain", "/update POST received.");
            if (Update.hasError())
            {
                log_msg("/update - something weng wrong");
                Update.printError(Serial);
            }
            else
            {
                log_msg("successfully flashed via OTA - rebooting...");
                ESP.restart();
            }
        },
        [](AsyncWebServerRequest *req, const String &fn, size_t index, uint8_t *data, size_t len, bool end) {
            if (!index)
            {
                //                printf("update - 2: %s, index = %d, len = %d, end = %d, data[0] = %0x\n", fn.c_str(), index, len, end, data[0]);
                log_msg("/update start upload of " + fn);
                if (!Update.begin(UPDATE_SIZE_UNKNOWN))
                {
                    Update.printError(Serial);
                    return;
                }
            }
            /* flashing firmware to ESP*/
            //                printf("upload: name=%s, fullsize=%d, current size = %d - data[0] = %0x\n", fn.c_str(), index, len, data[0]);
            if (Update.write(data, len) != len)
                Update.printError(Serial);
            if (end)
            {
                log_msg("/upload completed for " + fn + " - " + String(index + len) + "bytes.");
                if (Update.end(true))
                    log_msg("/update Success: " + fn + " - " + String(index + len) + "bytes.");
                else
                    Update.printError(Serial);
            }
        });

    server.begin();
    log_msg("Async OTA update server started...");
}

void loop_OTA(void)
{
}

#ifdef ASYNCELOTA
#include <AsyncElegantOTA.h>
static AsyncWebServer server(8080);

void setup_OTA(void)
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Hi! I am Formicula Control Center Update Server");
    });

    AsyncElegantOTA.begin(&server); // Start ElegantOTA
    server.begin();
    printf("Async OTA update server started...\n");
}

void loop_OTA(void)
{
    AsyncElegantOTA.loop();
}
#endif
#if 0

#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

const char *host = "Formicula CC";

static String style =
    "<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
    "input{background:#f1f1f1;border:0;padding:0 15px}body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}"
    "#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
    "#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#3498db;width:0%;height:10px}"
    "form{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
    ".btn{background:#3498db;color:#fff;cursor:pointer}</style>";

/* Login page */
static String loginIndex =
    "<form name=loginForm>"
    "<h1>ESP32 Login</h1>"
    "<input name=userid placeholder='User ID'> "
    "<input name=pwd placeholder=Password type=Password> "
    "<input type=submit onclick=check(this.form) class=btn value=Login></form>"
    "<script>"
    "function check(form) {"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{window.open('/serverIndex')}"
    "else"
    "{alert('Error Password or Username')}"
    "}"
    "</script>" +
    style;

/* Server Index Page */

static String serverIndex =
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
    "<input type='file' name='update'>"
    "<input type='submit' value='Update'>"
    "</form>"
    "<div id='prg'>progress: 0%</div>"
    "<script>"
    "$('form').submit(function(e){"
    "e.preventDefault();"
    "var form = $('#upload_form')[0];"
    "var data = new FormData(form);"
    " $.ajax({"
    "url: '/update',"
    "type: 'POST',"
    "data: data,"
    "contentType: false,"
    "processData:false,"
    "xhr: function() {"
    "var xhr = new window.XMLHttpRequest();"
    "xhr.upload.addEventListener('progress', function(evt) {"
    "if (evt.lengthComputable) {"
    "var per = evt.loaded / evt.total;"
    "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
    "}"
    "}, false);"
    "return xhr;"
    "},"
    "success:function(d, s) {"
    "console.log('success!')"
    "},"
    "error: function (a, b, c) {"
    "}"
    "});"
    "});"
    "</script>" + style;

void setup_OTA(WebServer *server)
{
  /*use mdns for host name resolution*/
  if (!MDNS.begin(host))
  { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  /*return index page which is stored in serverIndex */
  server->on("/OTA", HTTP_GET, [server]() {
    server->sendHeader("Connection", "close");
    server->send(200, "text/html", loginIndex);
  });
  server->on("/serverIndex", HTTP_GET, [server]() {
    server->sendHeader("Connection", "close");
    server->send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server->on(
      "/update", HTTP_POST, [server]() {
    server->sendHeader("Connection", "close");
    server->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart(); }, [server]() {
    HTTPUpload& upload = server->upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s, with size = %d\n", upload.filename.c_str(), upload.totalSize);
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      //printf("upload: name=%s, fullsize=%d, current size = %d\n", upload.filename.c_str(), upload.totalSize, upload.currentSize);
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    } });
}

void loop_OTA(void)
{
  ;
}

#endif