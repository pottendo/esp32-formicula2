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

class avgDHT; /* forware declaration */
typedef enum
{
    REAL_SENSOR,
    JUST_SWITCH
} sens_type_t;

class genSensor
{
protected:
    uiElements *ui;
    const String name;
    const sens_type_t type;
    SemaphoreHandle_t mutex;

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
    virtual String to_string(void) { return name; };
    bool operator==(const String s)
    {
        return name == s;
    }

    virtual float get_data(void) = 0;
    virtual void _add_data(float v) = 0;
    virtual void update_data(void) = 0;
    virtual void add_data(float v)
    {
        P(mutex);
        _add_data(v);
        V(mutex);
    }
    virtual void update_data(float v)
    {
        log_msg("genSensor " + name + " updated to: " + String(get_data()));
        add_data(v);
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
};

class periodicSensor : public genSensor
{
protected:
    lv_task_t *ticker_task;

public:
    periodicSensor(uiElements *ui, const String n, int period = 2000);
    virtual ~periodicSensor() = default;
    static void periodic_wrapper(lv_task_t *t);
    virtual float get_data(void) override = 0;
    virtual void update_data(void) override = 0;
    virtual void cb(void) = 0;
};

class myDS18B20 : public periodicSensor
{
    float temp;
    OneWire *wire;
    DallasTemperature *temps;
    int pin;

public:
    myDS18B20(uiElements *ui, const String n, int pin, int perdiod = 2000);
    virtual ~myDS18B20() = default;

    virtual void _add_data(float v) override {}
    virtual void update_data();
    virtual void cb(void) override
    {
        update_data();
        publish_data();
    }
    virtual float get_data(void) override { return temp; }
};

class myCapMoisture : public periodicSensor
{
    float val;
    int pin;

public:
    myCapMoisture(uiElements *ui, const String n, int p, int period = 5000)
        : periodicSensor(ui, n, period), pin(p) {}
    virtual ~myCapMoisture() = default;

    virtual void update_data()
    {
        val = analogRead(pin);
    }
    virtual void cb(void) override
    {
        update_data();
        publish_data();
    }
    virtual float get_data(void) override { return val; }
};

class multiPropertySensor : public periodicSensor
{

public:
    multiPropertySensor(uiElements *ui, String n, int period) : periodicSensor(ui, n, period) {}
    virtual ~multiPropertySensor() = default;
    virtual float get_data(void) override = 0;
    virtual void cb(void) = 0;
    virtual float get_temp(void) = 0;
    virtual float get_hum(void) = 0;
    virtual void add_parent(avgDHT *p) = 0;
};

class myDHT : public multiPropertySensor
{
    DHTesp dht_obj;
    int pin;
    DHTesp::DHT_MODEL_t model;
    float temp, hum;
    std::list<avgDHT *> parents;

public:
    myDHT(uiElements *ui, String n, int pin, int period = 2000, DHTesp::DHT_MODEL_t m = DHTesp::DHT22);
    virtual ~myDHT() = default;

    virtual void cb(void) override { update_data(); }
    inline int get_pin(void) { return pin; }
    virtual float get_data(void) { return temp * hum; } /* dummy, never used directly here */
    inline float get_temp(void) { return temp; }
    inline float get_hum(void) { return hum; }

    void update_data();
    virtual void add_parent(avgDHT *p) { parents.push_back(p); }
};

class myBM280 : public multiPropertySensor
{
    int address;
    float temp, hum;
    Adafruit_BME280 *bme; // use I2C interface
    Adafruit_Sensor *bme_temp;
    Adafruit_Sensor *bme_humidity;
    //Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
    std::list<avgDHT *> parents;

public:
    myBM280(uiElements *ui, String n, int address, int period = 2000);
    virtual ~myBM280() = default;

    virtual void cb(void) override { update_data(); }
    virtual float get_data(void) { return temp * hum; } /* dummy, never used directly here */
    inline float get_temp() { return temp; }
    inline float get_hum() { return hum; }

    void update_data();
    virtual void add_parent(avgDHT *p) { parents.push_back(p); }
};

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

    virtual float get_data(void) override { return 0; }
    virtual void _add_data(float v) override { return; }
    virtual void update_data(void) override { return ;}
};

