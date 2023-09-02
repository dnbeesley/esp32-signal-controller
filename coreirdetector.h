#ifndef CORE_IR_DETECTOR_H_
#define CORE_IR_DETECTOR_H_

#include "coreirdetector.h"

#include <Arduino.h>

#include <vector>

/**
 * @brief Add a subcrition to a topic with a callback.
 *
 * @param topic The name of the topic to which to subscribe
 * @param messageCallback The callback to call when a message is received
 * @param qos The quality of service
 */
void addSubscription(String &topic, std::function<void(const String &message)> messageCallback, uint8_t qos = (uint8_t)0U);

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
    const short mqttServerPort);

/**
 * @brief Calls the loop() function of the MQTT client
 *
 */
void detectorLoop();

/**
 * @brief Starts the IR detection process
 *
 * @param publishTopic To topic to which to publish
 */
void startIrDetection(String &publishTopic);

/**
 * @brief Stops the IR detection process
 *
 */
void stopDetection();

#endif
