# Smart-Food-Scale: Integrated Digital
Weighing and Wireless Transmission
System
ScalPro BLE is a high-precision weight measurement platform developed on the ESP32
microcontroller, designed to facilitate wireless data logging and intelligent power management.
The system serves as a bridge between high-sensitivity analog load cell sensors and modern
mobile computing devices through the implementation of the Bluetooth Low Energy (BLE)
protocol.
System Design and Signal Processing
The core of the system is built around the HX711 analog-to-digital converter, which is
specialized for weigh scale applications. To ensure measurement stability in diverse
environments, the software implements a dynamic Exponential Moving Average (EMA) filter.
This filtering algorithm utilizes a variable Alpha coefficient that adjusts based on the magnitude
of weight fluctuations, allowing the system to achieve high noise rejection during static weighing
while maintaining rapid response times during active loading.
Communication Architecture
Wireless data integration is achieved through a dedicated BLE server architecture. The system
defines a primary service with specific UUIDs to notify connected clients, such as tablets or
dedicated mobile applications, of weight changes instantaneously. This notification-based
communication reduces power consumption compared to continuous polling while ensuring that
the remote data log remains synchronized with the physical scale.
Energy Management and Persistence
Designed with portability in mind, the system incorporates advanced power management
features using the ESP32 Real-Time Clock (RTC) peripherals. It supports a deep-sleep state
that significantly reduces current draw when the device is idle, while maintaining external
wake-up capabilities through hardware interrupts. To ensure operational consistency, calibration
parameters including the scale factor and zero-offset are stored in Non-Volatile Memory via
EEPROM, eliminating the need for recalibration after power cycles.
User Interface and Configuration
The application provides a dual-interface system for operation and maintenance. For real-time
monitoring, an I2C-based Liquid Crystal Display provides immediate visual feedback. For
administrative tasks, a serial-based command interface allows for precise calibration
procedures, including taring and full-scale adjustment with reference weights. This multi-layered

approach ensures the system is both accessible for end-users and configurable for technical
maintenance.
Developed by Isara Phetsila, Computer Engineering Student at Ramkhamhaeng University.
