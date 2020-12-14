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

void setup_io(void) {
    Wire.begin();
}

void periodicSensor::periodic_wrapper(lv_task_t *t)
{
    periodicSensor *obj = static_cast<periodicSensor *>(t->user_data);
    obj->cb();
}

periodicSensor::periodicSensor(uiElements *ui, const String n, int period)
    : genSensor(ui, n)
{
    ticker_task = lv_task_create(periodicSensor::periodic_wrapper, period, LV_TASK_PRIO_LOW, this);
    if (!ticker_task)
        log_msg("Failed to create periodic task for sensor " + name);
}

/* Temperature DS18B20 */

myDS18B20::myDS18B20(uiElements *ui, const String n, int pin, int period)
    : periodicSensor(ui, n, period)
{
    wire = new OneWire(pin);
    temps = new DallasTemperature(wire);
    temps->begin();
}

void myDS18B20::update_data(void)
{
    temps->requestTemperatures();
    temp = temps->getTempCByIndex(0);
    if (temp != DEVICE_DISCONNECTED_C)
        log_msg(name + ":" + String(temp));
    else
        log_msg("Getting data from " + name + " failed.");
}

/* Temperature & Humidity */
myDHT::myDHT(uiElements *ui, String n, int p, int period, DHTesp::DHT_MODEL_t m)
    : multiPropertySensor(ui, n, period), pin(p), model(m), temp(-500), hum(-99)
{
    dht_obj.setup(pin, m);
}

void myDHT::update_data(void)
{
    String s = name;
    TempAndHumidity newValues = dht_obj.getTempAndHumidity();
    if (dht_obj.getStatus() != 0)
    {
        log_msg(s + "(" + get_pin() + ") - error status: " + String(dht_obj.getStatusString()));
        return;
    }
    P(mutex);
    temp = newValues.temperature;
    hum = newValues.humidity; // ((rand() % 100) - 50.0) / 20.0;
    V(mutex);
    //log_msg(String("DHT(") + get_pin() + "): Temp = " + String(temp) + ", Hum = " + String(hum));
    std::for_each(parents.begin(), parents.end(),
                  [&](avgDHT *p) { p->update_data(this); });
    // s += " " + String(temp) + "C | " + String(hum) + '%';
}

/* BME280 Sensor */
myBM280::myBM280(uiElements *ui, String n, int a, int period)
    : multiPropertySensor(ui, n, period), address(a), temp(-777), hum(-88)
{
    bme = new Adafruit_BME280;
    if (!bme->begin(address))
    {
        log_msg("failed to initialize BME280 sensor " + name);
    }
    bme_temp = bme->getTemperatureSensor();
    bme_humidity = bme->getHumiditySensor();
}

void myBM280::update_data(void)
{
    sensors_event_t temp_event, pressure_event, humidity_event;
    bme_temp->getEvent(&temp_event);
    bme_humidity->getEvent(&humidity_event);
    temp = temp_event.temperature;
    hum = humidity_event.relative_humidity;
    //log_msg(name + ": " + String(temp) + "C " + String(hum) + "%");
    // bme_pressure->getEvent(&pressure_event);
    std::for_each(parents.begin(), parents.end(),
                  [&](avgDHT *p) { p->update_data(this); });
}
