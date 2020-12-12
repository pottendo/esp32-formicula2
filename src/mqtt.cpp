#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>
#include <list>

#include "ui.h"
#include "circuits.h"

//static const String mqtt_server{"pottendo-pi30-phono"};
static const String mqtt_server{"fcce"};
static IPAddress server;
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

void callback(char *t, byte *payload, unsigned int len)
{
    if (len == 0)
    {
        log_msg("MQTT invalid message received... ignoring");
        return;
    }
    char *buf = (char *)malloc(len * sizeof(char) + 1);
    snprintf(buf, len + 1, "%s", payload);
    //log_msg(String("Sensor ") + t + " delivered: " + buf + "(" + String(len) + ")");
    const String topic{strchr(t + 1, '/')};

    // check if config things arriving
    if (topic.startsWith("/config"))
    {
        ui->update_config(String(buf));
    }

    bool fcce_alive = false;

    /* check circuit controlled via mqtt */
    std::for_each(circuits.begin(), circuits.end(),
                  [&](genCircuit *c) {
                      if ((String("/") + c->get_name()) == topic)
                      {
                          ui->ui_P();
                          if (!strchr(buf, '1'))
                              if (!strchr(buf, '0'))
                                  log_msg("Circuit " + c->get_name() + " unknown request: " + buf);
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
                          sensor->update_data(strtof(buf, NULL));
                          ui->ui_V();
                          fcce_alive = true;
                      }
                  });
    free(buf);
    if (fcce_alive)
        ui->update_config(String("/sensor-alive"));
}

WiFiClient wClient;
PubSubClient *client;

void reconnect()
{
    // Loop until we're reconnected
    while (!client->connected())
    {
        log_msg("Attempting MQTT connection...");
        // Attempt to connect
        if (client->connect("fccmqttclient"))
        {
            log_msg("connected");
            client->subscribe("#", 0);
            client->publish("fcce/config", "Formicula Control Center - aloha...");
        }
        else
        {
            log_msg("mqtt connect failed, rc=" + String(client->state()) + "... retrying.");
            delay(2500);
        }
    }
}

bool mqtt_reset(void)
{
    log_msg("mqtt reset requested: " + String(client->state()));
    return true;
}

void mqtt_publish(String topic, String msg)
{
    if (!client->connected() &&
        (mqtt_reset() == false))
        return;
    client->publish(topic.c_str(), msg.c_str());
}

void setup_mqtt(uiElements *u)
{
    ui = u;
    client = new PubSubClient{wClient};
    server = MDNS.queryHost(mqtt_server.c_str());  
    client->setServer(server, 1883);
    client->setCallback(callback);

    // Allow the hardware to sort itself out
    delay(500);
}

void loop_mqtt_dummy()
{
    static long value = 0;
    static char msg[50];
    static unsigned long lastMsg = millis();

    if (!WiFi.isConnected()){
        log_msg("Wifi not connected ... strange");
        return;
    }

    unsigned long now = millis();
    if (now - lastMsg > 2000)
    {
        if (!client->connected())
        {
            mqtt_reset();
        }
        else
        {
            ++value;
            snprintf(msg, 50, "Hello world from formicula control centre #%ld", value);
            Serial.print("Publish message: ");
            Serial.println(msg);
            client->publish("fcce/config", msg);
            const char *c;
            if (value % 2)
                c = "1";
            else
                c = "0";

            client->publish("fcce/Infrarot", c);
        }
        lastMsg = now;
    }
}

void loop_mqtt()
{
    if (!client->connected())
    {
        reconnect();
    }
    client->loop();
    loop_mqtt_dummy();
}

#if 0

#include <AsyncMqttClient.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <list>

#include "ui.h"
#include "io.h"
#include "circuits.h"


//static const String mqtt_server{"fcce"};
static const String mqtt_server{"pottend-pi30-phono"};
static IPAddress mqtt_ip;
static uiElements *ui;
static AsyncMqttClient *mqttClient;

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

