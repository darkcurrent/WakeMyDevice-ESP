# WakeMyDevice-ESP

WakeMyDevice-ESP is a Wake-on-LAN (WoL) tool designed for ESP8266 microcontrollers. It enables users to remotely wake up devices within the same local network via a web interface or MQTT commands. This repository details setting up and running the WoL program on an ESP8266, as well as configuring MQTT through HiveMQ for remote wake-up capabilities.

This repository is an enhanced version of the previous debian based [WakeMyDevice](https://github.com/darkcurrent/WakeMyDevice) application for ESP8266 and ESP32 microcontrollers. 

## Features

- Wake up devices from a web interface hosted on the ESP8266.
- Wake up devices by publishing to an MQTT topic.
- Manage a list of devices (add/delete) through the web interface.
- Modern, responsive design using Bootstrap for the web interface.

## Prerequisites

Before getting started, make sure you have:

- An ESP8266 microcontroller flashed with NodeMCU firmware.
- A HiveMQ account for MQTT capabilities.
- A device connected to your local network that supports Wake-on-LAN.

## Setting Up HiveMQ

[HiveMQ](https://www.hivemq.com/) offers MQTT broker services that can be used with the WakeMyDevice-ESP program.

1. **Create a HiveMQ Account:**
   - Visit [HiveMQ Cloud](https://console.hivemq.cloud/user/signup) and sign up for a new account.
   - Once signed up, log in to your HiveMQ Cloud dashboard.

2. **Create a New MQTT Broker:**
   - Go to the 'Clusters' section and create a new cluster.
   - After the cluster is created, note down the `Host` and `Port` provided. You will need these for the ESP8266 MQTT client configuration.

3. **Secure Your MQTT Broker:**
   - It's recommended to set up a unique username and password for your MQTT broker.
   - Under the 'Access Management' section, create new credentials that your ESP8266 will use to connect.

## Hardware Setup

1. Connect your ESP8266 to your computer using a USB cable.
2. Ensure that your ESP8266 can be programmed with the Arduino IDE or your preferred development tool.

## Software Setup

1. **Arduino IDE Configuration:**
   - Install the necessary libraries mentioned in the code (WiFiManager, PubSubClient, etc.).
   - Configure the IDE with the correct board settings for the ESP8266.

2. **Flash the ESP8266:**
   - Open the provided sketch in the Arduino IDE.
   - Fill in your WiFi credentials, HiveMQ broker details, and any other configuration required in the sketch.
   - Upload the sketch to your ESP8266.

## Usage

1. **Web Interface:**
   - Once the ESP8266 is running, connect to the `ESP8266WakeMyDevice` WiFi network that it broadcasts.
   - Use the web interface to configure your local network connection.
   - After connecting to your local network, visit the ESP8266's IP address in a web browser to manage and wake devices.

2. **MQTT:**
   - Publish the MAC address of the device you wish to wake to the `wakeup_list` MQTT topic.
   - Ensure the published MAC address follows one of the accepted formats: `XX-XX-XX-XX-XX-XX`, `XX:XX:XX:XX:XX:XX`, or `XXXXXXXXXXXX`.

## Troubleshooting

- If devices do not wake up, ensure that they are configured correctly for Wake-on-LAN in their BIOS/network settings.
- Verify that the MAC address formats are correct and that the ESP8266 is correctly connected to the local network.
- Check the MQTT broker details if remote wake-up is not functioning.

## Contributing

Contributions to the `WakeMyDevice-ESP` project are welcome. Please feel free to fork the repository, make your changes, and create a pull request.