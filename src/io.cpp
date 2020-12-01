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

#include <list>
#include "ui.h"
#include "io.h"

/* Temperature & Humidity */

static void triggerGetTemp(lv_task_t *t)
{
    //  t->resume();
    myDHT *obj = static_cast<myDHT *>(t->user_data);
    obj->update_data();
}

myDHT::myDHT(String n, int p, uiElements *u, DHTesp::DHT_MODEL_t m)
    : name(n), pin(p), ui(u), model(m), temp(-500), hum(-99)
{
    dht_obj.setup(pin, m);
    mutex = xSemaphoreCreateMutex();
    xSemaphoreGive(mutex); // Initialize to be accessible
    lv_task_t *dht_io_task = lv_task_create(triggerGetTemp, 2000, LV_TASK_PRIO_LOW, this);
    if (!dht_io_task)
    {
        log_msg("Failed to create taks for DHT sensor.");
    }

    ui_widget = lv_label_create(ui->get_tab(UI_CFG2), NULL);
    char buf[8];
    snprintf(buf, 8, "DHT(%d)", pin);
    lv_label_set_text(ui_widget, buf);
    lv_label_set_recolor(ui_widget, true);
    ui->add2ui(UI_CFG2, ui_widget);
}

void myDHT::update_data(void)
{
    String s = name;
    TempAndHumidity newValues = dht_obj.getTempAndHumidity();
    if (dht_obj.getStatus() != 0)
    {
        log_msg(s + "(" + get_pin() + ") - error status: " + String(dht_obj.getStatusString()));
        s += "#ff0000  !!! " + String(dht_obj.getStatusString()) + "#";
        lv_label_set_text(ui_widget, s.c_str());
        return;
    }
    P(mutex);
    temp = newValues.temperature;
    hum = newValues.humidity; // ((rand() % 100) - 50.0) / 20.0;
    V(mutex);
    //log_msg(String("DHT(") + get_pin() + "): Temp = " + String(temp) + ", Hum = " + String(hum));
    std::for_each(parents.begin(), parents.end(),
                  [&](avgDHT *p) { p->update_data(this); });
    s += " " + String(temp) + "C | " + String(hum) + '%';
    lv_label_set_text(ui_widget, s.c_str());
}
