#include <Arduino.h>
#include <ArduinoJson.h>
#include <EspMQTTClient.h>
#include "config.h"

#define INPUT_LENGTH 2
#define FIELD_NAME_LENGTH 32
#define SIGNAL_LENGTH 4

EspMQTTClient client;
String *detectorTopic;
String *ipAdress;
String *password;
String *signalTopic;
String *username;
String *wifiPassword;
String *wifiSsid;

const uint8_t beamPins[INPUT_LENGTH] = {D2, D10};
uint8_t beamState[INPUT_LENGTH] = {true, true};

const uint8_t redPins[SIGNAL_LENGTH] = {D0, D3, D5, D7};
const uint8_t greenPins[SIGNAL_LENGTH] = {D1, D4, D6, D8};

void setup()
{
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_ERROR
    Serial.begin(115200);
#endif

    log_d("Initialising config document");
    DynamicJsonDocument config(2048);
    loadConfig(config);

    log_d("Loading parameters");
    detectorTopic = new String((const char *)config["detectorTopic"]);
    ipAdress = new String((const char *)config["server"]["ipAddress"]);
    password = new String((const char *)config["auth"]["password"]);
    signalTopic = new String((const char *)config["signalTopic"]);
    username = new String((const char *)config["auth"]["username"]);
    wifiPassword = new String((const char *)config["wifi"]["password"]);
    wifiSsid = new String((const char *)config["wifi"]["ssid"]);
    short port = config["server"]["port"];

    log_d("Signal topic: %s", signalTopic->c_str());

#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
    client.enableDebuggingMessages();
#endif

    log_d("Connecting to WIFI SSID: %s", wifiSsid->c_str());
    client.setWifiCredentials(wifiSsid->c_str(), wifiPassword->c_str());

    log_d("Client name: %s", username->c_str());
    client.setMqttClientName(username->c_str());

    log_d("Connecting to the MQTT server: %s on port: %d", ipAdress->c_str(), port);
    client.setMqttServer(ipAdress->c_str(), username->c_str(), password->c_str(), port);
    log_i("Connected to the MQTT server");

    log_d("Enabling remote software update");
    client.enableHTTPWebUpdater();
    client.enableOTA();

    log_d("Setting up GPIO");
    size_t i;
    for (i = 0; i < INPUT_LENGTH; i++)
    {
        pinMode(beamPins[i], INPUT_PULLUP);
    }

    for (i = 0; i < SIGNAL_LENGTH; i++)
    {
        pinMode(redPins[i], OUTPUT);
        digitalWrite(redPins[i], HIGH);
        pinMode(greenPins[i], OUTPUT);
        digitalWrite(greenPins[i], HIGH);
    }

    log_d("Setup complete");
}

void loop()
{
    DynamicJsonDocument doc(256);
    String output;
    char fieldName[FIELD_NAME_LENGTH];
    uint8_t value;
    bool changed = false;
    client.loop();

    for (size_t i = 0; i < INPUT_LENGTH; i++)
    {
        value = digitalRead(beamPins[i]);
        if (value != beamState[i])
        {
            changed = true;
            log_i("Beam %d %s", i, value == LOW ? "broken" : "restored");
        }

        beamState[i] = value;
        snprintf(fieldName, FIELD_NAME_LENGTH, "detector%d", i);
        doc[fieldName] = value == HIGH;
    }

    if (changed)
    {
        serializeJson(doc, output);
        client.publish(*detectorTopic, output);
    }
}

void onConnectionEstablished()
{
    log_d("Adding subscription to topic: %s", signalTopic->c_str());
    client.subscribe(*signalTopic, onSignalStateReceive, 0);
}

void onSignalStateReceive(const String &payload)
{
    size_t i;
    log_d("Received signal state: %s", payload.c_str());
    for (i = 0; i < payload.length() && i < SIGNAL_LENGTH; i++)
    {
        switch (payload.charAt(i))
        {
        case 'R':
        case 'r':
            digitalWrite(redPins[i], LOW);
            digitalWrite(greenPins[i], HIGH);
            break;
        case 'Y':
        case 'y':
            digitalWrite(redPins[i], LOW);
            digitalWrite(greenPins[i], LOW);
            break;
        case 'G':
        case 'g':
            digitalWrite(redPins[i], HIGH);
            digitalWrite(greenPins[i], LOW);
            break;
        case '0':
            digitalWrite(redPins[i], HIGH);
            digitalWrite(greenPins[i], HIGH);
        }
    }
}
