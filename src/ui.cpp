#include <time.h>
#include "ui.h"
#include "circuits.h"

// globals
analogMeter *temp_meter;
analogMeter *hum_meter;

uiElements::uiElements(int idle_time) : saver(this, idle_time)
{
    extern const lv_img_dsc_t splash_screen;
    extern const lv_img_dsc_t biohazard;

    modes[UI_SPLASH] = lv_obj_create(NULL, NULL);
    /* splash */
    set_mode(UI_SPLASH);
    lv_obj_t *icon = lv_img_create(modes[UI_SPLASH], NULL);
    lv_img_set_src(icon, &splash_screen);
    lv_obj_align(icon, modes[UI_SPLASH], LV_ALIGN_CENTER, 0, 0);

    lv_task_handler(); // let the splash screen appear

    modes[UI_OPERATIONAL] = lv_obj_create(NULL, NULL);
    modes[UI_SCREENSAVER] = lv_obj_create(NULL, NULL);
    modes[UI_ALARM] = lv_obj_create(NULL, NULL);

    lv_obj_t *alarmpic = lv_img_create(modes[UI_ALARM], NULL);
    lv_img_set_src(alarmpic, &biohazard);
    lv_obj_align(alarmpic, modes[UI_ALARM], LV_ALIGN_CENTER, 0, 0);

    tab_view = lv_tabview_create(modes[UI_OPERATIONAL], NULL);
    tab_status = lv_tabview_add_tab(tab_view, "Status");
    tab_controls = lv_tabview_add_tab(tab_view, "Controls");
    tab_settings = lv_tabview_add_tab(tab_view, "Config");

    /* tab 1 - Status */
    temp_meter = new analogMeter(tab_status, "Temperatur", ctrl_temprange, "C");
    add_status(temp_meter->get_area());

    hum_meter = new analogMeter(tab_status, "Feuchtigkeit", ctrl_humrange, "\%");
    add_status(hum_meter->get_area());

    time_widget = lv_label_create(tab_controls, NULL);
    lv_label_set_text(time_widget, "Time: ");
    add_control(time_widget);
    load_widget = lv_label_create(tab_controls, NULL);
    lv_label_set_text(load_widget, "Load: ");
    add_control(load_widget);

    /* update screensaver, status widgets periodically per 1s */
    lv_task_create(update_task, 1000, LV_TASK_PRIO_LOWEST, this);
}

void uiElements::add_status(lv_obj_t *e)
{
    lv_obj_align(e, tab_status, LV_ALIGN_IN_TOP_MID, stat_x, stat_y);
    stat_y += lv_obj_get_height(e);
}

void uiElements::add_control(lv_obj_t *e)
{
    lv_obj_align(e, tab_controls, LV_ALIGN_IN_TOP_LEFT, ctrl_x, ctrl_y);
    ctrl_y += lv_obj_get_height(e);
}

void uiElements::add_setting(lv_obj_t *e)
{
    lv_obj_align(e, tab_settings, LV_ALIGN_IN_TOP_MID, setgs_x, setgs_y);
    setgs_y += lv_obj_get_height(e);
}

void uiElements::update_task(lv_task_t *ta)
{
    uiElements *ui = static_cast<uiElements *>(ta->user_data);
    ui->update();
}

void uiElements::update()
{
    struct tm t;
    static char buf[64];
    saver.update();
    // update time widget
    time_obj->get_time(&t);
    strftime(buf, 64, "Time: %a, %b %d %Y %H:%M:%S", &t);
    lv_label_set_text(time_widget, buf);
    // update load_widget
    snprintf(buf, 64, "Load: %d%%", 100 - lv_task_get_idle());
    lv_label_set_text(load_widget, buf);
};

