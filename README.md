
Victron is building wonderful devices for Solar systems.
Some of the devices have a VE.Direct interface and Victron disclosed the protocol.

- See also: https://www.victronenergy.com/live/vedirect_protocol:faq

I do own 2 MQTT75/15 Solar charger and all that I was missing was a MQTT interface for monitoring.

So here it is

# VE.Direct2MQTT

VE.Direct2MQTT is using an ESP32 developers board and the Arduino IDE to send all ASCII data coming from a VE.Direct device to a MQTT server.

With the help of the MQTT server you can integrate the monitoring data to virtually any Home Automation System.

## Features
- Listen to VE.Direct messages and publish a block (consisting of several key-value pairs) to a MQTT broker<br>Every key from the device will be appended to the MQTT_PREFIX and build a topic. e.g. MQTT_PREFIX="/MPPT"; Topic /MPPT/V will contain the Battery Voltage<br> so please see the VE.Direct protocol for the meaning o topics
- SSL enabled<br>If you are sending messages over the internet or using User/Passwords over the Internet you have to use SSL.<br>If you are using a locally protected network you can disable the Usage of SSL in the config file
- Have several SSID's to connect to, in case one or the other is not reachable from your position
- Have sveral MQTT Servers in case one is down.<br> The system will only be bound to one MQTT server at a time
- Have several OneWire temperature sensors<br>So you can see the temperature of e.g. the MPPT Solracharger or the batteries or your inverter, ...
- Timing parameters can be changed via MQTT<br>E.g. you can set that VE.Direct blocks are only transmitted every 10 seconds. The last received block will be transmitted, all other blocks are lost
- OTA (Over The Air Update)<br>If you have a webserver where you can put binary files on and run php scripts you can use that server to install new VE.Direct2MQTT software<br>Please make sure that you use SSL and User/Password
- One config file to enable/disable features and configure serial port or MQTT Topics


## Limitations
- VE.Direct2MQTT is only listening to messages of the VE.Direct device<br>It understands only the "ASCII" part of the protocol that is only good to receive a set of values. You can't request any special data or change any parameters of the VE.Direct device.<br>This is intentionally because this is monitoring only
- If you transmit a block only every 10 seconds, 9 previous blocks will be lost, because VE.Direct devices publish every second<br>This is normally not a problem.
- During sending OneWire Data or OTA check no VE.Direct blocks will be sent.<br>This is a limitation of the ESP32, having several tasks tranmitting in parallel caused crashes of the device

## Hardware Installation
![Please see the wiki](https://github.com/RalfJL/VE.Direct2MQTT/wiki)

## Software configuration
![Please see the wiki](https://github.com/RalfJL/VE.Direct2MQTT/wiki)

## Debugging
![Please see the wiki](https://github.com/RalfJL/VE.Direct2MQTT/wiki)

## Disclaimer
I WILL NOT BE HELD LIABLE FOR ANY DAMAGE THAT YOU DO TO YOU OR ONE OF YOUR DEVICES.