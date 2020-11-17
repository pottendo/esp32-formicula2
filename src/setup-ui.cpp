#include "lvgl.h"
#include "TFT_eSPI.h"

#include "ui.h"
#include "circuits.h"

TFT_eSPI tft = TFT_eSPI(); /* TFT instance */
lv_disp_buf_t disp_buf;
lv_color_t buf[LV_HOR_RES_MAX * 10];
lv_obj_t *log_handle;
xSemaphoreHandle ui_mutex;
tiny_hash_c<String, button_label_c *> button_objs{10};

/* helpers */
void log_msg(const char *s)
{
#if 0
	P(ui_mutex);
	lv_textarea_add_text(log_handle, s);
	lv_textarea_add_char(log_handle, '\n');
	V(ui_mutex);
#endif
	printf("%s\n", s);
}

void log_msg(String s)
{
	log_msg(s.c_str());
}

#if 0
static void lv_refresher(void *t)
{
	while (1)
	{
		//printf("lv_refresher running...\n");
		P(ui_mutex);
		lv_task_handler();
		V(ui_mutex);
		delay(10);
	}
}
#endif

/* Display flushing */
static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area,
						  lv_color_t *color_p)
{
	uint32_t w = (area->x2 - area->x1 + 1);
	uint32_t h = (area->y2 - area->y1 + 1);
	tft.startWrite();
	tft.setAddrWindow(area->x1, area->y1, w, h);
	tft.pushColors(&color_p->full, w * h, true);
	tft.endWrite();

	lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
static bool my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
	uint16_t touchX, touchY;

	bool touched = tft.getTouch(&touchX, &touchY, 600);
	if (!touched)
	{
		data->state = LV_INDEV_STATE_REL;
		return false;
	}
	else
	{
		data->state = LV_INDEV_STATE_PR;
	}

	if (touchX > screenWidth || touchY > screenHeight)
	{
		Serial.println("Y or y outside of expected parameters..");
		Serial.print("y:");
		Serial.print(touchX);
		Serial.print(" x:");
		Serial.print(touchY);
	}
	else
	{
		/*Set the coordinates*/
		data->point.x = touchX;
		data->point.y = touchY;
#if 0
		Serial.print("Data x");
		Serial.println(touchX);

		Serial.print("Data y");
		Serial.println(touchY);
#endif
	}

	return false; /*Return `false` because we are not buffering and no more data to read*/
}

void init_lvgl(void)
{
	pinMode(TFT_LED, OUTPUT);
	digitalWrite(TFT_LED, LOW); // Display-Beleuchtung einschalten

	ui_mutex = xSemaphoreCreateMutex();
	xSemaphoreGive(ui_mutex);

	lv_init();

	tft.begin();									 /* TFT init */
	tft.setRotation(0);								 /* Portrait orientation */
	uint16_t calData[5] = {365, 3383, 251, 3334, 2}; // for portrait
	//uint16_t calData[5] = { 247, 3296, 294, 3429, 1 }; // for landscape
	//tft.calibrateTouch(calData,TFT_MAGENTA, TFT_BLACK, 15);
	printf("caldata: %d, %d, %d, %d, %d\n", calData[0], calData[1], calData[2], calData[3], calData[4]);
	tft.setTouch(calData);

	lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);

	/*Initialize the display*/
	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.hor_res = 240;
	disp_drv.ver_res = 320;
	disp_drv.flush_cb = my_disp_flush;
	disp_drv.buffer = &disp_buf;
	lv_disp_drv_register(&disp_drv);

	/*Initialize the (dummy) input device driver*/
	lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = my_touchpad_read;
	lv_indev_drv_register(&indev_drv);

	lv_task_enable(true);
}

uiElements *setup_ui(void)
{
	uiElements *ui = new uiElements();

	/* tab 3 - Config */

	/* tab N logs */
#if 0
	log_handle = lv_textarea_create(tab2, NULL);
	lv_obj_set_size(log_handle, 200, 200);
	lv_obj_align(log_handle, NULL, LV_ALIGN_CENTER, 0, 0);
	lv_textarea_set_text(log_handle, "Log started...\n");
#endif
	log_msg("GUI Setup finished.");
	return ui;
}