# ESP32 RemoteThermostat
Remote thermostat using blebox devices working on ESP32. 

## Autors
- Krystian Kasprów (226776) 
- Anita Grontman (226752)

## Project Description

Blebox tempSensor can be used as thermostat by it self, but it has no possibility (yet) to check device state, and work in conditional.
So for that kind of tasks, there is a need to use server, since blebox devices comunicate using url api commands.
The goal is, to create an embedded server working on ESP32 (idf), that can be a control unit handling remote thermostat task.
This can also be template for other more complicated tasks using blebox devices.

## Project info
- Software: Eclipse (with ESP32 Dev plugins) 

## Done 
- Start access point, connect to local wi-fi (handled by default event loop)
- Start new thread
- Send request url
- Get request response	
- Phrase response json 
- Multithreading 
- GateBox state read
- TempSensor state read
- SwitchBox command send
- http server
- html web page send as http response 
- pass data through control panel (browser) to device
- thermostat function

## TODO:
- pass data through web page/control panel both directions (print device state)
- add script to search for IP device 
- device is disconnecting from grid after some time (few minutes), fix needed

## Control panel (html page) 
If device has successfully connected to wifi and receives IP address, <br />
use browser and type IP address. You will see control panel of device.
  
![Image description](images/webPage_view.png)
