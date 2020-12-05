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

#ifndef __ui_h__
#define __ui_h__
#include <Arduino.h>
#include <lvgl.h>
#include <stdio.h>
#include <stdlib.h>
#include <WString.h>

#define MUT_EXCL
#ifdef MUT_EXCL
#define P(sem) xSemaphoreTake((sem), portMAX_DELAY)
#define V(sem) xSemaphoreGive(sem)
#else
#define P(sem)
#define V(seM)
#endif

#define TFT_LED 15

//#define ALARM_SOUND
#define BUZZER_PIN 21

template <typename T>
class myRange;
class uiElements;
class genCircuit;
class uiScreensaver;

/* prototypes */
void init_lvgl(void);
uiElements *setup_ui(const int to);
void log_msg(const char *s);
void log_msg(const String s);
extern lv_obj_t *log_handle;
extern myRange<float> ctrl_temprange;
extern myRange<float> ctrl_humrange;
extern myRange<struct tm> def_day;

extern int glob_delay;

void setup_wifi(void);
void loop_wifi(void);
//#include <WebServer.h>
//void setup_OTA(WebServer *s);
//void setup_OTA(void);
//void loop_OTA(void);

template <typename T>
class myRange; // forward declaration

template <typename T, typename I>
class tiny_hash_c; // forward declaration

template <typename T>
class rangeSpinbox;

typedef enum
{
    UI_SPLASH = 0,
    UI_OPERATIONAL,
    UI_SCREENSAVER,
    UI_ALARM
} ui_modes_t;

typedef enum
{
    UI_STATUS = 0,
    UI_CTRLS,
    UI_CFG1,
    UI_CFG2,
    UI_SETTINGS
} ui_tabs_t;

class uiScreensaver
{
    uiElements *ui;
    int idle_time; /* in ms */
    ui_modes_t mode;

public:
    uiScreensaver(uiElements *u, int s = 2 * 60, ui_modes_t m = UI_OPERATIONAL) : ui(u), idle_time(s * 1000), mode(m) {}
    ~uiScreensaver() = default;

    void update();
};

class uiElements
{
    lv_obj_t *tab_view;
    lv_obj_t *tabs[5];                                                        /* 5 UI tabs */
    lv_obj_t *lastwidgets[5] = {nullptr, nullptr, nullptr, nullptr, nullptr}; /* remember last widget placed in tab to align next one */
    lv_obj_t *modes[4];                                                       /* 4 operation modes: SPLASH (startup), OPERATIONAL, SCREENSAVER, ALARM */
    ui_modes_t act_mode;
    uiScreensaver saver;
    lv_obj_t *mwidget;
    lv_obj_t *time_widget;
    lv_obj_t *load_widget;
    lv_obj_t *update_url;
    const int buzzer_channel = 8;
    const int bgled_channel = buzzer_channel + 1;
    SemaphoreHandle_t mutex;
    bool do_sound = false;
    bool do_manual = false;

public:
    uiElements(int idle_time);
    ~uiElements() = default;

    static void ui_task_wrapper(void *args);
    void ui_task(void);

    inline bool manual(void)
    {
        bool b;
        P(mutex);
        b = do_manual;
        V(mutex);
        return b;
    }
    inline bool play_sound(void)
    {
        bool b;
        P(mutex);
        b = do_sound;
        V(mutex);
        return b;
    }

    void add2ui(ui_tabs_t t, lv_obj_t *e, int dx = 0, int dy = 0);
    inline lv_obj_t *get_tab(ui_tabs_t t) { return tabs[t]; }

    inline void set_mode(ui_modes_t m)
    {
        P(mutex);
        act_mode = m;
        V(mutex);
        lv_scr_load(modes[m]);
    }
    inline ui_modes_t get_mode(void)
    {
        ui_modes_t r;
        P(mutex);
        r = act_mode;
        V(mutex);
        return r;
    }

    static void update_task(lv_task_t *t);
    void update(void);
    bool check_manual(void);
    int biohazard_alarm(void);
};

class uiCommons
{
protected:
    genCircuit *circuit;
    lv_obj_t *area;
    uiElements *ui;

public:
    uiCommons(uiElements *u) : ui(u) {}
    ~uiCommons() = default;

    inline lv_obj_t *get_area() { return area; }
};

/* class defintions */
class button_label_c : public uiCommons
{
    lv_obj_t *obj, *label;
    genCircuit *circuit;

public:
    button_label_c(uiElements *ui, ui_tabs_t t, genCircuit *c, int w, int h);
    ~button_label_c() = default;

    void cb(lv_event_t event);
    void set(uint8_t v);
};

class slider_label_c : public uiCommons
{
    myRange<float> &range;
    bool is_day;
    lv_obj_t *slider, *label;
    lv_obj_t *slider_up_label, *slider_down_label;

public:
    slider_label_c(uiElements *ui, ui_tabs_t t, genCircuit *c, myRange<float> &ra, myRange<float> &cr, int width, int height, bool day = true);
    ~slider_label_c() = default;

