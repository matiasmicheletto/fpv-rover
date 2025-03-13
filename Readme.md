# FPV Wi-Fi rover 

A three-wheeled rover built using a prototyping [chassis kit](https://www.amazon.com/perseids-Chassis-Encoder-Wheels-Battery/dp/B07DNYQ3PX) and the [ESP32-CAM](https://www.amazon.com/ESP32-CAM-MB-Aideepen-ESP32-CAM-Bluetooth-Arduino/dp/B0948ZFTQZ/ref=sr_1_2_sspa) board.  

By means of a HUD Display running in your browser, you can see the rover's point of view, and by moving the cursor over the screen, you can control the rover through WebSocket commands.  

## Demo
[![Demo](img/screen-capture.gif)](https://www.youtube.com/watch?v=7270GWGmxQA)  

## Requirements

  - ESP32-CAM.  
  - L298n Dual Full-Bridge.  
  - Kit 3 wheeled robot: chassis, motors, wheels and battery sockets.  
  - WiFi network (no internet needed).  
  - Web browser with WebSocket API support.  

## Getting started

  1. Build the rover attaching motors to the chasis, L298n bridge, power supply and on-off switch. Use at least 9V as supply, and not higher than 12V. A set of seven 1.5V AA batteries will make the job.     
  2. Create a ``credentials.h`` file in the [firmware/src/](firmware/src/) folder with your local WiFi SSID and password, for example:
  ```cpp
    const char* ssid = "MY_SSID";  
    const char* password = "my_pass";  
  ```
  3. Compile and upload the firmware to the ESP32-CAM board using the corresponding programmer shield. You can use the Arduino IDE or any other platform of your choice.  
  4. Use a serial monitor to see the IP address that your local network provided to the rover's ESP32-CAM board. After that, you can disconnect the board from the programmer shield.  
  5. Connect the microcontroller to the bridge using the pin map configured in [firmware/src/config.h](firmware/src/config.h). Use the 5V output of the bridge as power supply for the microcontroller. Now, the harware configuration is done, and the rover should be ready to use.  
  6. In your computer connected to the same WiFi network of the rover, setup a local server to serve the [client](client/index.html) web application. Open the app with your browser, depending on the server application, usually it is hosted with the URL: http://localhost:8080.   
  7. Set the IP address of the rover in the input and hit the "connect" button. You should see the video captured by the rovers camera as the background of the screen. If that doesn't work, then check the [client/js/index.js](client/js/index.js) script to look for any mismatch between ports or IP configurations.  
  8. That's it, control the rover by moving the cursor over the screen and have fun!  

## License

This project is published under the GPL V3.0 license.