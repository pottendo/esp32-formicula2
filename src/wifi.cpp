
#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>
#include <time.h>

#include "ui.h"

static WebServer server;
static AutoConnect portal(server);
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 1 * 3600;
const int daylightOffset_sec = 0;

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
    server.send(200, "text/plain", content);
}

void setup_wifi(void)
{
    log_msg("Setting up Wifi...");
    server.on("/", rootPage);
    if (portal.begin())
    {
        Serial.println("WiFi connected: " + WiFi.localIP().toString());
    }
    log_msg("done.\nSetting up local time...");
    //init and get the time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    printLocalTime();
}

void loop_wifi(void)
{
    portal.handleClient();
}