#ifdef ALARM_SOUND
void uiElements::biohazard_alarm(void)
{
    static int mode = 0; /* 0 slow, 1 fast */
    static int pitch = 400, delta;
    static const int pitch_max = 4000, pitch_min = pitch_max / 10, d1 = 20;
    static int count = 0, dir = 1;
    static bool initialized = false;

    if (act_mode != UI_ALARM)
    {
        if (initialized)
        {
            ledcWriteTone(buzzer_channel, 0);
            initialized = false;
            ledcDetachPin(BUZZER_PIN);
        }
        return;
    }
    /* setup Buzzer */
    if (!initialized)
    {
        ledcSetup(buzzer_channel, 0, 8);
        ledcAttachPin(BUZZER_PIN, buzzer_channel);
        ledcWrite(buzzer_channel, 125); /* duty cycle ~50% */
        initialized = true;
    }

    if (count < 6)
    {
        delta = d1 + d1 * (2 * mode);
    }
    else
    {
        count = 0;
        mode = mode ? 0 : 1;
    }
    pitch = pitch + dir * delta;

    if (pitch > pitch_max)
    {
        dir *= -1;
        pitch = pitch_max;
        count++;
    }
    if (pitch < pitch_min)
    {
        dir *= -1;
        pitch = pitch_min;
        count++;
    }
    ledcWriteTone(buzzer_channel, pitch);
}
#endif

static tiny_hash_c<lv_obj_t *, button_label_c *> button_callbacks(10);
static void button_cb_wrapper(lv_obj_t *obj, lv_event_t e)
{
    button_callbacks.retrieve(obj)->cb(e);
}

tiny_hash_c<lv_obj_t *, rangeSpinbox<myRange<struct tm>> *> spinbox_callbacks{6};
void spinbox_cb_wrapper(lv_obj_t *obj, lv_event_t e)
{
    spinbox_callbacks.retrieve(obj)->cb(obj, e);
}

button_label_c::button_label_c(lv_obj_t *parent, genCircuit *c, const char *l, int w, int h, lv_align_t alignment)
    : label_text(l)
{
    area = lv_obj_create(parent, NULL);
    lv_obj_set_size(area, w, h);
    circuit = c;
    obj = lv_switch_create(area, NULL);
    lv_obj_align(obj, area, LV_ALIGN_IN_TOP_RIGHT, -10, h / 4);
    lv_obj_set_event_cb(obj, button_cb_wrapper);
    button_callbacks.store(obj, this);

    label = lv_label_create(area, NULL);
    lv_label_set_text(label, label_text);
    lv_obj_align(label, area, LV_ALIGN_IN_TOP_LEFT, 10, h / 4);
    lv_obj_set_style_local_text_font(label, 0, LV_STATE_DEFAULT, &lv_font_montserrat_20);
}

