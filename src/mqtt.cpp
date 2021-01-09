#include <Arduino.h>
#include <MQTT.h>
#include <list>
#include <ESPmDNS.h>
#include <WiFiClientSecure.h>

#include "ui.h"
#include "circuits.h"
#include "mqtt.h"

static uiElements *ui;

static SemaphoreHandle_t mqtt_mutex; /* ensure exclusive access to mqtt client lib */
static std::list<genSensor *> remote_sensors;
static std::list<genCircuit *> circuits;
static std::list<myMqtt *> mqtt_connections;

static myMqtt *fcce_connection;       /* specific for fcc/fcce application, must exist */
static const char *client_id = "fcc"; /* identify fcc uniquely on mqtt */

static void fcce_upstream(String &t, String &payload);

/* to be exported to a separate .h */
#define MQTT_FCCE "fcce"
//#define MQTT_LOG "node02.myqtthub.com"
#define MQTT_LOG "pottendo-pi30-phono"
#define MQTT_LOG_USER "fcc-user"
#define MQTT_LOG_PW "foRmicula666"

void setup_mqtt(uiElements *u)
{
    ui = u;
    mqtt_mutex = xSemaphoreCreateMutex();
    V(mqtt_mutex);

    fcce_connection = new myMqttLocal(client_id, MQTT_FCCE, fcce_upstream, "fcce broker");
}

void loop_mqtt()
{
    std::for_each(mqtt_connections.begin(), mqtt_connections.end(),
                  [](myMqtt *c) {
                      if (c->connected())
                          c->loop();
                      else
                          c->reconnect();
                  });
}

void mqtt_publish(String topic, String msg, myMqtt *c, int qos)
{
    P(mqtt_mutex); /* ensure mut-excl acces into MQTTClient library */
    myMqtt *client = (c ? c : fcce_connection);
    client->publish(topic, msg, qos);
    V(mqtt_mutex);
}

/* sensors register to be called when topic /fcce/<SENSORNAME> appears */
void mqtt_register_sensor(genSensor *s)
{
    remote_sensors.push_back(s);
}

/* circuit register to be called when topic /fcce/<CIRCUITNAME> appears */
void mqtt_register_circuit(genCircuit *s)
{
    circuits.push_back(s);
}

myMqtt *mqtt_register_logger(void)
{
    return new myMqttLocal(client_id, MQTT_LOG, nullptr, "log broker");
}

/* class myMqtt broker */
myMqtt::myMqtt(const char *id, upstream_fn f, const char *n)
    : id(id)
{
    mutex = xSemaphoreCreateMutex();
    name = (n ? n : id);
    if (!f)
        up_fn = [](String &t, String &p) { log_msg(String(client_id) + " received: " + t + ":" + p); };
    else
        up_fn = f;
    V(mutex);

    mqtt_connections.push_back(this);
    log_msg(String(name) + " mqtt client created.");
}

void myMqtt::loop(void)
{
    //    if (!connected())
    //        reconnect();
    P(mutex);
    client->loop();
    V(mutex);
}

static void mqtt_connect_wrapper(void *arg)
{
    myMqtt *p = static_cast<myMqtt *>(arg);
    log_msg(String(p->get_name()) + "mqtt connection task launched...");
    while (!p->connected())
    {
        p->reconnect_body();
    }
    log_msg(String(p->get_name()) + "mqtt connection task wating to be killed.");
    while (1)
    {
        delay(1000); /* wait to be killed */
        p->cleanup();
    }
}

bool myMqtt::connected(void)
{
    bool r;
    P(mutex);
    r = client->connected();
    conn_stat = (r ? CONN : NO_CONN);
    V(mutex);
    return r;
}

void myMqtt::reconnect(void)
{
    //printf("reconnect requested...%p, connstat = %d, th = %p\n", this, conn_stat, th);
    P(mutex);
    if ((conn_stat != CONN) &&
        (th == nullptr))
    {
        int prio = uxTaskPriorityGet(nullptr);
        printf("reconnect requested...creating task for %p, priority = %d\n", this, prio);
        conn_stat = RECONN; /* safe as wrapped by mutex */
        xTaskCreate(mqtt_connect_wrapper, "mqtt-conn", 4000, this, prio /*configMAX_PRIORITIES - 1*/, &th);
    }
    V(mutex);
}

void myMqtt::cleanup(void)
{
    P(mutex);
    if (conn_stat == CONN && th)
    {
        log_msg(String(name) + " killing mqtt task... ");
        TaskHandle_t t = th;
        th = nullptr;
        V(mutex);
        vTaskDelete(t);
        return;
    }
    V(mutex);
    printf("cleanup done.\n");
}

