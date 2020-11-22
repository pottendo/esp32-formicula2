#ifndef __CIRCUIT_H__
#define __CIRCUIT_H__
#include <time.h>
#include "ui.h"
#include "io.h"
#include "wifi.h"

class genCircuit
{
public:
    genCircuit() = default;
    ~genCircuit() = default;

    virtual void io_set(uint8_t v, bool ign_inverse = false) = 0;
    virtual myRange<float> &get_range() = 0;
};

template <typename Sensor>
class myCircuit : public genCircuit
{
    const String circuit_name;
    Sensor &sensor;
    ioSwitch &io;
    myRange<struct tm> duty_cycle;
    myRange<float> range;
    float period;
    lv_task_t *circuit_task;
    button_label_c *button;
    slider_label_c *slider;
public:
    myCircuit(uiElements *ui, const String &n, Sensor &s, ioSwitch &i, float p, myRange<float> r, myRange<float> dr, myRange<struct tm> dc = {{0, 0, 0}, {59, 59, 23}})
        : circuit_name(n), sensor(s), io(i), duty_cycle(dc), range(r), period(p)
    {
        ui->add_control((button =
                             new button_label_c(ui->get_controls(), this, circuit_name.c_str(), 230, 48, LV_ALIGN_IN_TOP_LEFT))
                            ->get_area());
        if (sensor.get_type() == REAL_SENSOR)
        {
            ui->add_setting((slider =
                                 new slider_label_c(ui->get_settings(), this, circuit_name.c_str(), ctrl_temprange, dr, 230, 60, LV_ALIGN_IN_BOTTOM_LEFT))
                                ->get_area());
        }
        if (sensor.get_type() == JUST_SWITCH) {
            ui->add_setting((new rangeSpinbox<myRange<struct tm> >(ui->get_settings(), n.c_str(), duty_cycle, 230, 72))->get_area());
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

    inline const String &get_name(void) { return circuit_name; }
    inline myRange<float> &get_range() { return range; }
    void io_set(uint8_t v, bool ign_invers = false) override { io.set(v, ign_invers); }

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
        if (!time_obj->get_time(&t))
        {
            log_msg(circuit_name + "Failed to obtain time");
            return;
        }
        if (duty_cycle.is_in(t))
        {
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
                log_msg(circuit_name + range.to_string() + ": val=" + String(v1) + "...nothing to do.");
                //                io.toggle();
                button->set(io.state());
                return;
            }
            if (range.is_below(v1))
            {
                io.set(HIGH);
                log_msg(circuit_name + range.to_string() + ": val=" + String(v1) + "...switching " + io.to_string());
                button->set(io.state());
            }
            if (range.is_above(v1))
            {
                io.set(LOW);
                log_msg(circuit_name + range.to_string() + ": val=" + String(v1) + "...switching " + io.to_string());
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
               ", range" + range.to_string() +
               ", cycle: " + period + "s" +
               (io.is_invers() ? ", invers logic" : "");
    }
};

#endif