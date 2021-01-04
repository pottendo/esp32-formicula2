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

#include <WiFi.h>
#include <time.h>
#include <WebServer.h>
#include <AutoConnectOTA.h>
#include <AutoConnect.h>
#include <ESPmDNS.h>

#include "ui.h"
#include "wifi.h"

myTime *time_obj;

#define USE_AC
#ifdef USE_AC
static WebServer ip_server;
static AutoConnect portal(ip_server);
static AutoConnectConfig config;
#endif

static const String hostname = "fcc";

myTime::myTime()
{
    mutex = xSemaphoreCreateMutex();
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    V(mutex);
    printLocalTime();
    lv_task_create(myTime::sync_clock, 1000 * 60 * 60 * 24 /* 1x daily ntp sync */, LV_TASK_PRIO_LOWEST, this);
}

void printLocalTime()
{
    struct tm timeinfo;
    int err = 10;
    while (!getLocalTime(&timeinfo) && err--)
    {
        printf("Failed to obtain time - %d\n", 10 - err);
        delay(500);
    }
    if (err <= 0) {
        // tried for 10s to get time, better reboot and try again
        log_msg("failed to obtain time...rebooting.");
        ESP.restart();
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

#ifdef USE_AC
static void rootPage(void)
{
    String body;
    body = String("Formicula Control Center - go <a href=\"http://") + WiFi.localIP().toString().c_str() + "/_ac\"> fcc admin page/a>";
    const char *content = body.c_str();
    ip_server.send(200, "text/plain", content);
}
#endif

void setup_wifi(void)
{
    log_msg("Setting up Wifi...");
#ifdef USE_AC
    ip_server.on("/", rootPage);
    config.ota = AC_OTA_BUILTIN;
    config.hostName = hostname;
    portal.config(config);
    if (portal.begin())
    {
        Serial.println("WiFi connected: " + WiFi.localIP().toString());
    }
#else
    WiFi.begin("pottendoT", "poTtendosWLAN");
    while (!WiFi.isConnected())
    {
        log_msg("WiFi connection failed, retrying...");
        delay(500);
    }
#endif
    log_msg("done.\nSetting up local time...");
    printf("fcc IP = %s, GW = %s, DNS = %s\n",
           WiFi.localIP().toString().c_str(),
           WiFi.gatewayIP().toString().c_str(),
           WiFi.dnsIP().toString().c_str());
    //WiFi.printDiag(Serial);

    //init and get the time
    time_obj = new myTime();

    if (!MDNS.begin(hostname.c_str()))
    {
        log_msg("Setup of DNS for fcc failed.");
    }
}

void loop_wifi(void)
{
    if (!WiFi.isConnected())
        log_msg("Wifi not connected ... strange");
#ifdef USE_AC
    portal.handleClient();
#endif
}