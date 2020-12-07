#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>

#include "ui.h"

static String mqtt_server{"fcce"};

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
long int value = 0;

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    // Switch on the LED if an 1 was received as first character
    if ((char)payload[0] == '1')
    {
    }
    else
    {
    }
}

void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        printf("fcce IP Addr = %s\n", MDNS.queryHost(mqtt_server.c_str()).toString().c_str());

        // Create a random client ID
        String clientId = "fcc-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (client.connect(clientId.c_str()))
        {
            Serial.println("connected");
            // Once connected, publish an announcement...
            client.publish("/fcce/config", "Formicula Control Center connected.");
            // ... and resubscribe
            client.subscribe("/fcce/");
        }
        else
        {
            log_msg(String("failed, rc=") + client.state() + " ... try again in 2 seconds");
            delay(200);
        }
    }
}

void setup_mqtt()
{
    log_msg("Connecting MQTT broker " + mqtt_server);
    client.setServer(mqtt_server.c_str(), 1883);
    client.setCallback(callback);
    reconnect();
    printf("fcce IP Addr = %s\n", MDNS.queryHost(mqtt_server.c_str()).toString().c_str());
}

void loop_mqtt()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();

    unsigned long now = millis();
    if (now - lastMsg > 2000)
    {
        lastMsg = now;
        ++value;
        snprintf(msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
        Serial.print("Publish message: ");
        Serial.println(msg);
        client.publish("outTopic", msg);
    }
}
