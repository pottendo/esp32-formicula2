#include <time.h>
#include "ui.h"
#include "circuits.h"

static tiny_hash_c<lv_obj_t *, button_label_c *> button_callbacks(10);

static void button_cb_wrapper(lv_obj_t *obj, lv_event_t e)
{
    button_callbacks.retrieve(obj)->cb(e);
}

button_label_c::button_label_c(lv_obj_t *p, const char *l, int x, int y, lv_align_t alignment)
    : parent(p), label_text(l), x_pos(x), y_pos(y)
{
    obj = lv_switch_create(parent, NULL);
    lv_obj_align(obj, NULL, LV_ALIGN_IN_TOP_RIGHT, -x, y);
    lv_obj_set_event_cb(obj, button_cb_wrapper);
    button_callbacks.store(obj, this);

    label = lv_label_create(parent, NULL);
    lv_label_set_text(label, label_text);
    lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_LEFT, x, y + 2);
    lv_obj_set_style_local_text_font(label, 0, LV_STATE_DEFAULT, &lv_font_montserrat_20);
}

void button_label_c::cb(lv_event_t e)
{
    if (e == LV_EVENT_VALUE_CHANGED)
    {
        printf("%s: %s\n", label_text, lv_switch_get_state(obj) ? "On" : "Off");
        genCircuit *c = circuit_objs.retrieve(String(label_text));
        c->io_set(lv_switch_get_state(obj) ? HIGH : LOW);
    }
}

void button_label_c::set(uint8_t v)
{
    if (v == HIGH)
        lv_switch_on(obj, LV_ANIM_ON);
    else
        lv_switch_off(obj, LV_ANIM_ON);
}

static tiny_hash_c<lv_obj_t *, slider_label_c *> slider_callbacks(10);

static void slider_cb_wrapper(lv_obj_t *obj, lv_event_t e)
{
    slider_callbacks.retrieve(obj)->cb(e);
}