class avgDHT : public genSensor
{
protected:
    static const int sample_no = 10;
    std::array<float, sample_no> data;
    int act_ind;
    std::list<multiPropertySensor *> sensors;

public:
    avgDHT(uiElements *ui, const sens_type_t t, std::list<multiPropertySensor *> dhts, const char *n = "avgDHT", float def_val = 0.0)
        : genSensor(ui, String{n}, t), act_ind(0), sensors(dhts)
    {
        mutex = xSemaphoreCreateMutex();
        for_each(dhts.begin(), dhts.end(),
                 [&](multiPropertySensor *dht) { dht->add_parent(this); });
        for (int i = 0; i < sample_no; i++)
            data[i] = def_val;
        V(mutex);
    }
    ~avgDHT() = default;

    virtual void update_data(multiPropertySensor *) = 0;
    //    virtual void update_display(float) = 0;
    virtual void _add_data(float d) override
    {
        P(mutex);
        data[act_ind] = d;
        act_ind = ((act_ind + 1) % sample_no);
        V(mutex);
    }
    float get_data() override
    {
        float res = 0.0;
        P(mutex);
        std::array<float, sample_no> s = data;
        V(mutex);
        std::sort(s.begin(), s.end());
#if 0
        printf("data:           [");
        for (int i = 0; i < sample_no; i++) {
            printf("%.2f,", data[i]);
        }
        printf("]\n");
        printf("sorted data:    [");
        for (int i = 0; i < sample_no; i++) {
            printf("%.2f,", s[i]);
        }
        printf("]\n");
#endif
        for (int i = 2; i < sample_no - 2; i++)
            res = res + s[i];
        res = res / (sample_no - 4);
        //        update_display(res);
        return res;
    }
};

class tempSensor : public avgDHT
{
public:
    tempSensor(uiElements *ui, std::list<multiPropertySensor *> dhts, const char *n = "<LocalTempSensor>")
        : avgDHT(ui, REAL_SENSOR, dhts, n, 25.0) {}
    virtual ~tempSensor() = default;

    void update_data(multiPropertySensor *s) override
    {
        add_data(s->get_temp());
        update_display();
    }
#if 0
    void update_display(float v)
    {
        temp_meter->set_val(v);
    }
#endif
};

class humSensor : public avgDHT
{
public:
    humSensor(uiElements *ui, std::list<multiPropertySensor *> dhts, const char *n = "<LocalHumSensor>")
        : avgDHT(ui, REAL_SENSOR, dhts, n, 65.5) {}
    virtual ~humSensor() = default;

    void update_data(multiPropertySensor *s) override
    {
        add_data(s->get_hum());
        update_display();
    }
#if 0
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

    virtual void update_data(void) override {}   /* FIXME use from mqtt input */
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
        log_msg("Remote Sensor " + get_name() + " updated to " + String(get_data()));
        update_display();
        temp_meter->set_val(27.0);
        hum_meter->set_val(77.0);
    }
};

/**********************************************88
class myDHT
{
    DHTesp dht_obj;
    const String name;
    int pin;
    uiElements *ui;
    DHTesp::DHT_MODEL_t model;
    float temp, hum;
    SemaphoreHandle_t mutex;
    lv_task_t *dht_io_task;
    std::list<avgDHT *> parents;
    lv_obj_t *ui_widget;

public:
    myDHT(String n, int p, uiElements *ui, DHTesp::DHT_MODEL_t m = DHTesp::DHT22);
    ~myDHT() = default;

    inline int get_pin(void) { return pin; }
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

    void update_data();
    inline void add_parent(avgDHT *p)
    {
        P(mutex);
        parents.push_back(p);
        V(mutex);
    }
};

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
        servo->setPeriodHertz(50);
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

typedef enum
{
    REAL_SENSOR,
    JUST_SWITCH
} sens_type_t;

class genSensor
{
    uiElements *ui;
    const sens_type_t type;
    String name;

protected:
    SemaphoreHandle_t mutex;

public:
    genSensor(uiElements *ui, const sens_type_t t = REAL_SENSOR, String n = "<NONAME>") : ui(ui), type(t), name(String{n})
    {
        mutex = xSemaphoreCreateMutex();
        V(mutex);
        if (type == REAL_SENSOR) XXX / * don't register switch sensors (yet) * /
            ui->register_sensor(this);
    }
    ~genSensor() = default;

    sens_type_t get_type() { return type; }
    String get_name() { return name; };
    bool operator==(const String s)
    {
        return name == s;
    }
    virtual float get_data(void) = 0;
    virtual void _add_data(float v) = 0;
    virtual void add_data(float v)
    {
        P(mutex);
        _add_data(v);
        V(mutex);
    }
    virtual void update_data(float v)
    {
        add_data(v);
    }
    virtual void update_display(void)
    {
        if (type == REAL_SENSOR)
            ui->update_sensor(this);
    }
};
**********/

#endif