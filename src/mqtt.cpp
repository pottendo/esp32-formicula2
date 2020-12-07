#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <ESPmDNS.h>

#include "ui.h"

static const String mqtt_server{"fcce"};
static IPAddress mqtt_ip;
static uiElements *ui;
static AsyncMqttClient mqttClient;

void onMqttConnect(bool sessionPresent)
{
    log_msg("Connected to MQTT - " + String(sessionPresent));
    uint16_t packetIdSub = mqttClient.subscribe("#", 0);
    mqttClient.publish("/fcce/config", 0, true, "Formicula Control Center - aloha...");
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

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
    char *buf = (char *)malloc(len * sizeof(char) + 1);
    snprintf(buf, len + 1, "%s", payload);
    log_msg(String("Sensor ") + topic + " delivered: " + buf + "(" + String(len) + ")");
    free(buf);
}

void onMqttPublish(uint16_t packetId)
{
    log_msg("Publish acknowledged for ID " + String(packetId));
}

void setup_mqtt(uiElements *u)
{

    mqtt_ip = MDNS.queryHost(mqtt_server.c_str());

    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onSubscribe(onMqttSubscribe);
    mqttClient.onUnsubscribe(onMqttUnsubscribe);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.onPublish(onMqttPublish);
    mqttClient.setServer(mqtt_ip, 1883);

    mqttClient.connect();
    delay(20);
}

void loop_mqtt()
{
    static long value = 0;
    static char msg[50];
    static unsigned long lastMsg = 0;
    unsigned long now = millis();

    if (now - lastMsg > 2000)
    {
        if (!mqttClient.connected())
        {
            printf("MQTT not connected, retrying... %ld\n", now - lastMsg);
            mqttClient.connect();
        }
        else
        {
            ++value;
            snprintf(msg, 50, "Hello world from formicula control centre #%ld", value);
            Serial.print("Publish message: ");
            Serial.println(msg);
            mqttClient.publish("/fcce/config", 0, false, msg);
        }
        lastMsg = now;
    }
}