slider_label_c::slider_label_c(lv_obj_t *p, const char *l, myRange<float> &ra, int w, int h, lv_align_t alignment)
    : area(p), label_text(l), ctrl_range(ra)
{
    area = lv_obj_create(p, NULL);
    lv_obj_set_size(area, w, h);

    /* slider label */
    label = lv_label_create(area, NULL);
    lv_label_set_text(label, label_text);
    lv_obj_align(label, area, LV_ALIGN_IN_TOP_LEFT, 5, 10);
    lv_obj_set_style_local_text_font(label, 0, LV_STATE_DEFAULT, &lv_font_montserrat_20);

    /* slider */
    slider = lv_slider_create(area, NULL);
    lv_slider_set_type(slider, LV_SLIDER_TYPE_RANGE);
    lv_obj_set_width(slider, w - 20);
    lv_obj_align(slider, area, LV_ALIGN_IN_TOP_RIGHT, -10, h / 2);
    lv_obj_set_event_cb(slider, slider_cb_wrapper);
    slider_callbacks.store(slider, this);
    lv_slider_set_range(slider, ctrl_range.get_lbound() * 100, ctrl_range.get_ubound() * 100);

    myRange<float> *r = (String(label_text) == String("Temperatur")) ? &def_temprange : &def_humrange;
    lv_slider_set_left_value(slider, r->get_lbound() * 100, LV_ANIM_ON);
    lv_slider_set_value(slider, r->get_ubound() * 100, LV_ANIM_ON);

    static char b[10];
    /* Create a label for ranges below the slider */
    slider_up_label = lv_label_create(area, NULL);
    snprintf(b, 6, "%.2f", r->get_ubound());
    lv_label_set_text(slider_up_label, b);
    lv_obj_set_auto_realign(slider_up_label, true);
    lv_obj_align(slider_up_label, slider, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 10);

    slider_down_label = lv_label_create(area, NULL);
    snprintf(b, 6, "%.2f", r->get_lbound());
    lv_label_set_text(slider_down_label, b);
    lv_obj_set_auto_realign(slider_down_label, true);
    lv_obj_align(slider_down_label, slider, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
}

void slider_label_c::cb(lv_event_t e)
{
    if (e == LV_EVENT_VALUE_CHANGED)
    {
        myRange<float> *r = (String(label_text) == String("Temperatur")) ? &def_temprange : &def_humrange;
        static char buf[10]; /* max 3 bytes for number plus 1 null terminating byte */
        snprintf(buf, 6, "%.2f", static_cast<float>(lv_slider_get_value(slider)) / 100);
        lv_label_set_text(slider_up_label, buf);
        r->set_ubound(static_cast<float>(lv_slider_get_value(slider)) / 100);
        snprintf(buf, 6, "%.2f", static_cast<float>(lv_slider_get_left_value(slider)) / 100);
        lv_label_set_text(slider_down_label, buf);
        r->set_lbound(static_cast<float>(lv_slider_get_left_value(slider)) / 100);
    }
}

void slider_label_c::set_label(const char *l, lv_color_t c)
{
    //lv_obj_set_style_local_text_color(label, 0, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
    lv_label_set_text(label, l);
}

/* analogMeter */

analogMeter::analogMeter(lv_obj_t *tab, const char *n, myRange<float> r, const char *u) : name(n), val(0.0), unit(u)
{
    lv_obj_t *tmp;
    area = lv_obj_create(tab, NULL);
    lv_obj_set_size(area, 230, 126);

    lmeter = lv_linemeter_create(area, NULL);
    lv_linemeter_set_range(lmeter, r.get_lbound(), r.get_ubound()); /*Set the range*/
    lv_linemeter_set_value(lmeter, val);                            /*Set the current value*/
    lv_linemeter_set_scale(lmeter, 200, 15);                        /*Set the angle and number of lines*/
    lv_obj_set_size(lmeter, 130, 130);
    /* add scale labels */
    tmp = lv_label_create(lmeter, NULL);
    char buf[10];
    sprintf(buf, "%02d%s", (int)r.get_lbound(), unit);
    lv_label_set_text(tmp, buf);
    lv_obj_align(tmp, lmeter, LV_ALIGN_OUT_BOTTOM_LEFT, 0, -50);
    tmp = lv_label_create(lmeter, NULL);
    sprintf(buf, "%02d%s", (int)r.get_ubound(), unit);
    lv_label_set_text(tmp, buf);
    lv_obj_align(tmp, lmeter, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, -50);

    lv_obj_align(lmeter, area, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *t = lv_label_create(lmeter, NULL);
    lv_label_set_text(t, name);
    lv_obj_align(t, lmeter, LV_ALIGN_IN_BOTTOM_MID, 0, -10);

    temp_label = lv_label_create(lmeter, NULL);
    set_act();
    lv_obj_align(temp_label, lmeter, LV_ALIGN_CENTER, 0, 0);
}

/* helpers */
template <>
bool myRange<struct tm>::is_above(struct tm &t)
{
    if (t.tm_hour > ubound.tm_hour)
        return true;
    else if (t.tm_hour == ubound.tm_hour)
        if (t.tm_min > ubound.tm_min)
            return true;
        else if (t.tm_min == ubound.tm_min)
            if (t.tm_sec > ubound.tm_sec)
                return true;
            else
                return false;
        else
            return false;
    else
        return false;
}

template <>
bool myRange<struct tm>::is_below(struct tm &t)
{
    if (t.tm_hour < lbound.tm_hour)
        return true;
    else if (t.tm_hour == lbound.tm_hour)
        if (t.tm_min < lbound.tm_min)
            return true;
        else if (t.tm_min == lbound.tm_min)
            if (t.tm_sec < lbound.tm_sec)
                return true;
            else
                return false;
        else
            return false;
    else
        return false;
}

template <>
const String myRange<struct tm>::to_string(void)
{
    return String("[") + String(lbound.tm_hour) + ":" + String(lbound.tm_min) + ":" + String(lbound.tm_sec) + "-" +
           String(ubound.tm_hour) + ":" + String(ubound.tm_min) + ":" + String(ubound.tm_sec) + "]";
}

template <>
const String myRange<float>::to_string(void)
{
    return String("[") + String(lbound) + "-" + String(ubound) + "]";
}

template <>
bool myRange<float>::is_above(float &v)
{
    return !(v <= ubound);
}
template <>
bool myRange<float>::is_below(float &v)
{
    return !(v >= lbound);
}