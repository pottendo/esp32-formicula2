#include "lvgl.h"
#include "TFT_eSPI.h"

#include "ui.h"

TFT_eSPI tft = TFT_eSPI(); /* TFT instance */
lv_disp_buf_t disp_buf;
lv_color_t buf[LV_HOR_RES_MAX * 10];

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
	lv_init();

	tft.begin();									 /* TFT init */
	tft.setRotation(0);								 /* Portrait orientation */
	uint16_t calData[5] = {365, 3383, 251, 3334, 2}; // for landscape use { 247, 3296, 294, 3429, 1 };
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
}

void setup_ui(void)
{
	extern const lv_img_dsc_t splash_screen;
	lv_obj_t * icon = lv_img_create(lv_scr_act(), NULL);
	lv_img_set_src(icon, &splash_screen);

	static const char *switch_labels[] = {
		"Heizlampe 1: ",
		"Heizmatte 1: ",
		"Heizlampe 2: ",
		"Heizmatte 2: ",
		"Lüfter Nest: ",
		"Befeuchtung: ",
		NULL};
	static int offs = 32;

	for (int x = 0; switch_labels[x]; x++)
	{
		new button_label_c(switch_labels[x], 10, 10 + x * offs, LV_ALIGN_IN_TOP_LEFT);
	}

	temp_disp = new slider_label_c("Temp:    ", 10, -30, 22, 28, 120, LV_ALIGN_IN_BOTTOM_LEFT);
	hum_disp = new slider_label_c("Feuchte: ", 10, -76, 30, 99, 120, LV_ALIGN_IN_BOTTOM_LEFT);
}