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

#ifndef __mqtt_h__
#define __mqtt_h__

#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include "ui.h"

class myMqtt
{
    typedef enum { NO_CONN, CONN, RECONN } conn_stat_t;
    conn_stat_t conn_stat = NO_CONN;
    TaskHandle_t th = nullptr;
    int reconnects = 0;
    unsigned long last = 0;
    unsigned long connection_wd = 0;

protected:
    SemaphoreHandle_t mutex;
    MQTTClient *client;
    const char *id;
    const char *name;
    const char *user;
    const char *pw;
    typedef void (*upstream_fn)(String &topic, String &payload);
    upstream_fn up_fn;

public:
    myMqtt(const char *id, upstream_fn up_fn = nullptr, const char *name = nullptr, const char *user = "", const char *pw = "");
    virtual ~myMqtt() { delete client; vSemaphoreDelete(mutex); log_msg(String(name) + " destroyed.");};

    void set_conn_stat(conn_stat_t s) { P(mutex); conn_stat = s; V(mutex); }
    inline const char *get_name(void) { return name; }
    void loop(void);
    bool connected(void);
    void reconnect(void);
    void cleanup(void);
    void reconnect_body(void);
    virtual bool connect(void) = 0;
    void register_callback(upstream_fn fn);
    void publish(String &topic, String &payload, int qos = 0);
};

class myMqttSec : public myMqtt
{
    WiFiClientSecure net;

public:
    myMqttSec(const char *id, const char *server, upstream_fn up_fn = nullptr, const char *name = nullptr, int port = 8883, const char *user = "", const char *pw = "");
    virtual ~myMqttSec() = default;

    virtual bool connect(void) override;
};

class myMqttLocal : public myMqtt
{
    WiFiClient net;
    char id_buf[16];
    int nr = 0;
public:
    myMqttLocal(const char *id, const char *server, upstream_fn up_fn = nullptr, const char *name = nullptr, int port = 1883, const char *user = "", const char *pw = "");
    virtual ~myMqttLocal() = default;

    virtual bool connect(void) override;
    char *get_id(void);
};

void setup_mqtt(uiElements *ui);
void loop_mqtt(void);
void mqtt_register_sensor(genSensor *s);
void mqtt_register_circuit(genCircuit *s);
myMqtt *mqtt_register_logger(void);
bool mqtt_connect(MQTTClient *c);
void mqtt_publish(String topic, String msg, myMqtt *c = nullptr, int qos = 0);
void mqtt_P(void);
void mqtt_V(void);

#endif