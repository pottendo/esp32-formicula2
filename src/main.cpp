#include <Arduino.h>
#include "ui.h"

slider_label_c *temp_disp;
slider_label_c *hum_disp;
float act_temp = 42.6;
float act_hum = 54.4;

void update_sensors(void)
{
  static char b[64];
  sprintf(b, "T: %.2f", act_temp);
  temp_disp->set_label(b);
  sprintf(b, "L: %.2f", act_hum);
  hum_disp->set_label(b);

  //printf(b);
}

void setup()
{
  Serial.begin(115200);
  Serial.printf("Hello ESP32 - formicula control\n");
  init_lvgl();
  setup_ui();
}

void loop()
{
  // put your main code here, to run repeatedly:
  delay(10);
  act_temp += ((rand() % 10) - 5.0) / 100.0;
  act_hum += ((rand() % 10) - 5.0) / 100.0;
  update_sensors();
  lv_task_handler();
}