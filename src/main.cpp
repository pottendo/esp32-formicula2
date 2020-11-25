#include <Arduino.h>

#include "ui.h"
#include "io.h"
#include "circuits.h"

/* some globals */
myRange<float> ctrl_temprange{19.0, 31.0};
myRange<float> ctrl_humrange{50.0, 99.99};
const int ui_ss_timeout = 30; /* screensaver timeout in s */
int glob_delay = 10;

// module locals
static myDHT *th1, *th2;
static tempSensor *tsensor;
static humSensor *hsensor;
static timeSwitch *tswitch;
static ioSwitch *io_tswitch, *io_infrared, *io_fan, *io_fog, *io_spare1, *io_spare2;
static myCircuit<timeSwitch> *circuit_timeswitch;
static myCircuit<tempSensor> *circuit_infrared;
static myCircuit<humSensor> *circuit_fan;
static myCircuit<humSensor> *circuit_fog;
//static myCircuit<humSensor> *circuit_spare1;
//static myCircuit<tempSensor> *circuit_spare2;
static const char *circuit_names[] = {
    "Zeitschalter",
    "Infrarot",
    "Luefter",
    "Nebel",
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
    tsensor = new tempSensor(std::list<myDHT *>{th1, th2});
    hsensor = new humSensor(std::list<myDHT *>{th1, th2});
    tswitch = new timeSwitch();

    io_tswitch = new ioSwitch(27);
    io_infrared = new ioSwitch(12);
    io_fan = new ioSwitch(16, true); // inverse logic for fan
    io_fog = new ioSwitch(32);
    io_spare1 = new ioSwitch(26);
    io_spare2 = new ioSwitch(25);

    circuit_timeswitch =
        new myCircuit<timeSwitch>(ui, String(circuit_names[0]), *tswitch, *io_tswitch,
                                  5,
                                  myRange<float>{0.0, 0.0}, ctrl_temprange,
                                  *(new myRange<struct tm>{{0, 0, 7}, {0, 0, 22}}));
    circuit_infrared =
        new myCircuit<tempSensor>(ui, String(circuit_names[1]), *tsensor, *io_infrared,
                                  5,
                                  myRange<float>{24.0, 26.0}, ctrl_temprange);
    circuit_fan =
        new myCircuit<humSensor>(ui, String(circuit_names[2]), *hsensor, *io_fan,
                                 5,
                                 myRange<float>{85.0, 95.0}, ctrl_humrange); // inverse logic for fan as hum drops
    circuit_fog =
        new myCircuit<humSensor>(ui, String(circuit_names[3]), *hsensor, *io_fog,
                                 5,
                                 myRange<float>{75.0, 85.0}, ctrl_humrange);
    ui->set_mode(UI_OPERATIONAL);
#if 0
    circuit_spare1 = new myCircuit<humSensor>(String("Spare1"), *hsensor, *io_spare1, 0.250);
    circuit_spare2 = new myCircuit<tempSensor>(String("Spare2"), *tsensor, *io_spare2, 4);
#endif
}


void loop()
{
    lv_task_handler(); // all tasks (incl. sensors) are managed by lvgl!
#ifdef ALARM_SOUND
    if (ui->get_mode() == UI_ALARM)
        ui->biohazard_alarm();
#endif
    delay(glob_delay);
}