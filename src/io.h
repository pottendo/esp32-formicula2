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

#ifndef __io_h__
#define __io_h__

#include <DHTesp.h>
#include <ESP32Servo.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BME280.h>
#include <list>
#include <array>
#include "ui.h"

void setup_io(void);

/* forware declaration */
class avgDHT;
class periodicSensor;
class multiPropertySensor;
class avgSensor;

typedef enum
{
    REAL_SENSOR,
    AVG_SENSOR,
    JUST_SWITCH
} sens_type_t;

class genSensor
{
protected:
    uiElements *ui;
    const String name;
    const sens_type_t type;
    SemaphoreHandle_t mutex;
    std::list<avgSensor *> parents{};
    bool error = false;

public:
    genSensor(uiElements *ui, String n, const sens_type_t t = REAL_SENSOR) : ui(ui), name(n), type(t)
    {
        mutex = xSemaphoreCreateMutex();
        V(mutex);
        if (type == REAL_SENSOR) /* don't register switch sensors (yet) */
            ui->register_sensor(this);
    }
    virtual ~genSensor() = default;

    sens_type_t get_type() { return type; }
    String get_name() { return name; };
    virtual String _to_string() = 0;
    virtual String to_string(void)
    {
        if (error)
            return name + ": #ff0000 <ERROR>";
        else
            return name + ": " + _to_string();
    }
    bool operator==(const String s)
    {
        return name == s;
    }

    virtual float get_data(void) = 0;
    virtual void add_data(float v)
    {
        P(mutex);
        _add_data(v);
        V(mutex);
    }
    virtual void _add_data(float v) = 0;
    virtual void update_data(void) = 0;
    virtual void update_data(float v) { add_data(v); }
    virtual void update_data(multiPropertySensor *s)
    {
        log_msg(name + String(": genSensor update_data callled for multiPropertySensor - shouldn't happen!!!"));
    }

    virtual void update_display(void)
    {
        if (type == REAL_SENSOR)
            ui->update_sensor(this);
    }
    virtual void publish_data(void)
    {
        mqtt_publish(to_string(), String(get_data()));
        log_msg("Sensor " + name + " updated to: " + String(get_data()));
    }

    virtual void add_parent(avgSensor *p) { parents.push_back(p); }
};

class periodicSensor : public genSensor
{
protected:
    lv_task_t *ticker_task;

public:
    periodicSensor(uiElements *ui, const String n, int period = 2000);
    virtual ~periodicSensor() = default;
    static void periodic_wrapper(lv_task_t *t);
    virtual String _to_string(void) override
    {
        float v = get_data();
        if (isnan(v))
            return String("#ff0000 <unknown>");
        return String(v);
    };
    virtual float get_data(void) override = 0;
    virtual void update_data(void) = 0;
    virtual void cb(void)
    {
        update_data();
        update_display();
    }
};

class multiPropertySensor : public periodicSensor
{

public:
    multiPropertySensor(uiElements *ui, String n, int period) : periodicSensor(ui, n, period) {}
    virtual ~multiPropertySensor() = default;
    virtual String _to_string(void) = 0;
    virtual float get_data(void) override = 0;
    virtual float get_temp(void) = 0;
    virtual float get_hum(void) = 0;
};

class avgSensor : public genSensor
{
protected:
    static const int sample_no = 12;
    std::array<float, sample_no> data;
    std::list<genSensor *> sensors;
    int act_ind;
    float cached_data = 0.0;

public:
    avgSensor(uiElements *ui, String n, std::list<genSensor *> s, float def_val = 0.0)
        : genSensor(ui, n, AVG_SENSOR), sensors(s), act_ind(0)
    {
        mutex = xSemaphoreCreateMutex();
        for_each(sensors.begin(), sensors.end(),
                 [&](genSensor *sens) { sens->add_parent(this); });
        for (int i = 0; i < sample_no; i++)
            data[i] = def_val;
        V(mutex);
    }
    ~avgSensor() = default;

    virtual void update_data(void) override
    { /* log_msg(name + ": update_data called - shouldn't happen!!!"); */
    }
    virtual String _to_string(void) { return String(cached_data); };