    void cb(lv_event_t event);
    void set_label(const char *label, lv_color_t c = LV_COLOR_GRAY);
};
extern slider_label_c *temp_disp;
extern slider_label_c *hum_disp;

class settingsButton : public uiCommons
{
    const char *label_text;
    lv_obj_t *obj, *label;
    bool &state;

public:
    settingsButton(uiElements *ui, ui_tabs_t t, const char *l, bool &var, int width, int height);
    ~settingsButton() = default;

    void cb(lv_event_t event);
};

/* analog display meter */

class analogMeter : public uiCommons
{
    const char *name;
    float val;
    lv_obj_t *lmeter, *temp_label;
    const char *unit;

public:
    analogMeter(uiElements *ui, ui_tabs_t t, const char *n, myRange<float> r, const char *unit);
    ~analogMeter() = default;

    inline void set_val(float v)
    {
        val = v;
        set_act();
    }
    inline void set_act()
    {
        lv_label_set_text_fmt(temp_label, "%d.%02d",
                              static_cast<int>(val),
                              static_cast<int>(static_cast<float>((float)val - static_cast<int>(val)) * 100) % 100);
        lv_linemeter_set_value(lmeter, val);
    }

    inline float get_val() { return val; }
};

extern analogMeter *temp_meter;
extern analogMeter *hum_meter;

/* helpers */
template <typename T, typename I>
class tiny_hash_c
{
    struct pair
    {
        T a;
        I b;
        pair(T x, I y) : a(x), b(y) {}
        ~pair() = default;
    };
    int top, size;
    pair **items;

public:
    tiny_hash_c(size_t s) : top(0), size(s)
    {
        items = (struct pair **)malloc(sizeof(struct pair *) * s);
        //        printf("hash %p with %d items created.\n", this, s);
    }
    ~tiny_hash_c()
    {
        for (int x = 0; x < top; x++)
        {
            free(items[x]);
        }
        free(items);
    }

    inline void store(T x, I y)
    {
        if (top >= size)
            items[999] = new pair(x, y); /* panic here */
        items[top++] = new pair(x, y);
        //printf("hash %p(%d) stores value for %p\n", this, top, x);
    }
    I retrieve(T a)
    {
        //printf("hash %p(%d) - looking up '%p'...", this, top, a);
        for (int x = 0; x < top; x++)
        {
            //printf(" %p", items[x]->a);
            if (items[x]->a == a)
            {
                //printf("\n");
                return items[x]->b;
            }
        }
        /* never reach */
        printf("\nhash not found... *PANIC*\n"); /* hopefully we panic here */
        return items[999]->b;
    }
};

template <typename T>
class myRange
{
    T lbound, ubound;

public:
    myRange(T l, T u) : lbound(l), ubound(u) {}
    ~myRange() = default;

    inline T get_lbound() { return lbound; }
    inline T get_ubound() { return ubound; }
    inline void set_lbound(T v) { lbound = v; }
    inline void set_ubound(T v) { ubound = v; }
    inline void set_lbound(const int v);
    inline void set_ubound(const int v);

    bool is_above(T &v);
    bool is_below(T &v);
    bool is_in(T &v)
    {
#if 0
        Serial.println(&v);
        Serial.println();
        Serial.println(&lbound);
        Serial.println();
        Serial.println(&ubound);
        Serial.println();
#endif
        if (is_above(v) || is_below(v))
            return false;
        return true;
    }
    const String to_string();
    float to_float(const T &v);
};

extern tiny_hash_c<lv_obj_t *, rangeSpinbox<myRange<struct tm>> *> spinbox_callbacks;
extern void spinbox_cb_wrapper(lv_obj_t *obj, lv_event_t e);

