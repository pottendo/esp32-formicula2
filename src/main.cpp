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
myRange<float> ctrl_temprange1{21.0, 31.0};
myRange<float> ctrl_humrange1{60.0, 90.0};
myRange<float> ctrl_temprange2{28.0, 35.0};
myRange<float> ctrl_humrange2{60.0, 90.0};
myRange<struct tm> def_day{{0, 0, 7}, {0, 0, 18}};
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
static remoteSensor *fcce_temp, *berg_temp, *erde_temp;
static remoteSensor *fcce_hum, *berg_hum, *erde_hum;

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
    Serial.printf("Formicula Control Centre (AC OTA) - V1.1\n");

    ui = setup_ui(ui_ss_timeout);
    //setup_io();
    setup_wifi(ui);
    setup_mqtt(ui);
    setup_logger();

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
                                 ctrl_temprange1,
                                 nullptr,
                                 *(new myRange<struct tm>{{0, 0, 9}, {0, 0, 16}}));

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

    fcce_temp = new remoteSensor{ui, "/FCCETemp", 27.0};
    fcce_hum = new remoteSensor{ui, "/FCCEHum", 65.0};
    erde_temp = new remoteSensor{ui, "/ErdeTemp", 27.0};
    erde_hum = new remoteSensor{ui, "/ErdeHum", 65.0};
    berg_temp = new remoteSensor{ui, "/BergTemp", 27.0};
    berg_hum = new remoteSensor{ui, "/BergHum", 65.0};

#if 0
    genSensor *avg_temp1 = new avgSensor(ui, "Total Average Temp", std::list<genSensor *>{berg_temp});
    genSensor *avg_hum1 = new avgSensor(ui, "Total Average Hum", std::list<genSensor *>{erde_hum, berg_hum});
    genSensor *avg_temp2 = new avgSensor(ui, "Total Average Temp", std::list<genSensor *>{berg_temp});
    genSensor *avg_hum2 = new avgSensor(ui, "Total Average Hum", std::list<genSensor *>{erde_hum, berg_hum});
#endif
    ui->set_avg_sensors(berg_temp, erde_temp, berg_hum, erde_hum);

    circuit_infrared =
        new myCircuit<genSensor>(ui, "Infrarot", *berg_temp, *io_infrared,
                                 5,
                                 myRange<float>{27.0, 29.0},
                                 myRange<float>{24.0, 27.0},
                                 ctrl_temprange1);
    circuit_heater =
        new myCircuit<genSensor>(ui, "Heizmatte", *erde_temp, *io_heater,
                                 5,
                                 myRange<float>{30.0, 31.0},
                                 myRange<float>{29.0, 30.0},
                                 ctrl_temprange2,
                                 [](genCircuit *c) {
                                     static unsigned long last = 0;
                                     static uint8_t state = HIGH;

                                     if ((millis() - last) > 1000 * 60 * 15) /* for 15 minutes interval */
                                     {
                                         log_msg(c->get_name() + ": running fallback");
                                         c->io_set(((state == HIGH) ? LOW : HIGH), true, true);
                                         state = !state;
                                         last = millis();
                                     }
                                 });
    circuit_fan =
        new myCircuit<genSensor>(ui, "Luefter", *berg_hum, *io_fan,
                                 5,
                                 myRange<float>{72.0, 80.0},
                                 myRange<float>{72.0, 80.0},
                                 ctrl_humrange1,
                                 [](genCircuit *c) {
                                     static unsigned long last = 0;
                                     static uint8_t state = HIGH;

                                     if ((millis() - last) > 1000 * 60 * 10) /* for 10 minutes interval */
                                     {
                                         log_msg(c->get_name() + ": running fallback");
                                         c->io_set(((state == HIGH) ? LOW : HIGH), true, true);
                                         state = !state;
                                         last = millis();
                                     }
                                 }); // inverse logic for fan as hum drops

    circuit_dhum =
        new myCircuit<genSensor>(ui, "Nebler", *berg_hum, *io_fog,
                                 5,
                                 myRange<float>{65.0, 68.0},
                                 myRange<float>{65.0, 68.0},
                                 ctrl_humrange1);

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

    delay(25);
    ui->set_mode(UI_OPERATIONAL);
    //vTaskPrioritySet(nullptr, configMAX_PRIORITIES - 6);

    printf("main priority: %d\n", uxTaskPriorityGet(nullptr));
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