void button_label_c::cb(lv_event_t e)
{
    if (e == LV_EVENT_VALUE_CHANGED)
    {
        printf("%s: %s\n", label_text, lv_switch_get_state(obj) ? "On" : "Off");
        circuit->io_set((lv_switch_get_state(obj) ? HIGH : LOW), true); /* force HIGH/LOW without invers */
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

slider_label_c::slider_label_c(lv_obj_t *parent, genCircuit *c, const char *l, myRange<float> &ra, myRange<float> &dr, int w, int h, lv_align_t alignment)
    : label_text(l), ctrl_range(ra)
{
    circuit = c;
    area = lv_obj_create(parent, NULL);
    lv_obj_set_size(area, w, h);

    /* slider label */
    label = lv_label_create(area, NULL);
    lv_label_set_text(label, label_text);
    lv_obj_align(label, area, LV_ALIGN_IN_TOP_LEFT, 5, 5);
    //   lv_obj_set_style_local_text_font(label, 0, LV_STATE_DEFAULT, &lv_font_montserrat_20);

    /* slider */
    slider = lv_slider_create(area, NULL);
    lv_slider_set_type(slider, LV_SLIDER_TYPE_RANGE);
    lv_obj_set_width(slider, w - 20);
    lv_obj_align(slider, area, LV_ALIGN_IN_TOP_RIGHT, -10, h / 2);
    lv_obj_set_event_cb(slider, slider_cb_wrapper);
    slider_callbacks.store(slider, this);
    lv_slider_set_range(slider, dr.get_lbound() * 100, dr.get_ubound() * 100);

    myRange<float> r = circuit->get_range();
    lv_slider_set_left_value(slider, r.get_lbound() * 100, LV_ANIM_ON);
    lv_slider_set_value(slider, r.get_ubound() * 100, LV_ANIM_ON);

    static char b[10];
    /* Create a label for ranges below the slider */
    slider_up_label = lv_label_create(area, NULL);
    snprintf(b, 6, "%.2f", r.get_ubound());
    lv_label_set_text(slider_up_label, b);
    lv_obj_set_auto_realign(slider_up_label, true);
    lv_obj_align(slider_up_label, slider, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 5);

    slider_down_label = lv_label_create(area, NULL);
    snprintf(b, 6, "%.2f", r.get_lbound());
    lv_label_set_text(slider_down_label, b);
    lv_obj_set_auto_realign(slider_down_label, true);
    lv_obj_align(slider_down_label, slider, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
}

void slider_label_c::cb(lv_event_t e)
{
    if (e == LV_EVENT_VALUE_CHANGED)
    {
        myRange<float> &r = circuit->get_range();
        static char buf[10];
        snprintf(buf, 6, "%.2f", static_cast<float>(lv_slider_get_value(slider)) / 100);
        lv_label_set_text(slider_up_label, buf);
        r.set_ubound(static_cast<float>(lv_slider_get_value(slider)) / 100);
        snprintf(buf, 6, "%.2f", static_cast<float>(lv_slider_get_left_value(slider)) / 100);
        lv_label_set_text(slider_down_label, buf);
        r.set_lbound(static_cast<float>(lv_slider_get_left_value(slider)) / 100);
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

void uiScreensaver::update()
{
    uint32_t act = lv_disp_get_inactive_time(lv_disp_get_default());
    if (act < idle_time)
    {
        if (ui->get_mode() != UI_OPERATIONAL)
        {
            digitalWrite(TFT_LED, LOW);
            ui->set_mode(UI_OPERATIONAL);
#ifdef ALARM_SOUND
            ui->biohazard_alarm();
#endif
            glob_delay = 5;
        }
        return;
    }
    if (act < (idle_time + 10 * 1000)) /* show splash screen for 10 secs */
    {
        if (ui->get_mode() != UI_SPLASH)
            ui->set_mode(UI_SPLASH);
        return;
    }

    if ((temp_meter->get_val() < ctrl_temprange.get_lbound()) ||
        (temp_meter->get_val() > ctrl_temprange.get_ubound()) ||
        (hum_meter->get_val() < ctrl_humrange.get_lbound()) ||
        (hum_meter->get_val() > ctrl_humrange.get_ubound()))
    {
        int t = digitalRead(TFT_LED);
        digitalWrite(TFT_LED, (t == HIGH) ? LOW : HIGH);
        ui->set_mode(UI_ALARM);
        glob_delay = 5;

        return;
    }
    if (ui->get_mode() != UI_SCREENSAVER)
    {
        ui->set_mode(UI_SCREENSAVER);
        digitalWrite(TFT_LED, HIGH);
        glob_delay = 250;
    }
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

template <>
float myRange<struct tm>::to_float(const struct tm &v)
{
    float res = v.tm_hour + (100.0 / 60.0) * v.tm_min;
    //printf("%d:%d = %.02f\n", v.tm_hour, v.tm_min, res);
    return res;
}
template <>
void myRange<struct tm>::set_lbound(const int v)
{
    lbound.tm_hour = v / 100;
    lbound.tm_min = (v % 100) * 0.6;
    //printf("setting lbound %d:%d\n", lbound.tm_hour, lbound.tm_min);
}

template <>
void myRange<struct tm>::set_ubound(const int v)
{
    ubound.tm_hour = v / 100;
    ubound.tm_min = (v % 100) * 0.6;
    //printf("setting lbound %d:%d\n", ubound.tm_hour, ubound.tm_min);
}