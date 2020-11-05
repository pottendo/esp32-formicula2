#include "ui.h"

static tiny_hash_c<lv_obj_t *, button_label_c *> button_callbacks(10);

static void button_cb_wrapper(lv_obj_t *obj, lv_event_t e)
{
    button_callbacks.retrieve(obj)->cb(e);
}

button_label_c::button_label_c(const char *l, int x, int y, lv_align_t alignment)
    : label_text(l), x_pos(x), y_pos(y)
{
    obj = lv_switch_create(lv_scr_act(), NULL);
    lv_obj_align(obj, NULL, LV_ALIGN_IN_TOP_RIGHT, -x, y);
    lv_obj_set_event_cb(obj, button_cb_wrapper);
    button_callbacks.store(obj, this);

    label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(label, label_text);
    lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_LEFT, x, y + 2);
    lv_obj_set_style_local_text_font(label, 0, LV_STATE_DEFAULT, &lv_font_montserrat_20);
}

void button_label_c::cb(lv_event_t e)
{
    if (e == LV_EVENT_VALUE_CHANGED)
    {
        printf("%s: %s\n", label_text, lv_switch_get_state(obj) ? "On" : "Off");
    }
}

static tiny_hash_c<lv_obj_t *, slider_label_c *> slider_callbacks(10);

static void slider_cb_wrapper(lv_obj_t *obj, lv_event_t e)
{
    slider_callbacks.retrieve(obj)->cb(e);
}

slider_label_c::slider_label_c(const char *l, int x, int y, int mi, int ma, int w, lv_align_t alignment)
    : label_text(l), x_pos(x), y_pos(y), min(mi), max(ma), width(w)
{

    lv_obj_t *rect;

    rect = lv_obj_create(lv_scr_act(), NULL);
    lv_obj_set_size(rect, 220, 48);
    lv_obj_align(rect, NULL, alignment, x, y + 24);

    slider = lv_slider_create(lv_scr_act(), NULL);
    lv_slider_set_type(slider, LV_SLIDER_TYPE_RANGE);
    lv_obj_set_width(slider, width);
    lv_obj_align(slider, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, -x, y);
    lv_obj_set_event_cb(slider, slider_cb_wrapper);
    slider_callbacks.store(slider, this);
    lv_slider_set_range(slider, min*100, max*100);
    int t1 = (max - min)*100 * 0.25 + min*100;
    int t2 = (max - min)*100 * 0.75 + min*100;
    static char b[10];
    lv_slider_set_left_value(slider, t1, LV_ANIM_ON);
    lv_slider_set_value(slider, t2, LV_ANIM_ON);

    /* Create a label below the slider */
    slider_up_label = lv_label_create(lv_scr_act(), NULL);
    snprintf(b, 6, "%.2f", static_cast<float>(t2) / 100);
    lv_label_set_text(slider_up_label, b);
    lv_obj_set_auto_realign(slider_up_label, true);
    lv_obj_align(slider_up_label, slider, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 10);
    slider_down_label = lv_label_create(lv_scr_act(), NULL);
    snprintf(b, 6, "%.2f", static_cast<float>(t1) / 100);
    lv_label_set_text(slider_down_label, b);
    lv_obj_set_auto_realign(slider_down_label, true);
    lv_obj_align(slider_down_label, slider, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

    label = lv_label_create(lv_scr_act(), NULL);
    lv_label_set_text(label, label_text);
    lv_obj_align(label, NULL, alignment, x, y + 2);
    lv_obj_set_style_local_text_font(label, 0, LV_STATE_DEFAULT, &lv_font_montserrat_20);
}

void slider_label_c::cb(lv_event_t e)
{
    if (e == LV_EVENT_VALUE_CHANGED)
    {
        static char buf[10]; /* max 3 bytes for number plus 1 null terminating byte */
        snprintf(buf, 6, "%.2f", static_cast<float>(lv_slider_get_value(slider)) / 100);
        lv_label_set_text(slider_up_label, buf);
        snprintf(buf, 6, "%.2f", static_cast<float>(lv_slider_get_left_value(slider)) / 100);
        lv_label_set_text(slider_down_label, buf);
    }
}

void slider_label_c::set_label(const char *l, lv_color_t c)
{
    //lv_obj_set_style_local_text_color(label, 0, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
    lv_label_set_text(label, l);
}