# ESP32 Signal Controller

The program is designed to run on a Seeed XIAO ESP32C3 to control model railway signalling lights based on events published to an MQTT topic. It also controls an IR beam breaker circuit, which publishes events to an MQTT topic when the beam is broken. This uses the [EspMQTTClient](https://github.com/plapointe6/EspMQTTClient) library.

## Configuration

JSON file, written to the SPIFFS, needs to define the following:

```JSON
{
  "auth": {
    "username": "username",
    "password": "password"
  },
  "detectorTopic": "topic/detector",
  "server": {
    "ipAddress": "192.168.1.2",
    "port": 5672
  },
  "signalTopic": "topic/signal",
  "wifi": {
    "ssid": "Some-WIFI-SSID",
    "password": "wifi-password"
  }
}
```

## Signal lights control

The device controls 4 pairs of red-green LEDs with a common anode pin. The device has to sink current to do this. The cathodes have to be connected to the following pins via a ~ 100&#x03A9; resistor.

| LED Pair | Red Pin | Green Pin |
| -------- | ------- | --------- |
| 0        | D0      | D1        |
| 1        | D3      | D4        |
| 2        | D5      | D6        |
| 3        | D7      | D8        |

The content of the MQTT messages should be a series of ASCII Rs, Ys, Gs and 0s, to indicate the state of each LED.

For example:

- "YGRR" sets the first LED to yellow, the next to green, and the last two to red.
- "G0RR" sets the first LED to green, the next to off, and the last two to red.

## IR beam detection

The program monitories two [Pihut IR Beam Breaker Sensors](https://thepihut.com/products/ir-break-beam-sensor-3mm-leds) connect to D2 and D10. Pull up resistors are enabled by the program. When the program detects a change to the state off either of the inputs the program publishes the state in the followin format. A value of true corrsponds to the input pin being high and a beam being detected. A value of false corresponds to the input pin being low and a beam not bein detected.

```JSON
{
    "detector0": false,
    "detector1": true
}
```
