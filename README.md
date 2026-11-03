# Bare-Metal STM32F103 AHRS & Telemetry Firmware
### Register-Level ARM Cortex-M3 + Madgwick Sensor Fusion + Dual DMA

Bare-metal embedded firmware developed in C for the STM32F103C8T6 (ARM Cortex-M3) without HAL/LL libraries. Implements 400 kHz I2C sensor acquisition, autonomous I2C bus lockup recovery, 6-DOF Madgwick quaternion sensor fusion, bidirectional USART1 DMA communication, and a real-time 3D WebGL telemetry visualizer.

## Key Features

- 100% Bare-Metal Register-Level C Architecture (Zero HAL/LL bloat)
- 72 MHz System Clock configuration via direct RCC register manipulation
- Autonomous 9-clock I2C bus fault recovery routine
- 6-DOF Madgwick Quaternion AHRS filter running at an exact 50.00 Hz
- Asynchronous USART1 TX via DMA1 Channel 4 (Zero CPU blocking)
- Lock-free Circular Ring Buffer RX via DMA1 Channel 5
- Live gyroscope zero-bias calibration and runtime filter gain (Beta) tuning
- Real-time Three.js 3D WebGL visualizer over Web Serial API
- Automated Python Hardware-in-the-Loop (HIL) validation suite (100.00% integrity)

## System Architecture

MPU-6050 IMU (I2C1 @ 400 kHz)
→ Bare-Metal I2C Driver
→ 6-DOF Madgwick Quaternion Filter (50.00 Hz)
→ Packed 11-Byte Binary Frame + XOR Checksum
→ DMA1 Channel 4 (USART1 TX)
→ CP2102 USB-UART Bridge (115200 Baud)
→ Three.js 3D WebGL Dashboard & Python HIL Validator

## Firmware Architecture

### main.c

Responsible for:
- 72 MHz HSE PLL clock tree initialization
- Peripheral startup sequencing (SysTick, GPIO, I2C1, USART1, DMA1)
- 500-sample startup and runtime gyroscope zero-bias offset calibration
- Deterministic 50.00 Hz (20.00 ms) sensor fusion and telemetry control loop
- Non-blocking host command execution ('C', '1', '2', '3')

---

### i2c.c / i2c.h

Responsibilities:
- Autonomous 9-clock bit-bang bus recovery to clear stuck low SDA lines
- Direct register configuration for I2C1 Fast-Mode (400 kHz)
- Multi-byte burst read transactions (14-byte accelerometer + gyroscope readout)
- Hardware ACK/NACK and STOP condition sequencing

---

### usart.c / usart.h

Responsibilities:
- USART1 register initialization (115200 baud, 8N1)
- DMA1 Channel 4 configuration for non-blocking memory-to-peripheral TX bursts
- DMA1 Channel 5 configuration in Circular Mode for continuous background RX
- Lock-free ring buffer parser tracking hardware DMA1_CNDTR5 against software read index

---

### madgwick.c / madgwick.h

Responsibilities:
- 6-DOF Quaternion attitude estimation (q0, q1, q2, q3)
- Gradient descent accelerometer drift correction
- Fast inverse square root computation for Cortex-M3 (no hardware FPU)
- Dynamic runtime filter gain (beta) adjustment

---

### systick.c / systick.h

Responsibilities:
- Cortex-M3 Core SysTick timer initialization
- Deterministic 1 ms hardware delay generation for 50.00 Hz loop pacing

## System Flow

- **1. Boot & Clocks:** Power On → Reset_Handler → main() → 72 MHz RCC PLL Clock Configuration → SysTick Timer Initialization
- **2. Bus Recovery & Peripherals:** I2C1 9-Clock Bus Recovery Sequence → I2C1 & MPU-6050 Register Init → USART1 & Dual DMA1 (Ch4 TX / Ch5 Circular RX) Init
- **3. Sensor Calibration:** 500-Sample Stationary Gyroscope Zero-Bias Offset Calibration
- **4. Deterministic 50.00 Hz Control Loop:**
  - Poll Circular DMA1 Ch5 Ring Buffer for Host Commands
  - Burst-Read 14 Bytes from MPU-6050 over I2C1
  - Convert Raw LSBs to Physical Units (g and deg/s)
  - Update 6-DOF Madgwick Quaternion Filter
  - Pack 11-Byte Binary Frame with XOR Checksum
  - Trigger Non-Blocking DMA1 Ch4 Telemetry Burst
  - Wait Calibrated 19 ms Frame Delay (20.00 ms Total Period)

## Engineering Decisions

### Why Bare-Metal Registers Instead of STM32 HAL?

Direct register manipulation eliminates HAL abstraction overhead, reduces Flash/SRAM footprint, and provides cycle-accurate control over I2C hardware flags and DMA stream re-arming.

---

### Why Dual DMA (TX Ch4 + Circular RX Ch5)?

Offloading serial communication to DMA prevents CPU blocking during 115200 baud transmissions and allows asynchronous host command reception without UART RX interrupt latency jittering the 50 Hz sensor fusion loop.

---

### Why Madgwick Quaternion Filter Over Euler Angles or EKF?

Euler angles suffer from gimbal lock at ±90 degrees pitch, while an Extended Kalman Filter (EKF) is computationally expensive on an ARM Cortex-M3 lacking a hardware FPU. The Madgwick filter avoids singularities and converges rapidly using lightweight gradient descent math.

---

### Why 11-Byte Binary Protocol Instead of ASCII/JSON?

Streaming formatted ASCII floats requires 45+ bytes per frame and slow sprintf calls. Scaling quaternions to int16_t (x10000) inside a packed 11-byte binary frame cuts bandwidth by 75% and guarantees deterministic framing at 115200 baud.
