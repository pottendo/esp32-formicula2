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

#ifndef __logger_h__
#define __logger_h__

#include <Arduino.h>
#include <stdlib.h>
#include <utility>
#include <list>
#include <MQTT.h>

class myLogger
{
    typedef std::pair<time_t, String> log_entry_t;
    std::list<log_entry_t> msgs;
    String name;        /* used as topic prefix on mqtt */
    size_t max, len;
    SemaphoreHandle_t mutex;
    unsigned long last_cycle, period;
    unsigned long last_published;

    String entry2String(log_entry_t &e, bool asthml = true);
public:
    myLogger(String name, size_t len = 100, unsigned long period = 5 * 60 * 1000);
    ~myLogger() = default;

    void log(String m);
    String to_string(bool ashtml = true);

    void publish(MQTTClient *client);

    typedef enum
    {
        LOG_NO_LOG,
        LOG_MSG,
        LOG_SENSOR,
        LOG_CIRCUIT
    } myLog_t;
};

void log_msg(const char *s, myLogger::myLog_t where = myLogger::LOG_MSG);
void log_msg(const String s, myLogger::myLog_t where = myLogger::LOG_MSG);
String get_log(myLogger::myLog_t w, bool ashtml = true);
void setup_logger(void);
void log_publish(void);

#endif