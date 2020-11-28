#ifndef __CIRCUIT_H__
#define __CIRCUIT_H__
#include <time.h>
#include "ui.h"
#include "io.h"
#include "wifi.h"

class genCircuit
{
protected:
    const String circuit_name;

public:
    genCircuit(const String &n) : circuit_name(n) {}
    ~genCircuit() = default;

    inline const String &get_name(void) { return circuit_name; }
    virtual void io_set(uint8_t v, bool ign_inverse = false) = 0;
    virtual myRange<float> &get_range(bool) = 0;
};

template <typename Sensor>
class myCircuit : public genCircuit
{
    uiElements *ui;
    Sensor &sensor;
    ioSwitch &io;
    myRange<struct tm> duty_cycle;
    myRange<float> range_day;
    myRange<float> range_night;
    float period;
    lv_task_t *circuit_task;
    button_label_c *button;
    slider_label_c *slider_day, *slider_night;

public:
    myCircuit(uiElements *ui, const String &n, Sensor &s, ioSwitch &i, float p, myRange<float> rday, myRange<float> rnight, myRange<float> dr, myRange<struct tm> dc = {{0, 0, 0}, {0, 0, 24}})
        : genCircuit(n), ui(ui), sensor(s), io(i), duty_cycle(dc), range_day(rday), range_night(rnight), period(p)
    {
        ui->add2ui(UI_CTRLS, (button =
                                  new button_label_c(ui, UI_CTRLS, this, 200, 48))
                                 ->get_area());
        if (sensor.get_type() == REAL_SENSOR)
        {
            ui->add2ui(UI_CFG1, (slider_day =
                                     new slider_label_c(ui, UI_CFG1, this, range_day, dr, 200, 50))
                                    ->get_area(), 0, 10);
            ui->add2ui(UI_CFG1, (slider_night =
                                     new slider_label_c(ui, UI_CFG1, this, range_night, dr, 200, 50, false))
                                    ->get_area());
        }
        if (sensor.get_type() == JUST_SWITCH)
        {
            ui->add2ui(UI_CFG2, (new rangeSpinbox<myRange<struct tm>>(ui, UI_CFG2, n.c_str(), duty_cycle, 230, 72))->get_area());
        }

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

    inline myRange<float> &get_range(bool day = true) override { return day ? range_day : range_night; }
    void io_set(uint8_t v, bool ign_invers = false) override { io.set(v, ign_invers); }

    static void update_circuit(lv_task_t *t)
    {
        myCircuit<Sensor> *c = static_cast<myCircuit<Sensor> *>(t->user_data);
        c->update();
    }

    void update(void)
    {
        if (ui->check_manual())
            return; /* don't do anything in manual mode */
        struct tm t;
        //log_msg(circuit_name + " - update...");
        log_msg(this->to_string());
        if (!time_obj->get_time(&t))
        {
            log_msg(circuit_name + "Failed to obtain time");
            return;
        }
        if (duty_cycle.is_in(t))
        {
            myRange<float> &range = get_range(def_day.is_in(t));
            /* we're on duty */
            if (sensor.get_type() == JUST_SWITCH)
            {
                log_msg(circuit_name + range.to_string() + " switching on - " + String(io.state()));
                io.set(HIGH, true); // force switching on
                button->set(io.state());
                return;
            }
            float v1 = sensor.get_data();
            if (range.is_in(v1))
            {
                log_msg(circuit_name + ": " + (def_day.is_in(t) ? "day" : "night") +
                        range.to_string() + ": val=" + String(v1) + "...nothing to do.");
                //                io.toggle();
                button->set(io.state());
                return;
            }
            if (range.is_below(v1))
            {
                io.set(HIGH);
                log_msg(circuit_name + ": " + (def_day.is_in(t) ? "day" : "night") +
                        range.to_string() + ": val=" + String(v1) + "...switching " + io.to_string());
                button->set(io.state());
            }
            if (range.is_above(v1))
            {
                io.set(LOW);
                log_msg(circuit_name + ": " + (def_day.is_in(t) ? "day" : "night") +
                        range.to_string() + ": val=" + String(v1) + "...switching " + io.to_string());
                button->set(io.state());
            }
        }
        else
        {
            io_set(LOW, true); // force off if circuit is not on duty
            button->set(io.state());
            log_msg(circuit_name + " - not on duty, switching off");
        }
    }

    inline String to_string(void)
    {
        return circuit_name + ": duty" + duty_cycle.to_string() +
               ", range_day" + range_day.to_string() +
               ", range_night" + range_night.to_string() +
               ", cycle: " + period + "s" +
               (io.is_invers() ? ", invers logic" : "");
    }
};

#endif