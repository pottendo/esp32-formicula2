#include <Arduino.h>
#include <MQTT.h>
#include <list>
#include "ui.h"
#include "circuits.h"

// myqtthub credentials
#define EXTMQTT
#ifdef EXTMQTT
#include <WiFiClientSecure.h>
static const char *mqtt_server = "node02.myqtthub.com";
static int mqtt_port = 8883;
static const char *clientID = "fcc";
static const char *user = "fcc-user";
static const char *pw = "foRmicula666";
WiFiClientSecure net;
#else
#include <WiFi.h>
static const char *mqtt_server = "pottendo-pi30-phono";
static int mqtt_port = 1883;
static const char *clientID = "fcc";
WiFiClient net;
#endif

static MQTTClient client;

static uiElements *ui;
static std::list<genSensor *> remote_sensors;
static std::list<genCircuit *> circuits;

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

void callback(String &t, String &payload)
{
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
        ui->log_event(topic.c_str());
        ui->log_event(payload.c_str());
        ui->ui_V();
        log_msg(topic + payload);
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

void reconnect()
{
    static int reconnects = 0;
    static unsigned long last = 0;
    static unsigned long connection_wd = 0;

    // Loop until we're reconnected
    if ((!client.connected()) &&
        ((millis() - last) > 2500))
    {
        if (connection_wd == 0)
            connection_wd = millis();
        last = millis();
        reconnects++;
        log_msg("fcc not connected, attempting MQTT connection..." + String(reconnects));
        ui->log_event("MQTT not connected, retrying...");

        // Attempt to connect
#ifdef EXTMQTT
        //net.setInsecure();
        if (client.connect(clientID, user, pw))
#else
        if (client.connect(clientID))
#endif
        {
            client.subscribe("fcce/#", 0);
            mqtt_publish("/config", "Formicula Control Center - aloha...");
            reconnects = 0;
            connection_wd = 0;
            log_msg("fcc connected.");
        }
        else
        {
            unsigned long t1 = (millis() - connection_wd) / 1000;
            log_msg(String("Connection lost for: ") + String(t1) + "s...");
            if (t1 > 300)
            {
                log_msg("mqtt reconnections failed for 5min... rebooting");
                ESP.restart();
            }
            log_msg("mqtt connect failed, rc=" + String(client.lastError()) + "... retrying.");
        }
    }
}

bool mqtt_reset(void)
{
    log_msg("mqtt reset requested: " + String(client.lastError()));
    return true;
}

void mqtt_publish(String topic, String msg)
{
    if (!client.connected() &&
        (mqtt_reset() == false))
    {
        log_msg("mqtt client not connected...");
        return;
    }
    //log_msg("fcc publish: " + topic + " - " + msg);
    client.publish((clientID + topic).c_str(), msg.c_str());
}

void setup_mqtt(uiElements *u)
{
    ui = u;
    client.begin(mqtt_server, mqtt_port, net);
    client.onMessage(callback);
    delay(50);
}

void loop_mqtt()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();
}
