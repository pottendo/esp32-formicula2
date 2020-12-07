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
 
#include <Arduino.h>

#include "ui.h"
#include "io.h"
#include "circuits.h"

/* some globals */
myRange<float> ctrl_temprange{20.0, 30.0};
myRange<float> ctrl_humrange{50.0, 99.99};
myRange<struct tm> def_day{{0, 0, 7}, {0, 0, 19}};
const int ui_ss_timeout = 30; /* screensaver timeout in s */
int glob_delay = 10;

// module locals
static myDHT *th1, *th2;
static tempSensor *tsensor_berg, *tsensor_erde;
static humSensor *hsensor_berg, *hsensor_erde;
static timeSwitch *tswitch;
static ioDigitalIO *io_tswitch, *io_infrared, *io_heater, *io_fan, *io_fog /*, *io_spare2*/;
static ioServo *io_humswitch;
static myCircuit<timeSwitch> *circuit_timeswitch;
static myCircuit<tempSensor> *circuit_infrared;
static myCircuit<tempSensor> *circuit_heater;
static myCircuit<humSensor> *circuit_fan;
static myCircuit<humSensor> *circuit_fog;
static myCircuit<humSensor> *circuit_fog2;
static myCircuit<humSensor> *circuit_dhum;
//static myCircuit<tempSensor> *circuit_spare2;

static uiElements *ui;

void setup()
{
    Serial.begin(115200);
    Serial.printf("Formicula control(AC OTA) - V1.1\n");

    init_lvgl();
    ui = setup_ui(ui_ss_timeout);
    setup_wifi();
    //setup_mqtt();
    th1 = new myDHT("Berg", 17, ui, DHTesp::DHT22);
    th2 = new myDHT("Erde", 13, ui, DHTesp::DHT22);
    tswitch = new timeSwitch();
    tsensor_berg = new tempSensor(std::list<myDHT *>{th1}); // kreis "Infrarot"
    tsensor_erde = new tempSensor(std::list<myDHT *>{th2}); // kreis "Heizmatte"

    hsensor_berg = new humSensor(std::list<myDHT *>{th1});
    hsensor_erde = new humSensor(std::list<myDHT *>{th2});

    io_tswitch = new ioDigitalIO(27);
    io_infrared = new ioDigitalIO(12);
    io_heater = new ioDigitalIO(26);
    io_fan = new ioDigitalIO(16, true); // inverse logic for fan
    io_fog = new ioDigitalIO(32);
    //io_spare2 = new ioDigitalIO(25);

    io_humswitch = new ioServo(14, false, 0, 120);      /* should be 14, once HW is ready FIXME */

    circuit_timeswitch =
        new myCircuit<timeSwitch>(ui, "Zeitschalter", *tswitch, *io_tswitch,
                                  5,
                                  myRange<float>{0.0, 0.0},
                                  myRange<float>{0.0, 0.0},
                                  ctrl_temprange,
                                  *(new myRange<struct tm>{{0, 0, 7}, {0, 0, 22}}));
    circuit_infrared =
        new myCircuit<tempSensor>(ui, "Infrarot", *tsensor_berg, *io_infrared,
                                  5,
                                  myRange<float>{28.0, 29.0},
                                  myRange<float>{24.0, 27.0},
                                  ctrl_temprange);
    circuit_heater =
        new myCircuit<tempSensor>(ui, "Heizmatte", *tsensor_erde, *io_heater,
                                  5,
                                  myRange<float>{27.0, 28.0},
                                  myRange<float>{27.0, 28.0},
                                  ctrl_temprange);
    circuit_fan =
        new myCircuit<humSensor>(ui, "Luefter", *hsensor_berg, *io_fan,
                                 5,
                                 myRange<float>{65.0, 75.0},
                                 myRange<float>{65.0, 75.0},
                                 ctrl_humrange); // inverse logic for fan as hum drops
    circuit_fog =
        new myCircuit<humSensor>(ui, "Nebel Berg", *hsensor_berg, *io_fog,
                                 5,
                                 myRange<float>{60.0, 65.0},
                                 myRange<float>{60.0, 65.0},
                                 ctrl_humrange);
    circuit_fog2 =
        new myCircuit<humSensor>(ui, "Nebel Erde", *hsensor_erde, *io_fog,
                                 5,
                                 myRange<float>{65.0, 80.0},
                                 myRange<float>{65.0, 80.0},
                                 ctrl_humrange);
    circuit_dhum = 
        new myCircuit<humSensor>(ui, "Feuchtigkeit", *hsensor_erde, *io_humswitch,
                                 5,
                                 myRange<float>{65.0, 80.0},
                                 myRange<float>{65.0, 80.0},
                                 ctrl_humrange);


    ui->set_mode(UI_OPERATIONAL);
#if 0
    circuit_spare2 = new myCircuit<tempSensor>(String("Spare2"), *tsensor, *io_spare2, 4);
#endif
}

void loop()
{
//    server.handleClient();
    loop_wifi();
    //loop_mqtt();

    lv_task_handler(); // all tasks (incl. sensors) are managed by lvgl!
    delay(glob_delay);
}