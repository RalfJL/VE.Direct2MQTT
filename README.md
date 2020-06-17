
Victron is building wonderful devices for Solar systems.
Some of the devices have a VE.Direct interface and Victron disclosed the protocol.

- See also: https://www.victronenergy.com/live/vedirect_protocol:faq

I do own 2 MQTT75/15 Solar charger and all that I was missing was a MQTT interface for monitoring.

So here it is

# VE.Direct2MQTT

VE.Direct2MQTT is using an ESP32 developers board and the Arduino IDE to send all ASCII data coming from a VE.Direct device to a MQTT server.

With the help of the MQTT server you can integrate the monitoring data to virtually any Home Automation System.

## Feature
- Listen to VE.Direct messages and publishes a block (consisting of several key-value pairs) to a MQTT broker<br>Every key from the device will be appended to the MQTT_PREFIX and build a topic. e.g. MQTT_PREFIX="/MPPT"; Topic /MPPT/V will contain the Battery Voltage<br> so please see the VE.Direct protocol for the meaning o topics
- SSL enabled<br>If you are sending messages over the internet or using User/Passwords over the Internet you have to use SSL.<br>If you are using a locally protected network you can disable the Usage of SSL in the config file
- Have several SSID's to connect to, in case one or the other is not reachable from your position
- Have sveral MQTT Servers in case one is down.<br> The system will only be bound to one MQTT server at a time