#ifndef __CIRCUIT_H__
#define __CIRCUIT_H__
#include <time.h>
#include "ui.h"
#include "io.h"

class genCircuit;
extern tiny_hash_c<String, genCircuit *> circuit_objs;
extern tiny_hash_c<String, button_label_c *> button_objs;

class genCircuit
{
public:
    genCircuit() = default;
    ~genCircuit() = default;

    virtual void io_set(uint8_t v) = 0;
};

template <typename Sensor>
class myCircuit : public genCircuit
{
    const String circuit_name;
    Sensor &sensor;
    ioSwitch &io;
    myRange<struct tm> duty_cycle;
    myRange<float> &range;
    float period;
    bool invers;
    lv_task_t *circuit_task;

public:
    myCircuit(const String &n, Sensor &s, ioSwitch &i, float p, myRange<float> &r, bool inv = false, myRange<struct tm> dc = {{0, 0, 0}, {59, 59, 23}})
        : circuit_name(n), sensor(s), io(i), duty_cycle(dc), range(r), period(p), invers(inv)
    {
        circuit_task = lv_task_create(myCircuit::update_circuit, static_cast<uint32_t>(period * 1000), LV_TASK_PRIO_LOW, this);
        if (!circuit_task)
        {
            log_msg(circuit_name + ": task create failed.");
        }
        else
        {
            log_msg(this->to_string() + " ... initialized.");
        }
    };
    ~myCircuit() = default;
    inline const String &get_name(void) { return circuit_name; }
    void io_set(uint8_t v) override { io.set(v); }

    static void update_circuit(lv_task_t *t)
    {
        myCircuit<Sensor> *c = static_cast<myCircuit<Sensor> *>(t->user_data);
        c->update();
    }

    void update(void)
    {
        struct tm t;
        //log_msg(circuit_name + " - update...");
        log_msg(this->to_string());
        if (!getLocalTime(&t))
        {
            log_msg(circuit_name + "Failed to obtain time");
            return;
        }
        Serial.println(&t, "%A, %B %d %Y %H:%M:%S");
        if (duty_cycle.is_in(t))
        {
            /* we're on duty */
            float v1 = sensor.get_data();
            if (range.is_in(v1))
            {
                log_msg(circuit_name + range.to_string() + ": val=" + String(v1) + "...nothing to do.");
                io.toggle();
                button_objs.retrieve(circuit_name)->set(io.state());
                return;
            }
            if (range.is_below(v1))
            {
                uint8_t s = HIGH;
                String str{"on"};
                if (invers) { s = LOW; str=String("off"); }
                log_msg(circuit_name + range.to_string() + ": val=" + String(v1) + "...switching " + str);
                io.set(s);
                //io.toggle();
                button_objs.retrieve(circuit_name)->set(io.state());
            }
            if (range.is_above(v1))
            {
                uint8_t s = LOW;
                String str{"off"};
                if (invers) { s = HIGH; str=String("on"); }
                log_msg(circuit_name + range.to_string() + ": val=" + String(v1) + "...switching" + str);
                io.set(s);
                //io.toggle();
                button_objs.retrieve(circuit_name)->set(io.state());
            }
        }
        else
        {
            log_msg(circuit_name + " - not on duty.");
        }
    }

    inline String to_string(void)
    {
        return circuit_name + ": duty" + duty_cycle.to_string() +
               ", range" + range.to_string() +
               ", cycle: " + period + "s" + 
               (invers ? ", invers logic" : "");
    }
};

#endif