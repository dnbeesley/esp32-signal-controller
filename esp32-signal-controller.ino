#include "config.h"
#include "coreirdetector.h"

#define SIGNAL_LENGTH 4

const uint8_t redPins[SIGNAL_LENGTH] = {D3, D5, D7, D9};
const uint8_t greenPins[SIGNAL_LENGTH] = {D4, D6, D8, D10};

void setup()
{
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_ERROR
    Serial.begin(115200);
#endif

    size_t i;
    for (i = 0; i < SIGNAL_LENGTH; i++)
    {
        pinMode(redPins[i], OUTPUT);
        digitalWrite(redPins[i], HIGH);
        pinMode(greenPins[i], OUTPUT);
        digitalWrite(greenPins[i], HIGH);
    }

    log_d("Initialising config document");
    DynamicJsonDocument config(2048);
    loadConfig(config);

    String detectorTopic = config["detectorTopic"];
    String ipAdress = config["server"]["ipAddress"];
    String password = config["auth"]["password"];
    String signalTopic = config["signalTopic"];
    String username = config["auth"]["username"];
    String wifipasword = config["wifi"]["password"];
    String wifiSsid = config["wifi"]["ssid"];
    short port = config["server"]["port"];
    addSubscription(signalTopic, onSignalStateReceive);
    connect(wifiSsid, wifipasword, ipAdress, username, password, port);
    startIrDetection(detectorTopic);

    log_d("Setup complete");
}

void loop()
{
    detectorLoop();
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
