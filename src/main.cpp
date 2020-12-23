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
#if 0
static myDHT *th1, *th2;
static myDHT *dhtms_1;
static myDS18B20 *dsls_1;
static myBM280 *bmems_1;
static avgSensor *tsavg_erde;
static humSensorMulti *hsavg_erde_hin, *hsavg_berg;
static tempSensorMulti *tsavg_erde_hin, *tsavg_berg;
#endif
static remoteSensor *trem1, *trem2, *trem3;
static remoteSensor *hrem1, *hrem2, *hrem3;

static timeSwitch *tswitch;
static ioDigitalIO *io_tswitch, *io_infrared, *io_heater, *io_fan, *io_fog /*, *io_spare2*/;
static ioServo *io_humswitch;
static myCircuit<genSensor> *circuit_timeswitch;
static myCircuit<genSensor> *circuit_infrared;
static myCircuit<genSensor> *circuit_heater;
static myCircuit<genSensor> *circuit_fan;
#if 0
static myCircuit<genSensor> *circuit_fog;
static myCircuit<genSensor> *circuit_fog2;
#endif
static myCircuit<genSensor> *circuit_dhum;
//static myCircuit<tempSensor> *circuit_spare2;

static uiElements *ui;

void setup()
{
    Serial.begin(115200);
    Serial.printf("Formicula control(AC OTA) - V1.1\n");

    init_lvgl();
    ui = setup_ui(ui_ss_timeout);
    //setup_io();
    setup_wifi();
    setup_mqtt(ui);
#if 0    
    th1 = new myDHT("Berg", 17, ui, DHTesp::DHT22);
    th2 = new myDHT("Erde", 13, ui, DHTesp::DHT22);
    tsensor_berg = new tempSensor(std::list<myDHT *>{th1}); // kreis "Infrarot"
    tsensor_erde = new tempSensor(std::list<myDHT *>{th2}); // kreis "Heizmatte"

    hsensor_berg = new humSensor(std::list<myDHT *>{th1});
    hsensor_erde = new humSensor(std::list<myDHT *>{th2});
#endif
    tswitch = new timeSwitch(ui, "Tag/Nacht");
    io_tswitch = new ioDigitalIO(27);
    io_infrared = new ioDigitalIO(12);
    io_heater = new ioDigitalIO(26);
    io_fan = new ioDigitalIO(16, true); // inverse logic for fan
    io_fog = new ioDigitalIO(32);
    //io_spare2 = new ioDigitalIO(25);
    io_humswitch = new ioServo(14, false, 0, 120);

    circuit_timeswitch =
        new myCircuit<genSensor>(ui, "Zeitschalter", *tswitch, *io_tswitch,
                                 5,
                                 myRange<float>{0.0, 0.0},
                                 myRange<float>{0.0, 0.0},
                                 ctrl_temprange,
                                 nullptr,
                                 *(new myRange<struct tm>{{0, 0, 7}, {0, 0, 22}}));

#if 0
    dsls_1 = new myDS18B20(ui, "/Erde", 17, 5000);
    tsavg_erde = new avgSensor(ui, "/avgTempErde", std::list<genSensor *>{dsls_1}); // Circuit heater input sensor
    
    dhtms_1 = new myDHT(ui, "/Erdehin", 13, DHTesp::DHT22, 10000);
    hsavg_erde_hin = new humSensorMulti(ui, "/avgHumErdehin", std::list<genSensor *>{dhtms_1});
    tsavg_erde_hin = new tempSensorMulti(ui, "/avgTempErdehin", std::list<genSensor *>{dhtms_1});
#endif
#if 0
    /* won't work with touch display due to IC2 issues */
    bmems_1 = new myBM280(ui, "/tempBME280");
    hsavg_berg = new humSensorMulti(ui, "/humBME280", std::list<genSensor *>{bmems_1});
    tsavg_berg = new tempSensorMulti(ui, "/tempBME280", std::list<genSensor *>{bmems_1});
#endif

    trem1 = new remoteSensor{ui, "/FCCETemp", 27.0};
    hrem1 = new remoteSensor{ui, "/FCCEHum", 65.0};
    trem2 = new remoteSensor{ui, "/ErdeTemp", 27.0};
    hrem2 = new remoteSensor{ui, "/ErdeHum", 65.0};
    trem3 = new remoteSensor{ui, "/BergTemp", 27.0};
    hrem3 = new remoteSensor{ui, "/BergHum", 65.0};

    genSensor *avg_temp = new avgSensor(ui, "Total Average Temp", std::list<genSensor *>{trem2, trem3});
    genSensor *avg_hum = new avgSensor(ui, "Total Average Hum", std::list<genSensor *>{hrem2, hrem3});
    ui->set_avg_sensors(avg_temp, avg_hum);

    circuit_infrared =
        new myCircuit<genSensor>(ui, "Infrarot", *trem3, *io_infrared,
                                 5,
                                 myRange<float>{28.0, 29.0},
                                 myRange<float>{24.0, 27.0},
                                 ctrl_temprange);
    circuit_heater =
        new myCircuit<genSensor>(ui, "Heizmatte", *trem2, *io_heater,
                                 5,
                                 myRange<float>{27.0, 28.0},
                                 myRange<float>{27.0, 28.0},
                                 ctrl_temprange,
                                 [](genCircuit *c) {
                                     static unsigned long last = 0;
                                     static uint8_t state = HIGH;

                                     if ((millis() - last) > 1000 * 3 /* *60*30 for 30 minutes interval */)
                                     {
                                         log_msg(c->get_name() + ": running fallback");
                                         c->io_set(((state == HIGH) ? LOW : HIGH), true, true);
                                         state = !state;
                                         last = millis();
                                     }
                                 });
    circuit_fan =
        new myCircuit<genSensor>(ui, "Luefter", *hrem3, *io_fan,
                                 5,
                                 myRange<float>{65.0, 75.0},
                                 myRange<float>{65.0, 75.0},
                                 ctrl_humrange); // inverse logic for fan as hum drops
#if 0
    circuit_fog =
        new myCircuit<genSensor>(ui, "Nebel Berg", *hrem2, *io_fog,
                                 5,
                                 myRange<float>{60.0, 65.0},
                                 myRange<float>{60.0, 65.0},
                                 ctrl_humrange);
    circuit_fog2 =
        new myCircuit<genSensor>(ui, "Nebel Erde", *hrem3, *io_fog,
                                 5,
                                 myRange<float>{65.0, 80.0},
                                 myRange<float>{65.0, 80.0},
                                 ctrl_humrange);
#endif
    circuit_dhum =
        new myCircuit<genSensor>(ui, "Feuchtigkeit", *hrem2, *io_humswitch,
                                 5,
                                 myRange<float>{65.0, 80.0},
                                 myRange<float>{65.0, 80.0},
                                 ctrl_humrange);
    delay(25);
    ui->set_mode(UI_OPERATIONAL);
#if 0
    circuit_spare2 = new myCircuit<tempSensor>(String("Spare2"), *tsensor, *io_spare2, 4);
#endif
}

void loop()
{
    loop_wifi();
    loop_mqtt();

    ui->ui_P();        // mqtt & alarm handling is separate and if interaction with UI is needed, masterlock is needed.
    lv_task_handler(); // most tasks (incl. local sensors) are managed by lvgl!
    ui->ui_V();
    delay(glob_delay);
}