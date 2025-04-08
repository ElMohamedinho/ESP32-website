# ESP32-website
this is part of a project for a competition where the esp32 connects to a Wi-Fi network and hosts a website that only people connected to the same network can access by typing the esp32 Ip address into any web browser (you can get the ip adress from the serial monitor in the arduino ide )
displaying dashboard containing estimations calculated from sensors 

🔋 ESP32-S3 System Dashboard
This web-based dashboard is hosted on an ESP32-S3 and provides real-time monitoring for a hybrid renewable energy system combining Concentrated Solar Power (CSP) and Mountain Gravity Energy Storage (MGES).

📊 Dashboard Displays:
System Status: Current operational mode (e.g., Idle).

Current Energy Storage Level: Estimated percentage of stored energy (e.g., 81%).

Total Stored Potential Energy: Gravitational energy stored (e.g., 2.21 MWh).

Current Output from Thermal Power Plant: Output power from the CSP system (e.g., 11.8 kW).

Storage Level Graph: Time-series visualization of energy storage fluctuations.

🔧 Sensor & Control Integration:
Light Tracking:

Uses 3 photoresistors (LDRs):

leftTop → analog pin 5

rightBottom → analog pin 6

leftBottom → analog pin 7


These values help determine the optimal angle for sunlight capture.

Distance Sensing:

Ultrasonic sensor to estimate storage height (gravity energy).

Mirror Orientation:

Servo motor adjusts CSP mirror angle automatically.

Status Indicator:

Built-in RGB LED shows Wi-Fi connection state:

🔴 Red = Disconnected

🟢 Green = Connected

🟡 yellow = connecting

