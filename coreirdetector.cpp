#include "coreirdetector.h"

#include <ArduinoJson.h>
#include <EspMQTTClient.h>
#include <string.h>

#define ANALOG_MAX 4095
#define CHECK_INTERVAL 100000 // ms
#define DETECTOR_PINS \
    {                 \
        A0, A1        \
    }
#define DETECTOR_THREASHOLD 3276
#define FIELD_NAME_LENGTH 32
#define IR_TRANSMITTER D2
#define PUBLISH_INTERVAL 10000000 // us
#define PULSE_FREQUENCY 38000     // Hz
#define PULSE_INTERVAL 5000       // us
#define PULSE_LENGTH 1000         // us

typedef struct
{
    String wifiSsid;
    String wifiPassword;
    String mqttServerIp;
    String mqttUsername;
    String mqttPassword;
} Configuration;

typedef struct
{
    MessageReceivedCallback messageCallback;
    uint8_t qos;
    String topic;
} Subscription;

typedef struct
{
    std::vector<uint8_t> inputPins;
    unsigned long irTransmitted;
    std::vector<bool> irValue;
    String message;
    unsigned long messagePublished;
    String publishTopic;
    std::vector<Subscription> subscriptions;
} IrDetectorState;

Configuration config;
EspMQTTClient client;
IrDetectorState state;

/**
 * @brief Returns true if any of the values are true
 *
 * @param values
 */
bool any(std::vector<bool> values)
{
    size_t i;
    for (i = 0; i < values.size(); i++)
    {
        if (values[i])
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Publishes whether an IR signal has been detected on either of the inputs since the beginnig of the interval
 *
 */
void publishState()
{
    char fieldName[FIELD_NAME_LENGTH];
    size_t i;
    unsigned long intervalStart = micros() - CHECK_INTERVAL;
    DynamicJsonDocument doc(1024);

    for (i = 0; i < state.irValue.size(); i++)
    {
        snprintf(fieldName, FIELD_NAME_LENGTH, "detector%d", i);
        doc[fieldName] = (bool)state.irValue[i];
    }

    String output;
    serializeJson(doc, output);
    if (state.message != output || state.messagePublished < micros() - PUBLISH_INTERVAL)
    {
        state.messagePublished = micros();
        if (client.publish(state.publishTopic, output))
        {
            state.message = output;
        }
    }
}

/**
 * @brief Starts a pulse of IR signal
 *
 */
void startPulse()
{
    size_t i;
    state.irTransmitted = micros();
    unsigned long end = state.irTransmitted + PULSE_LENGTH;
    uint16_t value;
    std::vector<bool> values(state.inputPins.size());
    for (i = 0; i < values.size(); i++)
    {
        values[i] = true;
    }

    tone(IR_TRANSMITTER, PULSE_FREQUENCY);
    do
    {
        for (i = 0; i < state.inputPins.size(); i++)
        {
            if (!values[i])
            {
                continue;
            }

            value = analogRead(state.inputPins[i]);
            values[i] = value > DETECTOR_THREASHOLD;
        }
    } while (end > micros() && any(values));

    noTone(IR_TRANSMITTER);
    digitalWrite(IR_TRANSMITTER, LOW);
    state.irValue = values;
}

/**
 * @brief Add a subcrition to a topic with a callback.
 *
 * @param topic The name of the topic to which to subscribe
 * @param messageCallback The callback to call when a message is received
 * @param qos The quality of service
 */
void addSubscription(String &topic, std::function<void(const String &message)> messageCallback, uint8_t qos)
{
    log_d("Adding subscription to topic: %s", topic.c_str());
    state.subscriptions.push_back(Subscription{
        messageCallback,
        qos,
        topic});
}

/**
 * @brief Connects to the MQTT server
 *
 * @param wifiSsid
 * @param wifiPassword
 * @param mqttServerIp
 * @param mqttUsername
 * @param mqttPassword
 * @param mqttServerPort
 */
void connect(
    const String &wifiSsid,
    const String &wifiPassword,
    const String &mqttServerIp,
    const String &mqttUsername,
    const String &mqttPassword,
    const short mqttServerPort)
{
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
    client.enableDebuggingMessages();
#endif
    config.wifiSsid = wifiSsid;
    config.wifiPassword = wifiPassword;
    config.mqttServerIp = mqttServerIp;
    config.mqttUsername = mqttUsername;
    config.mqttPassword = mqttPassword;

    log_d("Connecting to WIFI SSID: %s", config.wifiSsid.c_str());
    client.setWifiCredentials(config.wifiSsid.c_str(), config.wifiPassword.c_str());

    log_d("Client name: %s", config.mqttUsername.c_str());
    client.setMqttClientName(config.mqttUsername.c_str());

    log_d("Connecting to the MQTT server: %s on port: %d", config.mqttServerIp.c_str(), mqttServerPort);
    client.setMqttServer(config.mqttServerIp.c_str(), config.mqttUsername.c_str(), config.mqttPassword.c_str(), mqttServerPort);
    log_i("Connected to the MQTT server");
}

/**
 * @brief Tasks to be performed in the main program loop
 *
 */
void detectorLoop()
{
    size_t i;
    uint16_t value;
    if (state.publishTopic.length() > 0)
    {
        unsigned long now = micros();
        if (client.isConnected() && state.messagePublished < now - CHECK_INTERVAL)
        {
            publishState();
        }
        else if (state.irTransmitted < now - PULSE_INTERVAL)
        {
            startPulse();
        }
    }

    client.loop();
}

/**
 * @brief Callback function used by EspMQTTClient.h
 *
 */
void onConnectionEstablished()
{
    for (int i = 0; i < state.subscriptions.size(); i++)
    {
        client.subscribe(
            state.subscriptions[i].topic,
            state.subscriptions[i].messageCallback,
            state.subscriptions[i].qos);
    }
}

/**
 * @brief Starts the IR detection process
 *
 * @param publishTopic To topic to which to publish
 */
void startIrDetection(String &publishTopic)
{
    pinMode(IR_TRANSMITTER, OUTPUT);

    state.inputPins = DETECTOR_PINS;
    state.irValue = std::vector<bool>(state.inputPins.size());
    state.publishTopic = publishTopic;
    for (int i = 0; i < state.inputPins.size(); i++)
    {
        state.irValue[i] = false;
        pinMode(state.inputPins[i], INPUT);
    }
}

/**
 * @brief Stops the IR detection process
 *
 */
void stopDetection()
{
    state.publishTopic = "";
}
