# ESP32 Signal Controller

The program is designed to run on a Seeed XIAO ESP32C3 to control model railway signalling lights based on events published to an AMQP topic. It also controls an IR beam breaker circuit, which publishes events to an AMQP topic when the beam is broken. This uses the [EspMQTTClient](https://github.com/plapointe6/EspMQTTClient) library.

## Configuration

A header file, config.h, which is not committed to the repo, needs to define the following cstring constants:

- AMQP_IP: The AMQP broker IP address
- AMQP_PASSWORD: The password used to log into the broker
- AMQP_PUBLISH_TOPIC: The topic to which to publish events about beam breaking
- AMQP_SUBSCRIBE_TOPIC: The topic to which to subscribe to receive events about changes to the signal lights
- AMQP_USERNAME: The user name used to log into the broker
- DEVICE_NAME: The device name to use when connecting to the AMQP broker
- WIFI_PASSWORD: The WiFi password
- WIFI_SSID: The WiFi SSID

It also needs an integer constant defined:

- AMQP_PORT: The port number to use

## Signal lights control

The device controls 4 pairs of red-green LEDs with a common anode pin. The device has to sink current to do this. The cathodes have to be connected to the following pins via a ~ 100&#x03A9; resistor.

| LED Pair | Red Pin | Green Pin |
| -------- | ------- | --------- |
| 0        | D0      | D1        |
| 1        | D2      | D3        |
| 2        | D7      | D8        |
| 3        | D9      | D10       |

The content of the AMQP messages should be a series of ASCII Rs, Ys, Gs and 0s, to indicate the state of each LED.

For example:
- "YGRR" sets the first LED to yellow, the next to green, and the last two to red.
- "G0RR" sets the first LED to green, the next to off, and the last two to red.

## IR beam detection

The IR detectors used detect pulses of 780nm infra-red modulated at 38KHz. Upon detection the output of the IR receive is set to low for a brief time. Through a set of interrupts the ESP32 records when a pulse was last recorded.

The anode of a IR LED and ~ 100&#x03A9; resistor needs to be connected to D6. The output pins of the IR detectors need to be connected to D4 and D5.

### Behaviour

- Every 5ms, a 1ms second pulse of 38KHz is produced from the IR LED.
- For each IR detector, if it has been more than 100ms since a signal was received and more than 100ms since an event was published to AMQP for that pin, a new event is published to indicate the beam is broken. However, if a signal has yet to be received at all an event won't be published.