/* mqtt async client specfics */
void onMqttConnect(bool sessionPresent)
{
    log_msg("Connected to MQTT - " + String(sessionPresent));
    mqttClient->subscribe("#", 0);
    mqttClient->publish("fcce/config", 0, true, "Formicula Control Center - aloha...");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
    log_msg("Disconnected from MQTT.");
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos)
{
    log_msg("Subscribe packet acknowledged." + String(packetId));
}

void onMqttUnsubscribe(uint16_t packetId)
{
    log_msg("Unsubscribe acknowledged.");
}

void onMqttMessage(char *t, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
    if (len == 0)
    {
        log_msg("MQTT invalid message received... ignoring");
        return;
    }
    char *buf = (char *)malloc(len * sizeof(char) + 1);
    snprintf(buf, len + 1, "%s", payload);
    //log_msg(String("Sensor ") + t + " delivered: " + buf + "(" + String(len) + ")");
    const String topic{strchr(t + 1, '/')};

    // check if config things arriving
    if (topic.startsWith("/config"))
    {
        ui->update_config(String(buf));
    }

    bool fcce_alive = false;

    /* check circuit controlled via mqtt */
    std::for_each(circuits.begin(), circuits.end(),
                  [&](genCircuit *c) {
                      if ((String("/") + c->get_name()) == topic)
                      {
                          ui->ui_P();
                          if (!strchr(buf, '1'))
                              if (!strchr(buf, '0'))
                                  log_msg("Circuit " + c->get_name() + " unknown request: " + buf);
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
                          sensor->update_data(strtof(buf, NULL));
                          ui->ui_V();
                          fcce_alive = true;
                      }
                  });
    free(buf);
    if (fcce_alive)
        ui->update_config(String("/sensor-alive"));
}

void onMqttPublish(uint16_t packetId)
{
    log_msg("Publish acknowledged for ID " + String(packetId));
}

void setup_mqtt(uiElements *u)
{
    ui = u;
    mqtt_ip = MDNS.queryHost(mqtt_server.c_str());
    mqttClient = new AsyncMqttClient;

    mqttClient->onConnect(onMqttConnect);
    mqttClient->onDisconnect(onMqttDisconnect);
    mqttClient->onSubscribe(onMqttSubscribe);
    mqttClient->onUnsubscribe(onMqttUnsubscribe);
    mqttClient->onMessage(onMqttMessage);
    mqttClient->onPublish(onMqttPublish);
    mqttClient->setServer(mqtt_ip, 1883);

    //mqttClient->connect();
    delay(20);
}

bool mqtt_reset(void)
{
    printf("MQTT not connected, connecting... ");
    delete mqttClient;
    setup_mqtt(ui);
    mqttClient->connect();
    delay(20);
    if (!mqttClient->connected())
    {
        printf("failed.\n");
        return false;
    }
    else
    {
        printf("success.\n");
        return true;
    }
}

void mqtt_publish(String topic, String msg)
{
    if (!mqttClient->connected() &&
        (mqtt_reset() == false))
        return;
    mqttClient->publish(topic.c_str(), 0, 0, msg.c_str());
}

void loop_mqtt()
{
    static long value = 0;
    static char msg[50];
    static unsigned long lastMsg = millis();

    if (!WiFi.isConnected())
        return;

    unsigned long now = millis();
    if (now - lastMsg > 2000)
    {
        if (!mqttClient->connected())
        {
            mqtt_reset();
        }
        else
        {
            ++value;
            snprintf(msg, 50, "Hello world from formicula control centre #%ld", value);
            Serial.print("Publish message: ");
            Serial.println(msg);
            mqttClient->publish("fcce/config", 0, false, msg);
            const char *c;
            if (value % 2)
                c = "1";
            else
                c = "0";

            mqttClient->publish("fcce/Infrarot", 0, false, c);
        }
        lastMsg = now;
    }
}
#endif