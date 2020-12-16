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

void setup_io(void)
{
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
    uint8_t address[8];
    wire = new OneWire(pin);
    temps = new DallasTemperature(wire);
    temps->begin();
    temps->requestTemperatures();
    /* temps->getDS18Count() doesn't work */
    while (wire->search(address))
    {
        no_DS18B20++;
    }
    log_msg(name + " found " + String(no_DS18B20) + " sensors.");
    if (no_DS18B20 == 0)
        error = true;
}

void myDS18B20::update_data(void)
{
    temps->begin();
    temps->requestTemperatures();

    P(mutex);
    for (int i = 0; i < no_DS18B20; i++)
    {
        all_temps[i] = temps->getTempCByIndex(i);
        if (all_temps[i] != DEVICE_DISCONNECTED_C)
        {
            error = false;
            //log_msg(name + ":" + String(all_temps[i]) + " Sensor " + String(i + 1) + "/" + String(no_DS18B20));
            mqtt_publish("fcc" + name + "-" + String(i), String(all_temps[i]));
            temp_meter->set_val(all_temps[i]);
        }
        else
        {
            log_msg("Getting data from " + name + " failed.");
            error = true;
        }
    }
    V(mutex);
    std::for_each(parents.begin(), parents.end(),
                  [&](avgSensor *p) {
                      for (int i = 0; i < no_DS18B20; i++)
                          p->add_data(all_temps[i]);
                  });
}

/* Temperature & Humidity */
myDHT::myDHT(uiElements *ui, String n, int p, DHTesp::DHT_MODEL_t m, int period)
    : multiPropertySensor(ui, n, period), pin(p), model(m), temp(-500), hum(-99)
{
    dht_obj.setup(pin, m);
}

void myDHT::update_data(void)
{
    error = false;
    TempAndHumidity newValues = dht_obj.getTempAndHumidity();
    if (dht_obj.getStatus() != 0)
    {
        log_msg(name + "(" + get_pin() + ") - error status: " + String(dht_obj.getStatusString()));
        error = true;
        return;
    }
    P(mutex);
    temp = newValues.temperature;
    hum = newValues.humidity; // ((rand() % 100) - 50.0) / 20.0;
    V(mutex);
    std::for_each(parents.begin(), parents.end(),
                  [&](genSensor *p) { p->update_data(this); });
}

/* BME280 Sensor */
myBM280::myBM280(uiElements *ui, String n, int a, int period)
    : multiPropertySensor(ui, n, period), address(a), temp(-777), hum(-88)
{
    bme = new Adafruit_BME280;
    if (!bme->begin(address))
    {
        log_msg("failed to initialize BME280 sensor " + name);
        error = true;
    }
    P(mutex);
    bme_temp = bme->getTemperatureSensor();
    bme_humidity = bme->getHumiditySensor();
    V(mutex);
}

void myBM280::update_data(void)
{
    sensors_event_t temp_event, humidity_event;
    P(mutex);
    error = false;
    bme_temp->getEvent(&temp_event);
    bme_humidity->getEvent(&humidity_event);
    temp = temp_event.temperature;
    hum = humidity_event.relative_humidity;
    if ((temp == NAN) || (hum == NAN))
        error = true;
    V(mutex);
    std::for_each(parents.begin(), parents.end(),
                  [&](genSensor *p) { p->update_data(this); });
}
