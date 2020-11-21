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

template <typename T>
class myRange;
class uiElements;
class genCircuit;
class uiScreensaver;

/* prototypes */
#define TFT_LED 15
void init_lvgl(void);
uiElements *setup_ui(const int to);
void log_msg(const char *s);
void log_msg(const String s);
extern lv_obj_t *log_handle;
extern myRange<float> ctrl_temprange;
extern myRange<float> ctrl_humrange;
extern int glob_delay;

void setup_wifi(void);
void loop_wifi(void);

template <typename T>
class myRange; // forward declaration

typedef enum
{
    UI_SPLASH = 0,
    UI_OPERATIONAL,
    UI_SCREENSAVER,
    UI_ALARM
} ui_modes_t;

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
    lv_obj_t *tab_status, *tab_controls, *tab_settings;
    lv_obj_t *modes[4];
    int stat_x = 0, stat_y = 0;
    int ctrl_x = 0, ctrl_y = 0;
    int setgs_x = 0, setgs_y = 0;
    ui_modes_t act_mode;
    uiScreensaver saver;
    lv_obj_t *time_widget;
    lv_obj_t *load_widget;
public:
    uiElements(int idle_time);
    ~uiElements() = default;

    inline lv_obj_t *get_status() { return tab_status; }
    inline lv_obj_t *get_controls() { return tab_controls; }
    inline lv_obj_t *get_settings() { return tab_settings; }

    void add_status(lv_obj_t *e);
    void add_control(lv_obj_t *e);
    void add_setting(lv_obj_t *e);

    inline void set_mode(ui_modes_t m) { act_mode = m; lv_scr_load(modes[m]); }
    inline ui_modes_t get_mode(void) { return act_mode; }

    static void update_task(lv_task_t *t);
    void update(void);
};

class uiCommons
{
protected:
    genCircuit *circuit;
    lv_obj_t *area;

public:
    uiCommons() = default;
    ~uiCommons() = default;

    inline lv_obj_t *get_area() { return area; }
};

/* class defintions */
class button_label_c : public uiCommons
{
    const char *label_text;
    lv_obj_t *obj, *label;

public:
    button_label_c(lv_obj_t *parent, genCircuit *c, const char *l, int w, int h, lv_align_t a);
    ~button_label_c() = default;

    void cb(lv_event_t event);
    void set(uint8_t v);
};

class slider_label_c : public uiCommons
{
    const char *label_text;
    myRange<float> &ctrl_range;
    lv_obj_t *slider, *label;
    lv_obj_t *slider_up_label, *slider_down_label;

public:
    slider_label_c(lv_obj_t *parent, genCircuit *c, const char *l, myRange<float> &ra, myRange<float> &da, int width, int height, lv_align_t a);
    ~slider_label_c() = default;

    void cb(lv_event_t event);
    void set_label(const char *label, lv_color_t c = LV_COLOR_GRAY);
};
extern slider_label_c *temp_disp;
extern slider_label_c *hum_disp;

/* analog display meter */

class analogMeter : public uiCommons
{
    const char *name;
    float val;
    lv_obj_t *lmeter, *temp_label;
    const char *unit;

public:
    analogMeter(lv_obj_t *tab, const char *n, myRange<float> r, const char *unit);
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
    tiny_hash_c(size_t s) { items = (struct pair **)malloc(sizeof(struct pair *) * s); }
    ~tiny_hash_c()
    {
        for (int x = 0; x < top; x++)
        {
            free(items[x]);
        }
        free(items);
    }

    inline void store(T x, I y) { items[top++] = new pair(x, y); }
    I retrieve(T a)
    {
        for (int x = 0; x < top; x++)
        {
            if (items[x]->a == a)
            {
                return items[x]->b;
            }
        }
        /* never reach */
        printf("hash not found... *PANIC*\n"); /* hopefully we panic here */
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