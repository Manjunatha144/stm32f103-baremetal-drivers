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
## Debugging Challenges Solved

### 1. I2C SDA Line Lockup on MCU Reset

Issue:
Resetting the STM32 mid-transaction left the MPU-6050 holding the open-drain SDA line low, causing the STM32 I2C1 peripheral to stay permanently stuck in the BUSY state.

Fix:
Implemented an autonomous startup recovery sequence that configures PB6 (SCL) as a GPIO output and toggles 9 clock pulses followed by a manual STOP condition before enabling the I2C1 peripheral.

---

### 2. Linker Undefined Reference (usart1_dma_read_cmd)

Issue:
Build failed during linking because the circular RX DMA ring buffer reader and Channel 5 register macros were declared in usart.h but missing from the translation unit in usart.c.

Fix:
Implemented DMA1_CH5 register definitions, the 16-byte static s_rx_ring buffer, and the usart1_dma_read_cmd() index tracker comparing RX_RING_SIZE - DMA1_CNDTR5.

---

### 3. Serial Port Access Denied (PermissionError on COM6)

Issue:
Python HIL validation script failed to open COM6 while the WebGL visualizer was active in Chrome.

Fix:
Released the Web Serial API hardware handle in the browser prior to executing automated Python HIL verification runs.

---

### 4. Loop Timing Drift (52.64 Hz vs 50.00 Hz Target)

Issue:
Initial systick_delay_ms(18) combined with ~1.0 ms I2C burst-read and Madgwick execution resulted in a 19.00 ms period (52.64 Hz), triggering a timing warning in the HIL validator.

Fix:
Calibrated loop delay to systick_delay_ms(19), achieving an exact 20.00 ms average frame period (50.00 Hz) with a 100.00% HIL pass rate.

## Tools & Technologies

### Embedded Platforms
- STM32F103C8T6 (ARM Cortex-M3 @ 72 MHz)
- InvenSense MPU-6050 6-DOF IMU
- Silicon Labs CP2102 USB-to-UART Bridge

### Architecture
- Bare-Metal Register-Level C (CMSIS / Reference Manual RM0008)
- Direct Memory Access (DMA1 Channel 4 & Channel 5)

### Communication Protocols
- I2C (400 kHz Fast-Mode)
- UART / USART (115200 8N1)
- Custom 11-Byte Binary Telemetry Framing with XOR Checksum

### Development & Validation Tools
- STM32CubeIDE & GNU Arm Embedded Toolchain (arm-none-eabi-gcc)
- Python 3 (pyserial, struct) for HIL Automated Testing
- Three.js (WebGL) & HTML5 Web Serial API
- Git & GitHub

## Hardware Pinout

| STM32F103 Pin | Peripheral | Connected Device | Mode / Function |
|---|---|---|---|
| PB6 | I2C1_SCL | MPU-6050 SCL | Alternate Function Open-Drain (400 kHz) |
| PB7 | I2C1_SDA | MPU-6050 SDA | Alternate Function Open-Drain (400 kHz) |
| PA9 | USART1_TX | CP2102 RXD | Alternate Function Push-Pull (DMA1 Ch4) |
| PA10 | USART1_RX | CP2102 TXD | Input Pull-Up (DMA1 Ch5 Circular Ring) |
| PC13 | GPIOC_ODR | Onboard LED | Active-Low Status & Calibration Indicator |

## Binary Telemetry Packet Structure (11 Bytes)

| Byte Index | Field | Data Type | Description |
|---|---|---|---|
| [0:1] | Header | uint8_t[2] | Frame sync preamble: 0xAA, 0x55 |
| [2:3] | q0 (w) | int16_t (LE) | Scaled quaternion component (w * 10000) |
| [4:5] | q1 (x) | int16_t (LE) | Scaled quaternion component (x * 10000) |
| [6:7] | q2 (y) | int16_t (LE) | Scaled quaternion component (y * 10000) |
| [8:9] | q3 (z) | int16_t (LE) | Scaled quaternion component (z * 10000) |
| [10] | CRC | uint8_t | Longitudinal XOR parity over bytes 0 to 9 |

### Host Control Commands (USART1 RX DMA)
- 'C' : Execute 500-sample live gyroscope zero-bias recalibration and tare attitude
- '1' : Set Madgwick filter gain beta = 0.03 (Smooth / High Stability)
- '2' : Set Madgwick filter gain beta = 0.08 (Normal / Balanced)
- '3' : Set Madgwick filter gain beta = 0.25 (Fast / High Dynamic Response)

------------------------------------------------------------
    STM32F103 HIL Validation Suite | Listening on COM6 @ 115200 baud
    ------------------------------------------------------------
    Capturing 500 binary telemetry packets...
    [100/500] Captured Q: [ 0.003, -0.233,  0.074,  0.021]
    [200/500] Captured Q: [-0.004, -0.233,  0.070,  0.018]
    [300/500] Captured Q: [-0.003, -0.233,  0.064,  0.007]
    [400/500] Captured Q: [ 0.003, -0.233,  0.063,  0.007]
    [500/500] Captured Q: [ 0.005, -0.233,  0.067,  0.013]

    ------------------------------------------------------------
    HIL VALIDATION TEST REPORT
    ------------------------------------------------------------
    Total Valid Packets:   500
    Sync Desync Events:    0
    Checksum Failures:     0
    Data Integrity Rate:   100.00%
    Average Frame Period:  20.00 ms (Target: 20.0 ms)
    Effective Frequency:   50.00 Hz (Target: 50.0 Hz)
    Total Test Duration:   9.99 seconds
    ------------------------------------------------------------
    [PASS] Firmware meets professional embedded stability standards.




## Repository Structure

- **Inc/** — Header files (`gpio.h`, `i2c.h`, `madgwick.h`, `systick.h`, `usart.h`)
- **Src/** — Register-level C drivers (`gpio.c`, `i2c.c`, `madgwick.c`, `main.c`, `systick.c`, `usart.c`)
- **Startup/** — ARM Cortex-M3 assembly startup (`startup_stm32f103c8tx.s`)
- **visualizer.html** — Three.js 3D WebGL telemetry dashboard (Web Serial API)
- **hil_validator.py** — Automated Python HIL packet integrity & timing validator
- **STM32F103C8TX_FLASH.ld** — Linker script

## Build & Run

### 1. Clone Repository
`git clone https://github.com/Manjunatha144/stm32f103-baremetal-drivers.git`

### 2. Build & Flash Firmware
Open project in STM32CubeIDE, build (`Ctrl + B`), and flash via ST-Link V2 (`Ctrl + F11`).

### 3. Launch 3D WebGL Visualizer
Open `visualizer.html` in Google Chrome or Microsoft Edge, click **Connect**, and select your CP2102 COM port (`115200` baud).

### 4. Run Automated HIL Validation
Disconnect the browser visualizer and run in terminal:
`python -m pip install pyserial`
`python hil_validator.py`

## Author

### Manjunatha H

Embedded and IoT Firmware Developer
