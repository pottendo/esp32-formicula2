
#include <WiFi.h>
#include <time.h>
#include <WebServer.h>
#include <AutoConnectOTA.h>
#include <AutoConnect.h>

#include "ui.h"
#include "wifi.h"

myTime *time_obj;

static WebServer ip_server;
static AutoConnect portal(ip_server);

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
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

static void rootPage(void)
{
    char content[] = "Formicula!";
    ip_server.send(200, "text/plain", content);
}

void setup_wifi(void)
{
    log_msg("Setting up Wifi...");
    ip_server.on("/", rootPage);
    if (portal.begin())
    {
        Serial.println("WiFi connected: " + WiFi.localIP().toString());
    }
    log_msg("done.\nSetting up local time...");
    //init and get the time
    time_obj = new myTime();

    //setup_OTA(&ip_server);
    //ip_server.begin();
}

void loop_wifi(void)
{
    //ip_server.handleClient();
}