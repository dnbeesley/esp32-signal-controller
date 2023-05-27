#include <Arduino.h>
#include <EspMQTTClient.h>
#include "config.h"

#define DETECTOR_LENGTH 2
#define IR_TRANSMITTER D6
#define INTERVAL 100000
#define OUTPUT_SIZE 64
#define SIGNAL_LENGTH 4

const uint8_t detectorPins[DETECTOR_LENGTH] = {D4, D5};
const uint8_t redPins[SIGNAL_LENGTH] = {D0, D2, D7, D9};
const uint8_t greenPins[SIGNAL_LENGTH] = {D1, D3, D8, D10};

EspMQTTClient client(
    WIFI_SSID,
    WIFI_PASSWORD,
    AMQP_IP,
    AMQP_USERNAME,
    AMQP_PASSWORD,
    DEVICE_NAME,
    AMQP_PORT);

std::vector<unsigned long> irDetected = {0, 0};
std::vector<unsigned long> irSent = {0, 0};

void setup()
{
    size_t i;
    for (i = 0; i < SIGNAL_LENGTH; i++)
    {
        pinMode(redPins[i], OUTPUT);
        pinMode(greenPins[i], OUTPUT);
    }

    for (i = 0; i < DETECTOR_LENGTH; i++)
    {
        pinMode(detectorPins[i], INPUT);
        attachInterrupt(digitalPinToInterrupt(detectorPins[i]), onIrDetect, FALLING);
    }
}

void loop()
{
    char output[OUTPUT_SIZE];
    size_t i;
    client.loop();
    unsigned long start = micros() - INTERVAL;
    if (*std::max_element(irDetected.begin(), irDetected.end()) < start)
    {
        // Start 38KHz output if neither input has gone low since the start of the interval
        tone(IR_TRANSMITTER, 38000);
    }

    if (*std::min_element(irDetected.begin(), irDetected.end()) >= start)
    {
        // Stop 38KHz output if both inputs have gone low since the start of the interval
        noTone(IR_TRANSMITTER);
    }

    for (i = 0; i < DETECTOR_LENGTH; i++)
    {
        // Send event if pin has been high since start of the interval
        if (irDetected[i] < start && irSent[i] < start)
        {
            irSent[i] = micros();
            snprintf(output, OUTPUT_SIZE, "%d", i);
            client.publish(AMQP_PUBLISH_TOPIC, output);
        }
    }
}

void onConnectionEstablished()
{
    client.subscribe(AMQP_SUBSCRIBE_TOPIC, onSignalStateReceive);
}

void onIrDetect()
{
    size_t i;
    for (i = 0; i < DETECTOR_LENGTH; i++)
    {
        if (!digitalRead(detectorPins[i]))
        {
            irDetected[i] = micros();
        }
    }
}

void onSignalStateReceive(const String &payload)
{
    size_t i;
    for (i = 0; i < payload.length() && i < SIGNAL_LENGTH; i++)
    {
        switch (payload.charAt(i))
        {
        case 'R':
        case 'r':
            digitalWrite(redPins[i], LOW);
            digitalWrite(greenPins[i], HIGH);
        case 'Y':
        case 'y':
            digitalWrite(redPins[i], LOW);
            digitalWrite(greenPins[i], LOW);
        case 'G':
        case 'g':
            digitalWrite(redPins[i], HIGH);
            digitalWrite(greenPins[i], LOW);
        }
    }
}
