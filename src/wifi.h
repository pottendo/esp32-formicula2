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

#ifndef __WIFI_H__
#define __WIFI_H__

void printLocalTime();

class myTime
{
    const char *ntpServer = "pool.ntp.org";
    const long gmtOffset_sec = 1 * 3600;
    const int daylightOffset_sec = 0;
    SemaphoreHandle_t mutex;

public:
    myTime();
    ~myTime() = default;

    static void sync_clock(lv_task_t *task)
    {
        myTime *t = static_cast<myTime *>(task->user_data);
        t->sync_time();
    }

    void sync_time(void)
    {
        P(mutex);
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        V(mutex);
        log_msg("NTP sync done.");
        printLocalTime();
    }
    inline bool get_time(struct tm *t)
    {
        bool res;
        //P(mutex);
        res = getLocalTime(t);
        //V(mutex);
        return res;
    }
};

extern myTime *time_obj;

#endif