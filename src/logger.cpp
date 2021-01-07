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

#include "logger.h"
#include "ui.h"

static myLogger msg_logger("/msg", 50, 90 * 1000);
static myLogger sensor_logger("/sensors", 50);
static myLogger circuit_logger("/circuits", 50);

#if 0
void logger_task(void *arg)
{
    log_msg("logger task started...");
    while (1)
    {
        delay(500);
        msg_logger.publish(log_mqtt_client);
    }
}
#endif

#define PUBLISH_LOG
static MQTTClient *log_mqtt_client;
void setup_logger(void)
{
#ifdef PUBLISH_LOG
    log_mqtt_client = mqtt_register_logger();
    if (!log_mqtt_client)
        return;
#endif
    //TaskHandle_t handle;
    //xTaskCreate(logger_task, "logger_task", 4000, NULL, configMAX_PRIORITIES - 1, &handle);
}

#ifdef PUBLISH_LOG
void log_publish(void)
{
    msg_logger.publish(log_mqtt_client);
}
#endif
/* helpers */
void log_msg(String s, myLogger::myLog_t where, bool publish)
{
    printf("%s\n", s.c_str());
    fflush(stdout);

    myLogger *l;
    switch (where)
    {
    case myLogger::LOG_MSG:
        l = &msg_logger;
        break;
    case myLogger::LOG_SENSOR:
        l = &sensor_logger;
        break;
    case myLogger::LOG_CIRCUIT:
        l = &circuit_logger;
        break;
    default:
        return;
    }
    l->log(s, publish);
}

void log_msg(const char *s, myLogger::myLog_t where, bool publish)
{
#if 0
	P(ui_mutex);
	lv_textarea_add_text(log_handle, s);
	lv_textarea_add_char(log_handle, '\n');
	V(ui_mutex);
#endif
    log_msg(String(s), where, publish);
}

myLogger::myLogger(String n, size_t max, unsigned long p)
    : name(n), max(max), len(0), last_cycle(millis()), period(p), nr(1)
{
    mutex = xSemaphoreCreateMutex();
    V(mutex);
}

void myLogger::log(String m, bool publish)
{
    if (publish && log_mqtt_client)
    {
        log_entry_t e{0, time(nullptr), m};
        mqtt_publish("/msg", entry2String(e, false), log_mqtt_client);
        return;
    }

    P(mutex);
    if (len++ >= max)
    {
        len = max;
        msgs.erase(msgs.begin());
    }
    msgs.push_back(log_entry_t{nr++, time(nullptr), m});
    V(mutex);
}

String get_log(myLogger::myLog_t where, bool ashtml)
{
    myLogger *l;
    switch (where)
    {
    case myLogger::LOG_MSG:
        l = &msg_logger;
        break;
    case myLogger::LOG_SENSOR:
        l = &sensor_logger;
        break;
    case myLogger::LOG_CIRCUIT:
        l = &circuit_logger;
        break;
    default:
        return "";
    }
    return l->to_string(ashtml);
}
String myLogger::to_string(bool ashtml)
{
    String ret{""};
    P(mutex);
    for (auto t = msgs.begin(); t != msgs.end(); t++)
    {
        if (ashtml)
        {
            ret += "<tr>";
            ret += entry2String(*t, ashtml);
            ret += "</tr>";
        }
        else
        {
            ret += entry2String(*t, ashtml);
            ret += '\n';
        }
    }
    V(mutex);
    return ret;
}

/* should be called within a task context, as it delays to not flood mqtt over the net too quickly */
void myLogger::publish(MQTTClient *c)
{
    if (!c)
        return;
    if ((millis() - last_cycle) < period)
        return;

    if (!mqtt_connect(c))
        return;
    last_cycle = millis();

    P(mutex);
    std::list<log_entry_t> tmp = msgs;
    V(mutex);
    static unsigned long last_published = 0;

    //int siz = tmp.size();
    //int i = 0;
    for (auto t = tmp.begin(); t != tmp.end(); t++)
    {
        time_t msgt = std::get<0>(*t);
        if (msgt > last_published)
        {
#if 0            
            char buf[256];
            snprintf(buf, 256, "%d/%d - %s", i, siz, entry2String(*t, false).c_str());
            printf("XXXXXXXXXXXXXXXSending: %s\n", buf);
            c->publish(name.c_str(), buf /* entry2String(*t)*/, true, 2);
#endif
            mqtt_publish(name.c_str(), entry2String(*t, false), c);
            last_published = msgt;
            delay(250);
            //i++;
        }
    }
}

/* private functions */
String myLogger::entry2String(log_entry_t &t, bool ashtml)
{
    String ret{""};
    char *str = ctime(&(std::get<1>(t)));
    char *nl = strchr(str, '\n');
    if (nl)
        *nl = '\0';
    if (ashtml)
    {
        ret += "<td>";
        ret += str;
        ret += "</td>";
        ret += "<td>";
        ret += std::get<2>(t);
        ret += "</td>";
    }
    else
    {
        ret += str;
        ret += ": ";
        ret += std::get<2>(t);
    }
    return ret;
}
