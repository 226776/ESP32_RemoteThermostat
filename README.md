# ESP32_RemoteThermostat
Remote thermostat using blebox devices. 

## Project Description

Blebox tempSensor can be used as thermostat by it self, but it has no possibility (yet) to check device state, and work in conditional.
So for that kind of tasks, there is a need to use server, since blebox devices comunicate using url api commands.
The goal is, to create an embedded server working on ESP32 (idf), that can be a control unit hanfling remote thermostat task.
This can also be template for other more complicated tasks using blebox devices.

## Project info
- Platform: Linux Debian 10 
- Software: Eclipse (with ESP32 Dev plugins) 

## Done 
- Starting access point, connecting to local wi-fi (handled by default event loop)
- Starting new thread
- Sending request url 
- Getting request response
- Phrasing response json 
- Multithreading


## TODO:
-
