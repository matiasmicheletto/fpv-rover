# FPV Wi-Fi rover 

This project consists on a simple three-wheeled rover built using the [chassis kit](https://www.amazon.com/perseids-Chassis-Encoder-Wheels-Battery/dp/B07DNYQ3PX/ref=sr_1_5?dchild=1&keywords=arduino+rover+robot&qid=1630333979&sr=8-5) and controlled through Wi-Fi network with a websocket server running on an ESP-32 microcontroller.  

Basically, through a web based client application that you can run in your browser, you get a HUD on your screen with the rover's point of view, and by moving the cursor over the HUD, you send the control action to the rover via websocket commands. It is also available an option that uses a Tobii Eyetracker to control the rover.  

## System main components
![Scheme](img/GeneralSchemeWB.png)  

## The assembled rover
![Assembling](img/assembling/Assembling_6.jpg)  

## Screen capture of the client application (controlled with mouse)
[![Demo](img/screen-capture.gif)](https://www.youtube.com/watch?v=7270GWGmxQA)  

## Requirements

#### Version 1
  - ESP32-CAM.  
  - L298n Dual Full-Bridge.  
  - Kit 3 wheeled robot.  
  - Chrome web browser.  

#### Version 2
  - NodeMCU Amica v3.  
  - L298n Dual Full-Bridge.  
  - Kit 3 wheeled robot.  
  - Tobii Eyetracker.  
  - Android Smartphone with IP camera app. For example:  
  https://play.google.com/store/apps/details?id=com.pas.webcam&hl=en  
  - Chrome web browser.  

## Getting started

#### Version 1
  1. Configure SSID and password of your network in the ESP32-CAM code using the FTDI module.  
  2. Turn on the rover.  
  3. Run the client with the Chrome browser, in your PC.  
  4. Put the corresponding IP addresses of the rover and the IP camera and push the "connect" buttons to stablish the websocket conection.  
  5. That's it. Have fun!  

#### Version 2
  1. Configure the smartphone as hotspot using "EyeRobot" as SSID and "eyerobot123" as password:  
  ![Android AP](img/smartphone_ap_LR.jpg)
  2. Turn on the rover.  
  3. Using the smartphone, verify that the rover has successfully conected and write down the IP address of the device:  
  ![Foto](img/smartphone_rover_ip_LR.jpg)
  4. Run the IP camera app and write down the connection IP.  
  5. Attach the smartphone to the rover. It is also recommended a remote control app for the smartphone, so you can use it from the computer without removing it from the rover.  
  6. Connect your personal computer to the "EyeRobot" wifi network.  
  7. Run the client with the Chrome browser, in your PC.  
  8. Put the corresponding IP addresses of the rover and the IP camera and push the "connect" buttons to stablish the websocket conection.  
  9. That's it. Have fun!  

## Tobii EyeX Web Socket Server

The author of the Eyetracker Tobii Web Socket Server is Stevche Radevski:  
https://github.com/sradevski/Tobii-EyeX-Web-Socket-Server  


## License

This project is published under the GPL V3.0 license.