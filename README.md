# STM32F103 Bare-Metal AHRS Telemetry System

Register-level C firmware on ARM Cortex-M3 (STM32F103) running an autonomous 50 Hz sensor fusion pipeline with MPU-6050, custom Madgwick quaternion filter, dual DMA (TX/circular RX), and Three.js WebGL visualizer.

## Features
- Direct register programming (RCC 72MHz, I2C1, USART1, SysTick) with zero HAL/LL libraries.
- Autonomous 9-clock I2C bus recovery routine to clear slave SDA lockups.
- Asynchronous DMA1 Ch4 (TX telemetry) and Ch5 (circular RX command parser).
- Hardware-in-the-Loop (HIL) validated with 100% packet integrity at 50.00 Hz.

## Hardware Wiring
- **MPU-6050 I2C1:** SCL -> PB6, SDA -> PB7 (3.3V, GND)
- **CP2102 USART1:** TX -> PA10, RX -> PA9 (115200 8N1)
