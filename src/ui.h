#ifndef __ui_h__
#define __ui_h__

#include <lvgl.h>
#include <stdio.h>
#include <stdlib.h>
#include <WString.h>

/* prototypes */
#define TFT_LED 15
void init_lvgl(void);
void setup_ui(void);
void log_msg(const char *s);
void log_msg(const String s);
extern lv_obj_t *log_handle;

/* class defintions */
class button_label_c
{
    lv_obj_t *parent;
    const char *label_text;
    lv_obj_t *obj, *label;
    int x_pos, y_pos;

public:
    button_label_c(lv_obj_t *tab, const char *l, int x, int y, lv_align_t a);
    ~button_label_c() = default;

    void cb(lv_event_t event);
};

class slider_label_c
{
    lv_obj_t *parent;
    const char *label_text;
    lv_obj_t *slider, *label;
    lv_obj_t *slider_up_label, *slider_down_label;
    int x_pos, y_pos;
    int min, max;
    int width;

public:
    slider_label_c(lv_obj_t *tab, const char *l, int x, int y, int min, int max, int width, lv_align_t a);
    ~slider_label_c() = default;

    void cb(lv_event_t event);
    void set_label(const char *label, lv_color_t c = LV_COLOR_GRAY);
};
extern slider_label_c *temp_disp;
extern slider_label_c *hum_disp;

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
        printf("hash %0x not found... *PANIC*", (int)a);
        return items[0]->b;
    }
};

#endif