# Working Principle of HaptiNav

## Obstacle Detection
The headband contains four ultrasonic sensors positioned to detect obstacles in different directions: left, center, right and downward.
Each sensor measures the distance to an obstacle using ultrasonic sound waves.

## Data Processing and communication
The ESP32 on the headband reads the sensor measurements and processes the distance information.
This helps the system determine the direction of nearby obstacles.
The headband ESP32 transmits the processed sensor information wirelessly to a second ESP32 mounted on the glove using ESP-NOW.

## Haptic Feedback
The glove uses servo motors to provide physical feedback corresponding to the detected obstacle direction.
The user can interpret these feedback patterns to understand where an obstacle has been detected.

## System Workflow
First, the ultrasonic sensors measure the distances to nearby obstacles.
The headband ESP32 processes these measurements and transmits the sensor information wirelessly to the glove ESP32.
The receiving ESP32 interprets the data and controls the servo motors to provide directional physical feedback to the user.
The motor gives feedback in such a way that when the obstacle is more far from the person , it will gently tap teh finger ,
when obstacle is little more close , the tap will have fore frequency , when the obstacle is really close, the motor
will tap with teh highest frequency.

## Safety Note
HaptiNav is a prototype developed for experimentation and learning.
It is not a replacement for a white cane, guide dog or other established mobility aids.
Its reliability and safety must be validated before practical use.
