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

#include <time.h>
#include <WiFi.h>
#include "ui.h"
#include "circuits.h"

uiElements::uiElements(int idle_time) : saver(this, idle_time), mwidget(nullptr)
{
    extern const lv_img_dsc_t splash_screen;
    extern const lv_img_dsc_t biohazard;
    extern const lv_img_dsc_t wifi_warning;

    mutex = xSemaphoreCreateMutex();
    V(mutex);

    ui_master_lock = xSemaphoreCreateMutex();
    V(ui_master_lock);

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
    modes[UI_WARNING] = lv_obj_create(NULL, NULL);

    lv_obj_t *alarmpic = lv_img_create(modes[UI_ALARM], NULL);
    lv_img_set_src(alarmpic, &biohazard);
    lv_obj_align(alarmpic, modes[UI_ALARM], LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *wifiwarningpic = lv_img_create(modes[UI_WARNING], NULL);
    lv_img_set_src(wifiwarningpic, &wifi_warning);
    lv_obj_align(wifiwarningpic, modes[UI_WARNING], LV_ALIGN_CENTER, 0, 0);

    tab_view = lv_tabview_create(modes[UI_OPERATIONAL], NULL);
    tabs[UI_STATUS] = lv_tabview_add_tab(tab_view, "Status");
    tabs[UI_CTRLS] = lv_tabview_add_tab(tab_view, "Ctrls");
    tabs[UI_CFG1] = lv_tabview_add_tab(tab_view, "Cfg1");
    tabs[UI_CFG2] = lv_tabview_add_tab(tab_view, "Cfg2");
    tabs[UI_SETTINGS] = lv_tabview_add_tab(tab_view, "Set");

    /* tab 1 - Status */
    avg_temp_berg = new analogMeter(this, UI_STATUS, "Temperatur Berg", ctrl_temprange, "C");
    add2ui(UI_STATUS, avg_temp_berg->get_area());
    avg_temp_berg->set_val(27.0);

    avg_temp_erde = new analogMeter(this, UI_STATUS, "Temperatur Erde", ctrl_temprange, "C");
    add2ui(UI_STATUS, avg_temp_erde->get_area());
    avg_temp_erde->set_val(27.0);

    avg_hum_berg = new analogMeter(this, UI_STATUS, "Feuchtigkeit Berg", ctrl_humrange, "\%");
    add2ui(UI_STATUS, avg_hum_berg->get_area());
    avg_hum_berg->set_val(65.0);

    avg_hum_erde = new analogMeter(this, UI_STATUS, "Feuchtigkeit Erde", ctrl_humrange, "\%");
    add2ui(UI_STATUS, avg_hum_erde->get_area());
    avg_hum_erde->set_val(65.0);

    time_widget = lv_label_create(tabs[UI_CFG2], NULL);
    lv_label_set_text(time_widget, "Time: ");
    add2ui(UI_CFG2, time_widget);
    fcce_widget = lv_label_create(tabs[UI_CFG2], NULL);
    lv_label_set_recolor(fcce_widget, true);
    lv_label_set_text(fcce_widget, "FCCE: ");
    add2ui(UI_CFG2, fcce_widget);
    fcce_widget_uptime = lv_label_create(tabs[UI_CFG2], NULL);
    lv_label_set_text(fcce_widget_uptime, "FCCE: ");
    add2ui(UI_CFG2, fcce_widget_uptime);
    load_widget = lv_label_create(tabs[UI_CFG2], NULL);
    lv_label_set_text(load_widget, "Load: ");
    add2ui(UI_CFG2, load_widget);
    update_url = lv_label_create(tabs[UI_CFG2], NULL);
    lv_label_set_text(update_url, String("http://" + WiFi.localIP().toString() + ":/_ac").c_str());
    add2ui(UI_CFG2, update_url);
    event_log = lv_textarea_create(tabs[UI_CFG2], NULL);
    lv_obj_set_size(event_log, 330, 72);
    lv_textarea_set_scroll_propagation(event_log, true);
    lv_textarea_set_text(event_log, "Event Log\n");
    lv_textarea_set_cursor_hidden(event_log, true);
    lv_textarea_set_sscrollbar_mode(event_log, LV_SCRLBAR_MODE_AUTO);
    lv_obj_set_style_local_bg_color(event_log, LV_TEXTAREA_PART_BG, LV_STATE_DEFAULT, LV_COLOR_GRAY);
    add2ui(UI_CFG2, event_log);

    add2ui(UI_CFG1, (new rangeSpinbox<myRange<struct tm>>(this, UI_CFG1, "Tag", def_day, 230, 72))->get_area());

    /* settings */
    add2ui(UI_SETTINGS, (new settingsButton(this, UI_SETTINGS, "Alarm Sound", do_sound, 230, 48))->get_area());
    add2ui(UI_SETTINGS, (new settingsButton(this, UI_SETTINGS, "Manuell", do_manual, 230, 48))->get_area());

    /* some action buttons */
    add2ui(UI_SETTINGS, (new actionButton(this, UI_SETTINGS, "Reset FCCE", [](uiCommons *p) {
                            p->get_ui()->log_event("reset fcce requested by user...");
                            mqtt_publish("/reset-request", "user request");
                        }))->get_area(),
           0, 5);

    add2ui(UI_SETTINGS, (new actionButton(this, UI_SETTINGS, "Reset FCC", [](uiCommons *p) {
                            p->get_ui()->log_event("reset fcc requested by user... rebooting.");
                            ESP.restart();
                        }))->get_area(),
           0, 5);
    add2ui(UI_SETTINGS, (new actionButton(this, UI_SETTINGS, "Clear Eventlog", [](uiCommons *p) {
                            log_msg("clear of eventlog requested.");
                            p->get_ui()->reset_eventlog();
                        }))->get_area(),
           0, 5);
    /* update screensaver, status widgets periodically per 1s */
    lv_task_create(update_task, 1000, LV_TASK_PRIO_LOWEST, this);

    /* lvgl independent stuff - NOT USED NOW FIXME */
    TaskHandle_t handle;
    xTaskCreate(ui_task_wrapper, "ui-task helper", 4000, this, configMAX_PRIORITIES - 1, &handle);
}

void uiElements::ui_task_wrapper(void *obj)
{
    uiElements *o = static_cast<uiElements *>(obj);
    o->ui_task();
}

void uiElements::add2ui(ui_tabs_t t, lv_obj_t *e, int dx, int dy)
{
    if (lastwidgets[t])
    {
        lv_obj_align(e, lastwidgets[t], LV_ALIGN_OUT_BOTTOM_LEFT, dx, dy);
    }
    else
    {
        lv_obj_align(e, get_tab(t), LV_ALIGN_IN_TOP_LEFT, dx, dy);
    }
    lastwidgets[t] = e;
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
    static bool ip_initialized = false;
    saver.update();

    // update update URL widget only once.
    if (!ip_initialized)
    {
        lv_label_set_text(update_url, String("http://" + WiFi.localIP().toString() + "/_ac").c_str());
        ip_initialized = true;
    }

    // update overview stats
    avg_temp_berg->set_val(sens_temp_berg->get_data());
    avg_temp_erde->set_val(sens_temp_erde->get_data());
    avg_hum_berg->set_val(sens_hum_berg->get_data());
    avg_hum_erde->set_val(sens_hum_erde->get_data());

    // fcce alive
    time_t now;
    time(&now);
    long diff = now - last_fcce_tick;
    if (diff > 91)
        snprintf(buf, 64, "#ff0000 FCCE last seen %lds ago", diff);
    else
        snprintf(buf, 64, "FCCE last seen %lds ago", diff);
    lv_label_set_text(fcce_widget, buf);
    // update time widget
    time_obj->get_time(&t);
    strftime(buf, 64, "Time: %a, %b %d %Y %H:%M:%S", &t);
    lv_label_set_text(time_widget, buf);
    // update load_widget
    //snprintf(buf, 64, "Load: %d%%", 100 - lv_task_get_idle());
    //lv_label_set_text(load_widget, buf);

    /* take care of a life-signal to fcce */
    static unsigned long fcc_wd = millis();
    if ((millis() - fcc_wd) > (90 * 1000))
    {
        fcc_wd = millis();
        unsigned long upt = fcc_wd / 1000;
        snprintf(buf, 64, "fcc/uptime %02ldh:%02ldm:%02lds, mem = %d",
                 upt / 3600,
                 (upt % 3600) / 60,
                 (upt % 60),
                 ESP.getFreeHeap());
        lv_label_set_text(load_widget, buf);

        mqtt_publish("/cc-alive", buf);
    }
};

void uiElements::ui_task(void)
{
    printf("ui-task launched...\n");
    delay(500);
    while (1)
    {
        int res = biohazard_alarm();
        delay(res);
    }
}

int uiElements::biohazard_alarm(void)
{
    static int mode = 0; /* 0 slow, 1 fast */
    static int pitch = 400, delta, bdelta;
    static const int pitch_max = 4000, pitch_min = pitch_max / 10, d1 = 20;
    static const int multiplier = 2;
    static int count = 0, dir = 1, bdir = 1;
    static bool initialized = false;
    static const int bd1 = 10;
    static int brightness = 0;

    if (get_mode() != UI_ALARM)
    {
        if (initialized)
        {
            ledcWriteTone(buzzer_channel, 0);
            initialized = false;
            ledcDetachPin(BUZZER_PIN);
            ledcDetachPin(TFT_LED);
            //digitalWrite(TFT_LED, LOW); /* turn on display */
        }
        return 500;
    }
    /* setup Buzzer */
    if (!initialized)
    {
        /* setup & attach PWM for buzzer sound */
        ledcSetup(buzzer_channel, 0, 8);
        ledcWrite(buzzer_channel, 125); /* duty cycle ~50% */
        if (play_sound())
        {
            ledcAttachPin(BUZZER_PIN, buzzer_channel);
        }
        /* setup & attach PWM for soft blinking backlight */
        ledcSetup(bgled_channel, 4000, 10); /* hardware PWM channel 1 */
        ledcWrite(bgled_channel, 255);      /* full brightness */
        ledcAttachPin(TFT_LED, bgled_channel);
        pitch = 400;
        mode = 0;
        brightness = 0;
        bdir = dir = 1;
        initialized = true;
    }

    if (count < 6)
    {
        delta = dir * (d1 + d1 * (multiplier * mode));
    }
    else
    {
        count = 0;
        mode = mode ? 0 : 1;
    }
    pitch = pitch + delta;

    if (pitch > pitch_max || pitch < pitch_min)
    {
        pitch -= delta;
        dir *= -1;
        count++;
    }
    //    if (play_sound())
    ledcWriteTone(buzzer_channel, pitch);

    bdelta = bdir * (bd1 + bd1 * (multiplier * mode));
    brightness = brightness + bdelta;
    if (brightness < 0 || brightness > 1024)
    {
        brightness -= bdelta;
        bdir *= -1;
    }
    ledcWrite(bgled_channel, brightness);

    return 5;
}

bool uiElements::check_manual(void)
{
    if (!manual())
    {
        if (mwidget)
            lv_obj_del(mwidget);
        mwidget = nullptr;
        return false;
    }

    if (!mwidget)
    {
        mwidget = lv_label_create(get_tab(UI_STATUS), NULL);
        lv_label_set_recolor(mwidget, true);
        lv_label_set_text(mwidget, "#ff0000 MANUELL MODUS");
        lv_obj_set_style_local_text_font(mwidget, 0, LV_STATE_DEFAULT, &lv_font_montserrat_20);
        lv_obj_align(mwidget, get_tab(UI_STATUS), LV_ALIGN_CENTER, 0, 0);
    }
    return true;
}

static tiny_hash_c<genSensor *, lv_obj_t *> sensor_widgets(10);
void uiElements::register_sensor(genSensor *s)
{
    lv_obj_t *sensor_label = lv_label_create(tabs[UI_CFG2], NULL);
    lv_label_set_recolor(sensor_label, true);
    lv_label_set_text(sensor_label, "#ff0000 <not-yet-initialized>");
    add2ui(UI_CFG2, sensor_label);
    sensor_widgets.store(s, sensor_label);
}

void uiElements::update_sensor(genSensor *s)
{
    lv_obj_t *w = sensor_widgets.retrieve(s);
    lv_label_set_text(w, s->to_string().c_str());
}

void uiElements::update_config(String s)
{
    if (s.startsWith("/sensor-alive"))
    {
        time(&last_fcce_tick);
        return;
    }
    if (s.startsWith("/uptime"))
    {
        time(&last_fcce_tick);
        log_msg(String("fcce: ") + s);
        lv_label_set_text(fcce_widget_uptime, s.c_str());
        return;
    }
    log_msg(String("Update arrived: ") + s);
}

void uiElements::set_switch(String s)
{
    log_msg("Setting switch via mqtt: " + s);
}

void uiElements::log_event(const char *s)
{
    static int count = 0;
    uint32_t pos;
    char buf[12];
    snprintf(buf, 12, "\n%03d:", count++);
    lv_textarea_add_text(event_log, buf);
    lv_textarea_add_text(event_log, s);
    log_msg(buf);
    pos = lv_textarea_get_cursor_pos(event_log);
    if (pos > 500)
    {
        /* delete older messages */
        lv_textarea_set_cursor_pos(event_log, pos - 500);
        for (int i = pos - 500; i > 0; i--)
            lv_textarea_del_char(event_log);

        lv_textarea_set_cursor_pos(event_log, LV_TEXTAREA_CURSOR_LAST);
    }
}

void uiElements::reset_eventlog(void)
{
    while (lv_textarea_get_cursor_pos(event_log))
    {
        lv_textarea_del_char(event_log);
    }
}

bool uiElements::is_critical(void)
{
    if ((avg_temp_berg->get_val() < ctrl_temprange.get_lbound()) ||
        (avg_temp_erde->get_val() < ctrl_temprange.get_lbound()) ||
        (avg_hum_berg->get_val() < ctrl_humrange.get_lbound()) ||
        (avg_hum_erde->get_val() < ctrl_humrange.get_lbound()) ||
        (avg_temp_berg->get_val() > ctrl_temprange.get_ubound()) ||
        (avg_temp_erde->get_val() > ctrl_temprange.get_ubound()) ||
        (avg_hum_berg->get_val() > ctrl_humrange.get_ubound()) ||
        (avg_hum_erde->get_val() > ctrl_humrange.get_ubound()))
        return true;
    return false;
}

/* button with label widget */
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

button_label_c::button_label_c(uiElements *ui, ui_tabs_t t, genCircuit *c, int w, int h)
    : uiCommons(ui), circuit(c)
{
    area = lv_obj_create(ui->get_tab(t), NULL);
    lv_obj_set_size(area, w, h);
    obj = lv_switch_create(area, NULL);
    lv_obj_align(obj, area, LV_ALIGN_IN_TOP_RIGHT, -10, h / 4);
    lv_obj_set_event_cb(obj, button_cb_wrapper);
    button_callbacks.store(obj, this);

    label = lv_label_create(area, NULL);
    lv_label_set_text(label, c->get_name().c_str());
    lv_obj_align(label, area, LV_ALIGN_IN_TOP_LEFT, 10, h / 4);
    lv_obj_set_style_local_text_font(label, 0, LV_STATE_DEFAULT, &lv_font_montserrat_20);
}

void button_label_c::cb(lv_event_t e)
{
    if (e == LV_EVENT_VALUE_CHANGED)
    {
        printf("%s: %s\n", circuit->get_name().c_str(), lv_switch_get_state(obj) ? "On" : "Off");
        circuit->io_set((lv_switch_get_state(obj) ? HIGH : LOW), true); /* force HIGH/LOW without invers */
    }
}

void button_label_c::set(uint8_t v)
{
    if (v) /* any value of servo > 0 is 'on' - FIXME */
        lv_switch_on(obj, LV_ANIM_ON);
    else
        lv_switch_off(obj, LV_ANIM_ON);
}

static tiny_hash_c<lv_obj_t *, slider_label_c *> slider_callbacks(10);
static void slider_cb_wrapper(lv_obj_t *obj, lv_event_t e)
{
    slider_callbacks.retrieve(obj)->cb(e);
}

slider_label_c::slider_label_c(uiElements *ui, ui_tabs_t t, genCircuit *c, myRange<float> &ra, myRange<float> &cr, int w, int h, bool d)
    : uiCommons(ui), range(ra), is_day(d)
{
    static char b[64];
    area = lv_obj_create(ui->get_tab(t), NULL);
    lv_obj_set_size(area, w, h);
    lv_obj_set_style_local_bg_color(area, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_MAKE(0xc0, 0xc0, 0xc0));

    /* slider */
    slider = lv_slider_create(area, NULL);
    lv_slider_set_type(slider, LV_SLIDER_TYPE_RANGE);
    lv_obj_set_width(slider, w - 20);
    lv_obj_align(slider, area, LV_ALIGN_IN_TOP_RIGHT, -10, h / 2);
    lv_obj_set_event_cb(slider, slider_cb_wrapper);
    slider_callbacks.store(slider, this);
    lv_slider_set_range(slider, cr.get_lbound() * 100, cr.get_ubound() * 100);

    lv_slider_set_left_value(slider, range.get_lbound() * 100, LV_ANIM_ON);
    lv_slider_set_value(slider, range.get_ubound() * 100, LV_ANIM_ON);

    /* slider label */
    snprintf(b, 64, "%s", (is_day ? c->get_name().c_str() : ""));
    label = lv_label_create(area, NULL);
    lv_label_set_recolor(label, true);
    lv_label_set_text(label, b);
    lv_obj_align(label, slider, LV_ALIGN_OUT_TOP_LEFT, 0, -5);
    //   lv_obj_set_style_local_text_font(label, 0, LV_STATE_DEFAULT, &lv_font_montserrat_20);

    /* Create a label for ranges above the slider */
    slider_up_label = lv_label_create(area, NULL);
    lv_label_set_recolor(slider_up_label, true);
    snprintf(b, 64, "%s%.2f", (is_day ? "" : "#0000ff "), range.get_ubound());
    lv_label_set_text(slider_up_label, b);
    lv_obj_set_auto_realign(slider_up_label, true);
    lv_obj_align(slider_up_label, slider, LV_ALIGN_OUT_TOP_RIGHT, 0, -5);

    slider_down_label = lv_label_create(area, NULL);
    lv_label_set_recolor(slider_down_label, true);
    snprintf(b, 64, "%s%.2f-", (is_day ? "" : "#0000ff "), range.get_lbound());
    lv_label_set_text(slider_down_label, b);
    lv_obj_set_auto_realign(slider_down_label, true);
    lv_obj_align(slider_down_label, slider_up_label, LV_ALIGN_OUT_LEFT_MID, 0, 0);
}

void slider_label_c::cb(lv_event_t e)
{
    if (e == LV_EVENT_VALUE_CHANGED)
    {
        static char buf[64];
        const char *col = (is_day ? "" : "#0000ff ");
        snprintf(buf, 64, "%s%.2f", col, static_cast<float>(lv_slider_get_value(slider)) / 100);
        lv_label_set_text(slider_up_label, buf);
        range.set_ubound(static_cast<float>(lv_slider_get_value(slider)) / 100);
        snprintf(buf, 64, "%s%.2f-", col, static_cast<float>(lv_slider_get_left_value(slider)) / 100);
        lv_label_set_text(slider_down_label, buf);
        range.set_lbound(static_cast<float>(lv_slider_get_left_value(slider)) / 100);
    }
}

void slider_label_c::set_label(const char *l, lv_color_t c)
{
    //lv_obj_set_style_local_text_color(label, 0, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
    lv_label_set_text(label, l);
}

static tiny_hash_c<lv_obj_t *, settingsButton *> bsettings_callbacks(10);
static void bsettings_cb_wrapper(lv_obj_t *obj, lv_event_t e)
{
    bsettings_callbacks.retrieve(obj)->cb(e);
}

settingsButton::settingsButton(uiElements *ui, ui_tabs_t t, const char *l, bool &v, int w, int h)
    : uiCommons(ui), label_text(l), state(v)
{
    area = lv_obj_create(ui->get_tab(t), NULL);
    lv_obj_set_size(area, w, h);

    obj = lv_switch_create(area, NULL);
    lv_obj_align(obj, area, LV_ALIGN_IN_TOP_RIGHT, -10, h / 4);
    lv_obj_set_event_cb(obj, bsettings_cb_wrapper);
    bsettings_callbacks.store(obj, this);

    label = lv_label_create(area, NULL);
    lv_label_set_text(label, label_text);
    lv_obj_align(label, area, LV_ALIGN_IN_TOP_LEFT, 10, h / 4);
    lv_obj_set_style_local_text_font(label, 0, LV_STATE_DEFAULT, &lv_font_montserrat_20);
}

void settingsButton::cb(lv_event_t e)
{
    if (e == LV_EVENT_VALUE_CHANGED)
    {
        state = lv_switch_get_state(obj);
    }
}

static tiny_hash_c<lv_obj_t *, actionButton *> actionButton_callbacks(10);
static void actionButton_cb_wrapper(lv_obj_t *obj, lv_event_t e)
{
    actionButton_callbacks.retrieve(obj)->cb(e);
}

actionButton::actionButton(uiElements *ui, ui_tabs_t tab, String label, actionButton_cb f)
    : uiCommons(ui)
{
    /* cool gum animation - from lvgl docs */
    static lv_anim_path_t path_overshoot;
    lv_anim_path_init(&path_overshoot);
    lv_anim_path_set_cb(&path_overshoot, lv_anim_path_overshoot);

    static lv_anim_path_t path_ease_out;
    lv_anim_path_init(&path_ease_out);
    lv_anim_path_set_cb(&path_ease_out, lv_anim_path_ease_out);

    static lv_anim_path_t path_ease_in_out;
    lv_anim_path_init(&path_ease_in_out);
    lv_anim_path_set_cb(&path_ease_in_out, lv_anim_path_ease_in_out);

    /*Gum-like button*/
    static lv_style_t style_gum;
    lv_style_init(&style_gum);
    lv_style_set_transform_width(&style_gum, LV_STATE_PRESSED, 10);
    lv_style_set_transform_height(&style_gum, LV_STATE_PRESSED, -10);
    lv_style_set_value_letter_space(&style_gum, LV_STATE_PRESSED, 5);
    lv_style_set_transition_path(&style_gum, LV_STATE_DEFAULT, &path_overshoot);
    lv_style_set_transition_path(&style_gum, LV_STATE_PRESSED, &path_ease_in_out);
    lv_style_set_transition_time(&style_gum, LV_STATE_DEFAULT, 250);
    lv_style_set_transition_delay(&style_gum, LV_STATE_DEFAULT, 100);
    lv_style_set_transition_prop_1(&style_gum, LV_STATE_DEFAULT, LV_STYLE_TRANSFORM_WIDTH);
    lv_style_set_transition_prop_2(&style_gum, LV_STATE_DEFAULT, LV_STYLE_TRANSFORM_HEIGHT);
    lv_style_set_transition_prop_3(&style_gum, LV_STATE_DEFAULT, LV_STYLE_VALUE_LETTER_SPACE);

    lv_obj_t *btn = area = lv_btn_create(ui->get_tab(tab), NULL);
    lv_obj_set_event_cb(btn, actionButton_cb_wrapper);
    actionButton_callbacks.store(btn, this);
    lv_obj_add_style(btn, LV_BTN_PART_MAIN, &style_gum);

    lv_obj_t *l = lv_label_create(btn, NULL);
    lv_label_set_text(l, label.c_str());

    cb_func = f;
}

void actionButton::cb(lv_event_t e)
{
    if (e == LV_EVENT_CLICKED)
    {
        cb_func(this);
    }
}

/* analogMeter */
analogMeter::analogMeter(uiElements *ui, ui_tabs_t t, const char *n, myRange<float> r, const char *u) : uiCommons(ui), name(n), val(0.0), unit(u)
{
    lv_obj_t *tmp;
    area = lv_obj_create(ui->get_tab(t), NULL);
    lv_obj_set_size(area, 200, 126);

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

    lv_obj_t *te = lv_label_create(lmeter, NULL);
    lv_label_set_text(te, name);
    lv_obj_align(te, lmeter, LV_ALIGN_IN_BOTTOM_MID, 0, -10);

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

    if (ui->is_critical())
    {
        //int t = digitalRead(TFT_LED);
        //digitalWrite(TFT_LED, (t == HIGH) ? LOW : HIGH);
        ui->set_mode(UI_ALARM);
        glob_delay = 5;
        return;
    }
    if (ui->get_mode() == UI_WARNING)
    {
        digitalWrite(TFT_LED, LOW);
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