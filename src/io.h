#ifndef __io_h__
#define __io_h__

#include <DHTesp.h>
#include <list>
#include <array>

class avgDHT; /* forware declaration */

class myDHT
{
    DHTesp dht_obj;
    int pin;
    DHTesp::DHT_MODEL_t model;
    float temp, hum;
    SemaphoreHandle_t mutex;
    lv_task_t *dht_io_task;
    std::list<avgDHT *> parents;

public:
    myDHT(int p, DHTesp::DHT_MODEL_t m = DHTesp::DHT22);
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
    uint8_t mode;
    const String on{"on"};
    const String off{"off"};

public:
    ioSwitch(uint8_t gpio, bool i = false, uint8_t m = OUTPUT) : pin(gpio), invers(i), mode(m) { pinMode(pin, m); }
    ~ioSwitch() = default;

    inline int state() { return digitalRead(pin); }
    inline int set(uint8_t n, bool ign_invers = false)
    {
        int res = state();
        if (invers && !ign_invers)
        {
            n = (n == HIGH) ? LOW : HIGH;
        }
        digitalWrite(pin, n);
        return res;
    }
    inline int toggle()
    {
        int res = state();
        set((res == LOW) ? HIGH : LOW);
        return res;
    }
    const String &to_string(void)
    {
        int s = state();
//        if (invers)
//            return ((s == HIGH) ? off : on);
        return ((s == HIGH) ? on : off);
    }
    inline bool is_invers(void) { return invers; }
};

class avgDHT
{
protected:
    static const int sample_no = 10;
    std::array<float, sample_no> data;
    int act_ind;
    std::list<myDHT *> sensors;
    const char *name;
    SemaphoreHandle_t mutex;

public:
    avgDHT(std::list<myDHT *> dhts, const char *n = "avgDHT", float def_val = 0.0) : act_ind(0), sensors(dhts), name(n)
    {
        mutex = xSemaphoreCreateMutex();
        for_each(dhts.begin(), dhts.end(),
                 [&](myDHT *dht) { dht->add_parent(this); });
        for (int i = 0; i < sample_no; i++)
            data[i] = def_val;
        V(mutex);
    }
    ~avgDHT() = default;

    virtual void update_data(myDHT *) = 0;
    virtual void update_display(float) = 0;
    const char *get_name() { return name; };
    void add_data(float d)
    {
        P(mutex);
        data[act_ind] = d;
        act_ind = ((act_ind + 1) % sample_no);
        V(mutex);
    }
    float get_data()
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
        update_display(res);
        return res;
    }
};

class tempSensor : public avgDHT
{
public:
    tempSensor(std::list<myDHT *> dhts, const char *n = "TempSensor") : avgDHT(dhts, n, 23.0) {}
    virtual ~tempSensor() = default;

    void update_data(myDHT *s) override
    {
        add_data(s->get_temp());
    }
    void update_display(float v)
    {
        temp_meter->set_val(v);
    }
};
class humSensor : public avgDHT
{
public:
    humSensor(std::list<myDHT *> dhts, const char *n = "HumSensor") : avgDHT(dhts, n, 55.5) {}
    virtual ~humSensor() = default;

    void update_data(myDHT *s) override
    {
        add_data(s->get_hum());
    }

    void update_display(float v)
    {
        hum_meter->set_val(v);
    }
};

#endif