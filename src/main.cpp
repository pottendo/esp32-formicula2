#include <Arduino.h>
//#include <esp_task_wdt.h>

#include "ui.h"
#include "io.h"
#include "circuits.h"

/* some globals */
slider_label_c *temp_disp;
slider_label_c *hum_disp;
analogMeter *temp_meter;
analogMeter *hum_meter;

myDHT *th1, *th2;
tempSensor *tsensor;
humSensor *hsensor;
ioSwitch *io_halogen, *io_infrared, *io_fan, *io_fog, *io_spare1, *io_spare2;
myCircuit<tempSensor> *circuit_halogen;
myCircuit<tempSensor> *circuit_infrared;
myCircuit<humSensor> *circuit_fan;
myCircuit<humSensor> *circuit_fog;
myCircuit<humSensor> *circuit_spare1;
myCircuit<tempSensor> *circuit_spare2;
const char *circuit_names[] = {
    "Halogen",
    "Infrared",
    "Fan",
    "Fog",
    NULL};
tiny_hash_c<String, genCircuit *> circuit_objs{10};

myRange<float> def_temprange{25.0, 27.0};
myRange<float> def_humrange{85.0, 90.0};
myRange<float> ctrl_temprange{22.0, 30.0};
myRange<float> ctrl_humrange{80.0, 99.99};

void update_sensors(void)
{
    static char b[64];
    sprintf(b, "T: %.2f", (float)th1->get_temp());
    temp_disp->set_label(b);
    sprintf(b, "L: %.2f", (float)th1->get_hum());
    hum_disp->set_label(b);

    //printf(b);
}

void setup()
{
    Serial.begin(115200);
    Serial.printf("Hello ESP32 - formicula control\n");

    //esp_task_wdt_init(5, true);

    init_lvgl();
    setup_wifi();
    setup_ui();
    th1 = new myDHT(17);
    th2 = new myDHT(13);
    tsensor = new tempSensor(std::list<myDHT *>{th1, th2});
    hsensor = new humSensor(std::list<myDHT *>{th1, th2});
    io_halogen = new ioSwitch(27);
    io_infrared = new ioSwitch(12);
    io_fan = new ioSwitch(16);
    io_fog = new ioSwitch(32);
    io_spare1 = new ioSwitch(25);
    io_spare2 = new ioSwitch(26);

    circuit_halogen = new myCircuit<tempSensor>(String(circuit_names[0]), *tsensor, *io_halogen, 10, def_temprange);
    circuit_objs.store(String(circuit_names[0]), circuit_halogen);
    circuit_infrared = new myCircuit<tempSensor>(String(circuit_names[1]), *tsensor, *io_infrared, 20, def_temprange);
    circuit_objs.store(String(circuit_names[1]), circuit_infrared);
    circuit_fan = new myCircuit<humSensor>(String(circuit_names[2]), *hsensor, *io_fan, 30, def_humrange, true);      // inverse logic for fan as hum drops
    circuit_objs.store(String(circuit_names[2]), circuit_fan);
    circuit_fog = new myCircuit<humSensor>(String(circuit_names[3]), *hsensor, *io_fog, 50, def_humrange);
    circuit_objs.store(String(circuit_names[3]), circuit_fog);
#if 0
    circuit_spare1 = new myCircuit<humSensor>(String("Spare1"), *hsensor, *io_spare1, 0.250);
    circuit_spare2 = new myCircuit<tempSensor>(String("Spare2"), *tsensor, *io_spare2, 4);
#endif
}

void loop()
{
    // put your main code here, to run repeatedly:
    delay(10);
    //update_sensors();
    //loop_wifi();
    lv_task_handler();
}