    // virtual void update_display(float) = 0;
    // virtual void update_display(void) = 0;
    virtual void _add_data(float d) override
    {
        data[act_ind] = d;
        act_ind = ((act_ind + 1) % sample_no);
    }
    float get_data() override
    {
        float res = 0.0;
        int crop = 0;

        std::list<float> s;
        P(mutex);
        for (size_t i = 0; i < data.size(); i++)
            if (!isnan(data[i]))
                s.push_back(data[i]);
        V(mutex);

        if (s.size() > 2)
        {
            s.sort();
            crop = (s.size() > 6) ? 2 : 1;
        }
        while (crop-- > 0)
        {
            s.pop_front();
            s.pop_back();
        }
        for (auto i = s.begin(); i != s.end(); i++)
            res = res + *i;
        res = res / s.size();
        cached_data = res;
#if 0
        printf("%s data:           [", name.c_str());
        fflush(stdout);
        for (int i = 0; i < sample_no; i++)
        {
            printf("%.2f,", data[i]);
            fflush(stdout);
        }
        printf("]\n");
        fflush(stdout);
        printf("%s sorted data:    [", name.c_str());
        fflush(stdout);
        for (auto i = s.begin(); i != s.end(); i++)
        {
            printf("%.2f,", *i);
            fflush(stdout);
        }
        printf("]\n");
#endif
        return res;
    }
};

class myDS18B20 : public periodicSensor
{
    float temp;
    OneWire *wire;
    DallasTemperature *temps;
    int pin;
    int no_DS18B20 = 0;
    float all_temps[8] = {0, 0, 0, 0, 0, 0, 0, 0};

public:
    myDS18B20(uiElements *ui, const String n, int pin, int perdiod = 2000);
    virtual ~myDS18B20() = default;

    virtual String _to_string(void)
    {
        String s;
        for (int i = 0; i < no_DS18B20; i++)
        {
            s += String(all_temps[i]);
            s += "C ";
        }
        return s;
    }
    virtual void _add_data(float v) override { temp = v; }
    virtual void update_data() override;
    virtual float get_data(void) override
    {
        float r;
        P(mutex);
        r = temp;
        V(mutex);
        return r;
    }
};

class myCapMoisture : public periodicSensor
{
    float val;
    int pin;

public:
    myCapMoisture(uiElements *ui, const String n, int p, int period = 5000)
        : periodicSensor(ui, n, period), pin(p) {}
    virtual ~myCapMoisture() = default;

    virtual void _add_data(float v) override { val = v; }
    virtual void update_data() override
    {
        P(mutex);
        val = analogRead(pin);
        V(mutex);
    }
    virtual float get_data(void) override
    {
        float r;
        P(mutex);
        r = val;
        V(mutex);
        return r;
    }
};

class myDHT : public multiPropertySensor
{
    DHTesp dht_obj;
    int pin;
    DHTesp::DHT_MODEL_t model;
    float temp, hum;

public:
    myDHT(uiElements *ui, String n, int pin, DHTesp::DHT_MODEL_t m = DHTesp::DHT22, int period = 2000);
    virtual ~myDHT() = default;

    virtual String _to_string(void) { return String(get_temp()) + "C," + String(get_hum()) + "%"; }
    virtual void update_data() override;
    inline int get_pin(void) { return pin; }
    virtual float get_data(void) { return -temp * hum; } /* dummy, never used directly here */
    virtual void _add_data(float v) { return; }          /* dummy, never used directly */
    inline float get_temp(void) { return temp; }
    inline float get_hum(void) { return hum; }
};

class myBM280 : public multiPropertySensor
{
    int address;
    float temp, hum;
    Adafruit_BME280 *bme; // use I2C interface
    Adafruit_Sensor *bme_temp;
    Adafruit_Sensor *bme_humidity;
    //Adafruit_Sensor *bme_pressure = bme.getPressureSensor();

public:
    myBM280(uiElements *ui, String n, int address = 0x76, int period = 2000);
    virtual ~myBM280() = default;

    virtual String _to_string(void) { return String(get_temp()) + "C," + String(get_hum()) + "%"; }
    virtual void update_data() override;
    virtual float get_data(void) { return temp * hum; } /* dummy, never used directly here */
    virtual void _add_data(float v) { return; }         /* dummy, never used directly */
    inline float get_temp()
    {
        float r;
        P(mutex);
        r = temp;
        V(mutex);
        return r;
    }
    inline float get_hum()
    {
        float r;
        P(mutex);
        r = hum;
        V(mutex);
        return r;
    }
};

class tempSensorMulti : public avgSensor
{
public:
    tempSensorMulti(uiElements *ui, String n, std::list<genSensor *> sensors)
        : avgSensor(ui, n, sensors, 25.0) {}
    virtual ~tempSensorMulti() = default;
    virtual void update_data(void) override { log_msg(name + ": update_data called - shouldn't happen!!!"); }
    virtual void update_data(multiPropertySensor *s) override
    {
        float v = s->get_temp();
        add_data(v);
        mqtt_publish(name, String(v));
    }

#if 0
    void update_display(float v)
    {
        temp_meter->set_val(v);
    }
#endif
};

