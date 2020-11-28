#include <Arduino.h>

#include "ui.h"
#include "io.h"
#include "circuits.h"

/* some globals */
myRange<float> ctrl_temprange{20.0, 30.0};
myRange<float> ctrl_humrange{50.0, 99.99};
const int ui_ss_timeout = 30; /* screensaver timeout in s */
int glob_delay = 10;

// module locals
static myDHT *th1, *th2;
static tempSensor *tsensor_berg, *tsensor_erde;
static humSensor *hsensor_berg, *hsensor_erde;
static timeSwitch *tswitch;
static ioSwitch *io_tswitch, *io_infrared, *io_heater, *io_fan, *io_fog, *io_spare2;
static myCircuit<timeSwitch> *circuit_timeswitch;
static myCircuit<tempSensor> *circuit_infrared;
static myCircuit<tempSensor> *circuit_heater;
static myCircuit<humSensor> *circuit_fan;
static myCircuit<humSensor> *circuit_fog;
static myCircuit<humSensor> *circuit_fog2;
//static myCircuit<humSensor> *circuit_spare1;
//static myCircuit<tempSensor> *circuit_spare2;
static const char *circuit_names[] = {
    "Zeitschalter",
    "Infrarot",
    "Heizmatte", 
    "Luefter",
    "Nebel Berg",
    "Nebel Erde",
    NULL};
static uiElements *ui;

void setup()
{
    Serial.begin(115200);
    Serial.printf("Hello ESP32 - formicula control\n");

    init_lvgl();
    ui = setup_ui(ui_ss_timeout);
    setup_wifi();
    th1 = new myDHT("Berg", 17, ui);
    th2 = new myDHT("Erde", 13, ui);
    tswitch = new timeSwitch();
    tsensor_berg = new tempSensor(std::list<myDHT *>{th1}); // kreis "Infrarot"
    tsensor_erde = new tempSensor(std::list<myDHT *>{th2}); // kreis "Heizmatte"

    hsensor_berg = new humSensor(std::list<myDHT *>{th1});
    hsensor_erde = new humSensor(std::list<myDHT *>{th2});

    io_tswitch = new ioSwitch(27);
    io_infrared = new ioSwitch(12);
    io_heater = new ioSwitch(26);
    io_fan = new ioSwitch(16, true); // inverse logic for fan
    io_fog = new ioSwitch(32);
    io_spare2 = new ioSwitch(25);

    circuit_timeswitch =
        new myCircuit<timeSwitch>(ui, String(circuit_names[0]), *tswitch, *io_tswitch,
                                  5,
                                  myRange<float>{0.0, 0.0}, ctrl_temprange,
                                  *(new myRange<struct tm>{{0, 0, 7}, {0, 0, 22}}));
    circuit_infrared =
        new myCircuit<tempSensor>(ui, String(circuit_names[1]), *tsensor_berg, *io_infrared,
                                  5,
                                  myRange<float>{28.0, 29.0}, ctrl_temprange);
    circuit_heater =
        new myCircuit<tempSensor>(ui, String(circuit_names[2]), *tsensor_erde, *io_infrared,
                                  5,
                                  myRange<float>{27.0, 28.0}, ctrl_temprange);
    circuit_fan =
        new myCircuit<humSensor>(ui, String(circuit_names[3]), *hsensor_berg, *io_fan,
                                 5,
                                 myRange<float>{65.0, 75.0}, ctrl_humrange); // inverse logic for fan as hum drops
    circuit_fog =
        new myCircuit<humSensor>(ui, String(circuit_names[4]), *hsensor_berg, *io_fog,
                                 5,
                                 myRange<float>{60.0, 65.0}, ctrl_humrange);
    circuit_fog2 =
        new myCircuit<humSensor>(ui, String(circuit_names[5]), *hsensor_erde, *io_fog,
                                 5,
                                 myRange<float>{65.0, 75.0}, ctrl_humrange);
    ui->set_mode(UI_OPERATIONAL);
#if 0
    circuit_spare2 = new myCircuit<tempSensor>(String("Spare2"), *tsensor, *io_spare2, 4);
#endif
}

void loop()
{
    lv_task_handler(); // all tasks (incl. sensors) are managed by lvgl!
    delay(glob_delay);
}