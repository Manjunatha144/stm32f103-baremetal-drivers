\# STM32F103 Bare-Metal AHRS Telemetry \& Sensor Fusion System



A high-performance, register-level embedded telemetry engine built on the ARM Cortex-M3 (STM32F103C8T6 "Blue Pill") and MPU-6050 6-DOF IMU. The firmware runs an autonomous 50 Hz attitude estimation pipeline utilizing a custom Madgwick quaternion filter, zero-copy USART DMA streaming, circular RX command parsing, and autonomous I2C bus fault recovery without HAL/LL library dependencies.



Includes an interactive WebGL 3D telemetry dashboard via the browser Web Serial API and an automated Python Hardware-in-the-Loop (HIL) qualification harness.



---



\## Technical Highlights



\- \*\*Bare-Metal Register Architecture:\*\* 100% direct register manipulation for RCC (72 MHz PLL clock tree), GPIO, SysTick, I2C1, and USART1 with zero external vendor libraries.

\- \*\*Hardware Fault Recovery:\*\* Integrated 9-clock bit-bang recovery sequence executed on boot to clear peripheral SDA lockups caused by unclean MCU resets or slave clock stretching.

\- \*\*Asynchronous Dual DMA Pipeline:\*\*

&nbsp; - \*\*TX (Channel 4):\*\* Non-blocking memory-to-peripheral burst transmission of 11-byte packed binary frames.

&nbsp; - \*\*RX (Channel 5):\*\* Circular ring-buffered DMA receiver for host-to-device command handling without CPU interrupt overhead.

\- \*\*6-DOF Quaternion Sensor Fusion:\*\* On-chip Madgwick AHRS implementation yielding drift-compensated, singularity-free unit quaternions ($w, x, y, z$).

\- \*\*Automated HIL Qualification:\*\* Verified timing accuracy and transmission reliability through automated Python test scripts.



---



\## Hardware Pinout \& Wiring



| Peripheral | STM32 Pin | Connected Device | Function / Mode |

|---|---|---|---|

| \*\*I2C1\_SCL\*\* | `PB6` | MPU-6050 `SCL` | I2C Clock (Open-Drain, 400 kHz) |

| \*\*I2C1\_SDA\*\* | `PB7` | MPU-6050 `SDA` | I2C Data (Open-Drain, 400 kHz) |

| \*\*USART1\_TX\*\* | `PA9` | CP2102 `RXD` | DMA Telemetry Stream (115200 8N1) |

| \*\*USART1\_RX\*\* | `PA10` | CP2102 `TXD` | Circular DMA Command Stream |

| \*\*Heartbeat LED\*\* | `PC13` | Onboard LED | System Heartbeat \& Calibration Status |

| \*\*Power Rails\*\* | `3.3V` / `GND` | MPU-6050 / CP2102 | Regulated Power \& Ground |



---



\## Telemetry Protocol \& Binary Packet Structure



The STM32 streams packed 11-byte binary frames at 50 Hz. Floating-point quaternion values are scaled by $10,000$ and cast to signed 16-bit integers (`int16\_t`) to optimize transmission bandwidth:

| Byte Index | Field | Data Type | Description |

|---|---|---|---|

| `\[0:1]` | Header | `uint8\_t\[2]` | Frame sync preamble: `0xAA, 0x55` |

| `\[2:3]` | `q0` (w) | `int16\_t` (LE) | Scaled quaternion component ($\\times 10,000$) |

| `\[4:5]` | `q1` (x) | `int16\_t` (LE) | Scaled quaternion component ($\\times 10,000$) |

| `\[6:7]` | `q2` (y) | `int16\_t` (LE) | Scaled quaternion component ($\\times 10,000$) |

| `\[8:9]` | `q3` (z) | `int16\_t` (LE) | Scaled quaternion component ($\\times 10,000$) |

| `\[10]` | CRC | `uint8\_t` | Longitudinal XOR parity over bytes `0` to `9` |



\### Host-to-Device Command Set (USART1 RX DMA)

Sending single-byte ASCII characters triggers immediate firmware execution:

\- `'C'`: Executes a 500-sample gyroscope zero-bias recalibration and tares orientation.

\- `'1'`: Sets Madgwick filter gain $\\beta = 0.03$ (High stability / smooth mode).

\- `'2'`: Sets Madgwick filter gain $\\beta = 0.08$ (Balanced default mode).

\- `'3'`: Sets Madgwick filter gain $\\beta = 0.25$ (High dynamic response mode).



---



\## HIL Verification \& QA Benchmark Report



Automated hardware-in-the-loop qualification captured across 500 consecutive binary frames:



```text

============================================================

STM32F103 HIL Validation Suite | Listening on COM6 @ 115200 baud

============================================================

Capturing 500 binary telemetry packets...

\[100/500] Captured Q: \[ 0.003, -0.233,  0.074,  0.021]

\[200/500] Captured Q: \[-0.004, -0.233,  0.070,  0.018]

\[300/500] Captured Q: \[-0.003, -0.233,  0.064,  0.007]

\[400/500] Captured Q: \[ 0.003, -0.233,  0.063,  0.007]

\[500/500] Captured Q: \[ 0.005, -0.233,  0.067,  0.013]



============================================================

HIL VALIDATION TEST REPORT

============================================================

Total Valid Packets:   500

Sync Desync Events:    0

Checksum Failures:     0

Data Integrity Rate:   100.00%

Average Frame Period:  20.00 ms (Target: 20.0 ms)

Effective Frequency:   50.00 Hz (Target: 50.0 Hz)

Total Test Duration:   9.99 seconds

============================================================

\[PASS] Firmware meets professional embedded stability standards.



Repository Structure



├── Inc/

│   ├── i2c.h              # I2C register prototypes \& recovery declarations

│   ├── madgwick.h         # Madgwick AHRS filter API \& state definitions

│   ├── systick.h          # Precision millisecond timer interface

│   └── usart.h            # USART1 \& DMA ring buffer prototypes

├── Src/

│   ├── i2c.c              # Register-level I2C read/write driver \& bus recovery

│   ├── madgwick.c         # Madgwick gradient descent algorithm

│   ├── main.c             # 72 MHz PLL clock, setup, and 50 Hz control loop

│   ├── systick.c          # SysTick hardware delay implementation

│   └── usart.c            # Asynchronous DMA1 Ch4 TX \& Ch5 circular RX drivers

├── visualizer.html        # Three.js 3D telemetry dashboard (Web Serial API)

├── hil\_validator.py       # Python automated test suite \& timing analyzer

└── README.md