void myMqtt::reconnect_body(void)
{

    // Loop until we're reconnected
    if (client->connected())
    {
        set_conn_stat(CONN);
        log_msg("reconnect_body, client is connected - shouldn't happen here");
    }
    if ((millis() - last) < 2500)
    {
        delay(500);
        return;
    }
    if (connection_wd == 0)
        connection_wd = millis();
    last = millis();
    reconnects++;
    //log_msg("fcc not connected, attempting MQTT connection..." + String(reconnects));
    ui->log_event(("mqtt connecting..." + String(reconnects)).c_str());

    // Attempt to connect
    if (connect())
    {
        reconnects = 0;
        connection_wd = 0;
        log_msg("fcc connected.");
        P(mutex);
        client->subscribe("fcce/#", 0);
        V(mutex);
        mqtt_publish("/config", "Formicula Control Center - aloha...", this);
        set_conn_stat(CONN);
    }
    else
    {
        unsigned long t1 = (millis() - connection_wd) / 1000;
        log_msg(String("Connection lost for: ") + String(t1) + "s...");
        if (t1 > 300)
        {
            log_msg("mqtt reconnections failed for 5min... rebooting", myLogger::LOG_MSG, true);
            delay(250);
            ESP.restart();
        }
    }
}

void myMqtt::publish(String &t, String &p, int qos)
{
    P(mutex);
    if (!client->connected() ||
        !client->publish(client_id + t, p, false, qos))
    {
        log_msg(String(name) + " mqtt failed, rc=" + String(client->lastError()) + ", discarding: " + t + ":" + p);
    }
    V(mutex);
}

void myMqtt::register_callback(upstream_fn fn)
{
    P(mutex);
    client->onMessage(fn);
    V(mutex);
}

myMqttSec::myMqttSec(const char *id, const char *server, upstream_fn fn, const char *n, int port, const char *user, const char *pw)
    : myMqtt(id, fn, n), user(user), pw(pw)
{
    client = new MQTTClient{256};
    client->begin(server, port, net);
    client->onMessage(up_fn);
    delay(50);
}

bool myMqttSec::connect(void)
{
    P(mutex);
    if (!client->connected() &&
        !client->connect(id, user, pw))
    {
        log_msg(String(name) + " mqtt connect failed, rc=" + String(client->lastError()));
        V(mutex);
        return false;
    }
    V(mutex);
    return true;
}

myMqttLocal::myMqttLocal(const char *id, const char *server, upstream_fn fn, const char *n, int port)
    : myMqtt(id, fn, n)
{
    client = new MQTTClient{256}; /* default msg size, 128 */
    IPAddress sv = MDNS.queryHost(server);
    if (sv == INADDR_NONE)
    {
        log_msg(String(name) + " mqtt broker '" + server + "' not found.");
        return; // XXX throw exception here
    }

    client->begin(sv, port, net);
    client->onMessage(up_fn);
    // XXX callback
    delay(50);
}

bool myMqttLocal::connect(void)
{
    P(mqtt_mutex);
    if (!client->connected() &&
        !client->connect(id))
    {
        log_msg(String(name) + " mqtt connect failed, rc=" + String(client->lastError()));
        V(mqtt_mutex);
        return false;
    }
    V(mqtt_mutex);
    return true;
}

static void fcce_upstream(String &t, String &payload)
{
    //log_msg("fcc mqtt cb: " + t + ":" + payload);
    String topic = t.substring(t.indexOf('/'));
    // check if config things arriving
    if (topic.startsWith("/config"))
    {
        ui->update_config(payload);
    }

    bool fcce_alive = false;

    if (payload.startsWith("<ERR>"))
    {
        ui->ui_P();
        ui->log_event((topic + payload).c_str(), myLogger::LOG_SENSOR);
        //ui->log_event(payload.c_str(), myLogger::LOG_SENSOR);
        ui->ui_V();
        //log_msg(topic + payload);
        fcce_alive = true;
        goto out;
    }

    /* check circuit controlled via mqtt */
    std::for_each(circuits.begin(), circuits.end(),
                  [&](genCircuit *c) {
                      if ((String("/") + c->get_name()) == topic)
                      {
                          ui->ui_P();
                          if (!strchr(payload.c_str(), '1'))
                              if (!strchr(payload.c_str(), '0'))
                                  log_msg("Circuit " + c->get_name() + " unknown request: " + payload);
                              else
                                  c->io_set(LOW, true, true);
                          else
                              c->io_set(HIGH, true, true);
                          ui->ui_V();
                      }
                  });

    /* check sensor update, find the corresponding sensor by name & update */
    std::for_each(remote_sensors.begin(), remote_sensors.end(),
                  [&](genSensor *sensor) {
                      //log_msg(String("checking '") + sens_name + "' against '" + sensor->get_name() + '\'');
                      if (*sensor == topic)
                      {
                          ui->ui_P();
                          sensor->update_data(strtof(payload.c_str(), NULL));
                          ui->ui_V();
                          fcce_alive = true;
                      }
                  });
out:
    if (fcce_alive)
        ui->update_config(String("/sensor-alive"));
}
