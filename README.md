# HaptiNav — Wireless Haptic Navigation System

## Overview

HaptiNav is a wearable assistive technology prototype designed to help visually impaired individuals detect nearby obstacles through ultrasonic sensing and directional haptic feedback.

The system combines a sensor-equipped headband with a feedback glove, using wireless communication between two ESP32 microcontrollers.

## Problem Statement

Navigating unfamiliar environments can be challenging for visually impaired individuals. HaptiNav explores an additional way to communicate nearby obstacle information through tactile feedback.

## System Architecture

- Sensing unit: Ultrasonic sensors mounted on the headband.
- Processing unit: ESP32 microcontroller processes sensor readings.
- Wireless communication: ESP-NOW transmits information between the headband and glove.
- Feedback unit: A second ESP32 controls servo motors on the glove to provide directional cues.

## Hardware Components

- ESP32 development boards × 2
- Ultrasonic sensors × 4
- Servo motors × 3
- Headband and glove
- Power supply and connecting wires

## Challenges

- Reliable obstacle detection at different angles.
- Inconsistent ultrasonic reflections from certain surfaces.
- Integration of wireless communication and mechanical feedback.
- Wearability, comfort, and power management.

## Future Improvements

- Improve detection of ground-level obstacles.
- Explore vibration motors for tactile feedback.
- Optimize power consumption and wearable design.
- Conduct further testing in different environments.

## Project Status

Prototype developed; further testing and refinement are planned.

## Disclaimer

HaptiNav is an educational prototype and is not a replacement for established mobility aids such as a white cane or guide dog.
