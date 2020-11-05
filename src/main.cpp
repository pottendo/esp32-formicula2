#include <Arduino.h>
#include "ui.h"
#include "io.h"

slider_label_c *temp_disp;
slider_label_c *hum_disp;
myDHT *th1;//, *th2;

void update_sensors(void)
{
  static char b[64];
  sprintf(b, "T: %.2f", (float) th1->get_temp());
  temp_disp->set_label(b);
  sprintf(b, "L: %.2f", (float) th1->get_hum());
  hum_disp->set_label(b);

  //printf(b);
}

void setup()
{
  Serial.begin(115200);
  Serial.printf("Hello ESP32 - formicula control\n");
  init_lvgl();
  setup_ui();
  th1 = new myDHT(12);
  //th2 = new myDHT(14);
}

void loop()
{
  // put your main code here, to run repeatedly:
  delay(10);
  update_sensors();
  lv_task_handler();
}