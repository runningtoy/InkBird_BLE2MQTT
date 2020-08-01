# iBBQ-Gateway 

an ESP32 Based iBBQ (Inkbird) BLE to MQTT Gateway

## Requirements

## Usage

After flashing the device you should connect to your WiFi, it should connect to your iBBQ Device and starts to publish data
rename the `credentials.example.h` to `credentials.h` and edit it to your needs

## Features

* Automatically connects to iBBQ Bluetooth BBQ thermometers (tested with IBT-6X)
* Adapts amount of channels to connected thermometer
* Should work with most ESP32 boards available
* Should work with iBBQ based Bluetooth BBQ thermometers with up to 8 channels
* Publish temperature (Celsius) as MQTT Topics
* Publish battery level as MQTT Topic
* Battery status according to (https://github.com/sworisbreathing/go-ibbq/issues/2#issuecomment-650725433)[https://github.com/sworisbreathing/go-ibbq/issues/2#issuecomment-650725433]

## ToDO

## Protocol description
(https://gist.github.com/uucidl/b9c60b6d36d8080d085a8e3310621d64)
