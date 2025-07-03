# Shuttle Bus Tracking System

A comprehensive IoT and embedded systems project designed for university-level shuttle bus tracking and monitoring. This system integrates multiple sensors and technologies to provide real-time location tracking, passenger queue monitoring, environmental sensing, and safety alerts for efficient campus shuttle operations.

## 🧠 Features

- **GPS Tracking**
  - Real-time location tracking using the u-blox NEO-6M GPS module to monitor shuttle movement.
- **Queue Monitoring System**
  - Laser and LDR-based people counter to monitor queue length at bus stops.
  - Triggers a buzzer when the queue exceeds a predefined threshold.
- **RFID Authentication**
  - Verifies passengers using RFID cards to ensure only registered individuals can board.
- **Environmental Monitoring**
  - **DHT11 Sensor**: Measures temperature and humidity inside the bus.
  - **MQ2 Gas Sensor**: Detects harmful gases (e.g., smoke or LPG leaks) for safety alerts.
- **Safety Alerts**
  - Buzzer alerts for:
    - Gas leakage (distinct pattern)
    - Overcrowding at bus stops
    - Unauthorized RFID access

## 🛠️ Hardware Components

- ESP32 (or Arduino-based MCU)
- u-blox NEO-6M GPS Module
- RFID Reader (RC522)
- DHT11 Sensor
- MQ-2 Gas Sensor
- Laser emitter + LDR (Queue counter)
- Buzzer
- Power supply & display units (optional)

## 📚 Use Cases

- University shuttle bus management
- Smart campus transportation systems
- Real-time passenger and safety monitoring

## 📸 Circuit Diagram

![Circuit Diagram](https://github.com/user-attachments/assets/5d94cf5a-3680-42a2-81e1-930c1b37c8b7)