class humSensorMulti : public avgSensor
{
public:
    humSensorMulti(uiElements *ui, String n, std::list<genSensor *> sensors)
        : avgSensor(ui, n, sensors, 65.5) {}
    virtual ~humSensorMulti() = default;
    virtual void update_data(void) override { log_msg(name + ": update_data called - shouldn't happen!!!"); }
    virtual void update_data(multiPropertySensor *s) override
    {
        float v = s->get_hum();
        add_data(v);
        mqtt_publish(name, String(v));
    }

#if 0
    void _update_data(genSensor *s) override
    {
        add_data(static_cast<multiPropertySensor *>(s)->get_hum());
        update_display();
    }

    void update_display(float v)
    {
        hum_meter->set_val(v);
    }
#endif
};

class remoteSensor : public genSensor
{
    float d;

public:
    remoteSensor(uiElements *ui, const char *n = "<RemoteSensor>", float def_val = -99.0)
        : genSensor(ui, String{n}, REAL_SENSOR), d(def_val)
    {
        mqtt_register_sensor(this);
    };
    virtual ~remoteSensor() = default;
    virtual void update_data(void) override { log_msg(name + ": update_data called - shouldn't happen!!!"); }

    virtual String _to_string(void)
    {
        float v = get_data();
        if (isnan(v))
            return String("#ff0000 <unknown>");
        return String(v);
    };

    virtual float get_data(void) override
    {
        float r;
        P(mutex);
        r = d;
        V(mutex);
        return r;
    }

    virtual void _add_data(float v)
    {
        d = v;
    }
    void update_data(float v)
    {
        add_data(v);
        //log_msg("Remote Sensor " + get_name() + " updated to " + String(get_data()));
        std::for_each(parents.begin(), parents.end(),
                      [&](avgSensor *p) {
                          p->add_data(v);
                          p->update_data();
                      });
        update_display();
    }
};

/* Digital IO switches */

class ioSwitch
{
    uint8_t pin;
    bool invers;

public:
    ioSwitch(uint8_t gpio, bool i = false) : pin(gpio), invers(i) {}
    ~ioSwitch() = default;

    virtual void _set(uint8_t pin, int val) = 0;
    virtual int _state(uint8_t pin) = 0;
    virtual const String to_string(void) = 0;
    inline int state() { return _state(pin); }
    inline int set(uint8_t n, bool ign_invers = false)
    {
        int res = state();
        if (invers && !ign_invers)
        {
            n = (n == HIGH) ? LOW : HIGH;
        }
        _set(pin, n);
        return res;
    }
    inline int toggle()
    {
        int res = state();
        set((res == LOW) ? HIGH : LOW);
        return res;
    }

    inline bool is_invers(void) { return invers; }
};

class ioDigitalIO : public ioSwitch
{
    const String on{"on"};
    const String off{"off"};

public:
    ioDigitalIO(uint8_t pin, bool i = false, uint8_t m = OUTPUT) : ioSwitch(pin, i) { pinMode(pin, m); }
    ~ioDigitalIO() = default;

    void _set(uint8_t pin, int v) override
    {
        digitalWrite(pin, v);
        //printf("digital write IO%d = %d\n", pin, v);
    }
    int _state(uint8_t pin) override
    {
        return digitalRead(pin);
    }

    const String to_string(void) override
    {
        int s = state();
        //        if (invers)
        //            return ((s == HIGH) ? off : on);
        return ((s == HIGH) ? on : off);
    }
};

class ioServo : public ioSwitch
{
    Servo *servo;
    int low, high;

public:
    ioServo(uint8_t pin, bool i = false, int l = 0, int h = 180) : ioSwitch(pin, i), low(l), high(h)
    {
        servo = new Servo;
#if 0
        ESP32PWM::allocateTimer(0);
        ESP32PWM::allocateTimer(1);
        ESP32PWM::allocateTimer(2);
        ESP32PWM::allocateTimer(3);
#endif
        //servo->setPeriodHertz(50);
        servo->attach(pin, 50, 2400);
    }

    void _set(uint8_t pin, int v) override
    {
        int val = map(v, LOW, HIGH, low, high);
        //printf("servo write IO%d = %d\n", pin, val);
        servo->write(val);
    }

    int _state(uint8_t pin) override
    {
        return servo->read();
    }

    const String to_string(void) override
    {
        return String(state());
    }
};

class timeSwitch : public genSensor
{
public:
    timeSwitch(uiElements *ui, const char *n = "<TimeSwitch>")
        : genSensor(ui, String{n}, JUST_SWITCH) {}
    ~timeSwitch() = default;

    virtual String _to_string(void) { return name; }
    virtual float get_data(void) override { return 0; }
    virtual void _add_data(float v) override { return; }
    virtual void update_data(void) override { return; }
};

#endif