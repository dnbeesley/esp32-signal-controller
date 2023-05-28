#include <Arduino.h>
#include <EspMQTTClient.h>
#include "config.h"

#define CHECK_INTERVAL 100000
#define DETECTOR_LENGTH 2
#define IR_TRANSMITTER D6
#define PRESCALER (F_CPU / 1000000UL)
#define PULSE_FREQUENCY 38000
#define PULSE_INTERVAL 5000
#define PULSE_LENGTH 1000
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

unsigned long irDetected[] = {ULONG_MAX, ULONG_MAX};
unsigned long irSent[] = {0, 0};
hw_timer_t *pulseTimer;

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

    pulseTimer = timerBegin(1, PRESCALER, true);
    timerAttachInterrupt(pulseTimer, onStartPulse, true);
    timerAlarmWrite(pulseTimer, PULSE_INTERVAL, true);
    timerAlarmEnable(pulseTimer);
    log_d("Setup complete");
}

void loop()
{
    String output;
    size_t i;
    client.loop();
    unsigned long start = micros() - CHECK_INTERVAL;

    for (i = 0; i < DETECTOR_LENGTH; i++)
    {
        // Send event if pin has been high since start of the interval and not event yet sent in inteval
        if (irDetected[i] < start && irSent[i] < start)
        {
            log_d("IR beam on input: %d was last detected at: %d and a message sent at: %d", i, irDetected[i], irSent[i]);
            irSent[i] = micros();
            output = String(i);
            log_d("Sending new message");
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
    log_d("Received signal state: %s", payload.c_str());
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
        case '0':
            digitalWrite(redPins[i], HIGH);
            digitalWrite(greenPins[i], HIGH);
        }
    }
}

void onStartPulse()
{
    tone(IR_TRANSMITTER, PULSE_FREQUENCY, PULSE_LENGTH);
}