template <typename T>
class rangeSpinbox : public uiCommons
{
    const char *label;
    T &range;
    tiny_hash_c<lv_obj_t *, int> btype{6};
    lv_obj_t *spinbox_lower, *spinbox_upper;

public:
    rangeSpinbox(uiElements *ui, ui_tabs_t t, const char *n, T &r, int w, int h) : uiCommons(ui), label(n), range(r)
    {
        area = lv_obj_create(ui->get_tab(t), NULL);
        lv_obj_set_size(area, w, h);
        lv_obj_set_style_local_bg_color(area, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_MAKE(0xc0, 0xc0, 0xc0));

        lv_obj_t *l = lv_label_create(area, NULL);
        lv_label_set_text(l, label);
        lv_obj_align(l, area, LV_ALIGN_IN_LEFT_MID, 0, 0);

        /* start of range */
        lv_obj_t *spinbox = lv_spinbox_create(area, NULL);
        spinbox_lower = spinbox;
        lv_spinbox_set_range(spinbox, 0, 24 * 100);
        lv_spinbox_set_digit_format(spinbox, 4, 2);
        //lv_spinbox_step_prev(spinbox);
        lv_spinbox_set_step(spinbox, 25);
        lv_spinbox_set_value(spinbox, range.to_float(range.get_lbound()) * 100);
        lv_obj_set_width(spinbox, 60);
        lv_obj_align(spinbox, area, LV_ALIGN_IN_TOP_RIGHT, -38, 2);
        lv_textarea_set_cursor_hidden(spinbox, true);

        lv_coord_t sh = lv_obj_get_height(spinbox);
        lv_obj_t *btn = lv_btn_create(area, NULL);
        lv_obj_set_size(btn, sh, sh);
        lv_obj_align(btn, spinbox, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
        lv_theme_apply(btn, LV_THEME_SPINBOX_BTN);
        lv_obj_set_style_local_value_str(btn, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_SYMBOL_PLUS);
        lv_obj_set_event_cb(btn, spinbox_cb_wrapper);
        spinbox_callbacks.store(btn, this);
        btype.store(btn, 0);

        btn = lv_btn_create(area, btn);
        lv_obj_align(btn, spinbox, LV_ALIGN_OUT_LEFT_MID, -5, 0);
        lv_obj_set_event_cb(btn, spinbox_cb_wrapper);
        lv_obj_set_style_local_value_str(btn, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_SYMBOL_MINUS);
        spinbox_callbacks.store(btn, this);
        btype.store(btn, 1);

        /* end of range */
        spinbox_upper = spinbox = lv_spinbox_create(area, NULL);
        lv_spinbox_set_range(spinbox, 0, 24 * 100);
        lv_spinbox_set_digit_format(spinbox, 4, 2);
        //lv_spinbox_step_prev(spinbox);
        lv_spinbox_set_step(spinbox, 25);
        lv_spinbox_set_value(spinbox, range.to_float(range.get_ubound()) * 100);
        lv_obj_set_width(spinbox, 60);
        lv_obj_align(spinbox, spinbox_lower, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
        lv_textarea_set_cursor_hidden(spinbox, true);

        sh = lv_obj_get_height(spinbox);
        btn = lv_btn_create(area, NULL);
        lv_obj_set_size(btn, sh, sh);
        lv_obj_align(btn, spinbox, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
        lv_theme_apply(btn, LV_THEME_SPINBOX_BTN);
        lv_obj_set_style_local_value_str(btn, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_SYMBOL_PLUS);
        lv_obj_set_event_cb(btn, spinbox_cb_wrapper);
        spinbox_callbacks.store(btn, this);
        btype.store(btn, 2);

        btn = lv_btn_create(area, btn);
        lv_obj_align(btn, spinbox, LV_ALIGN_OUT_LEFT_MID, -5, 0);
        lv_obj_set_event_cb(btn, spinbox_cb_wrapper);
        lv_obj_set_style_local_value_str(btn, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, LV_SYMBOL_MINUS);
        spinbox_callbacks.store(btn, this);
        btype.store(btn, 3);
    }
    ~rangeSpinbox() = default;

    void cb(lv_obj_t *o, lv_event_t e)
    {
        int v, t;
        if (e == LV_EVENT_SHORT_CLICKED || e == LV_EVENT_LONG_PRESSED_REPEAT)
        {
            switch (btype.retrieve(o))
            {
            case 0:
                t = lv_spinbox_get_value(spinbox_upper);
                if (lv_spinbox_get_value(spinbox_lower) == t)
                    return;
                lv_spinbox_increment(spinbox_lower);
                v = lv_spinbox_get_value(spinbox_lower);
                range.set_lbound(v);
                break;
            case 1:
                lv_spinbox_decrement(spinbox_lower);
                v = lv_spinbox_get_value(spinbox_lower);
                range.set_lbound(v);
                break;
            case 2:
                lv_spinbox_increment(spinbox_upper);
                v = lv_spinbox_get_value(spinbox_upper);
                range.set_ubound(v);
                break;
            case 3:
                t = lv_spinbox_get_value(spinbox_lower);
                if (lv_spinbox_get_value(spinbox_upper) == t)
                    return;
                lv_spinbox_decrement(spinbox_upper);
                v = lv_spinbox_get_value(spinbox_upper);
                range.set_ubound(v);
                break;
            default:
                break;
            }
        }
    }
};

template <typename T>
class myList
{
    T *items;
    int last = 0;

public:
    myList(size_t s) { items = (T *)malloc(sizeof(T) * s); }
    ~myList() { free(items); }

    inline void add_item(T i) { items[last++] = i; }
    inline T operator[](unsigned int i)
    {
        if (i >= last)
        {
            return (T) nullptr;
        }
        return items[i];
    }
};
#endif