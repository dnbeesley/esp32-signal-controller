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
| 0        | D3      | D4        |
| 1        | D5      | D6        |
| 2        | D7      | D8        |
| 3        | D9      | D10       |

The content of the MQTT messages should be a series of ASCII Rs, Ys, Gs and 0s, to indicate the state of each LED.

For example:

- "YGRR" sets the first LED to yellow, the next to green, and the last two to red.
- "G0RR" sets the first LED to green, the next to off, and the last two to red.

## IR beam detection

The IR detectors used detect pulses of 780nm infra-red modulated at 38KHz. Upon detection the output of the IR received is set to low for a brief time. The IR receives should connect to the ESP32's analogue inputs via a low pass filter. The ESP32 detects if the beam has been broken by checking if the the anlog input goes below ~80% of the maximum during the time when the IR pulse is generated.

The anode of a IR LED and ~ 100&#x03A9; resistor needs to be connected to D2. The output pins of the IR detectors need to be connected to D4 and D5.

### Behaviour

- Every 5ms, a 1ms second pulse of 38KHz is produced from the IR LED.
- Every 100ms a message is published to the MQTT topic indicating whether the output of the IR received has remained above the threashold in that time. If a signal has been detected on D0, but not D1 this will produce a message in the following format:

```JSON
{
    "detector0": false,
    "detector1": true
}
```
