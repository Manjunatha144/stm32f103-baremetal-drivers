import serial
import time
import struct

# Line 6: Set this to your actual CP2102/USB-UART COM port from Device Manager
PORT = 'COM6'
BAUD = 115200
PACKET_SIZE = 11
TARGET_PACKETS = 500  # Number of packets to capture for validation

def run_hil_validation():
    print("=" * 60)
    print(f"STM32F103 HIL Validation Suite | Listening on {PORT} @ {BAUD} baud")
    print("=" * 60)

    try:
        ser = serial.Serial(PORT, BAUD, timeout=1.0)
    except Exception as e:
        print(f"[ERROR] Could not open port {PORT}: {e}")
        return

    ser.reset_input_buffer()

    valid_packets = 0
    checksum_errors = 0
    sync_errors = 0
    
    start_time = time.time()
    last_packet_time = start_time
    intervals = []

    print(f"Capturing {TARGET_PACKETS} binary telemetry packets...")

    buffer = bytearray()

    while valid_packets < TARGET_PACKETS:
        data = ser.read(ser.in_waiting or 1)
        if data:
            buffer.extend(data)

        # Process complete frames in buffer
        while len(buffer) >= PACKET_SIZE:
            # Check 2-byte sync header: 0xAA, 0x55
            if buffer[0] != 0xAA or buffer[1] != 0x55:
                buffer.pop(0)
                sync_errors += 1
                continue

            # Extract 11-byte frame
            frame = buffer[:PACKET_SIZE]

            # Verify XOR Checksum over first 10 bytes
            chk = 0
            for i in range(10):
                chk ^= frame[i]

            if chk != frame[10]:
                checksum_errors += 1
                buffer.pop(0)
                continue

            # Unpack signed 16-bit quaternion components (scaled by 10,000)
            _, q0, q1, q2, q3 = struct.unpack('<xxhhhhB', frame)
            
            w = q0 / 10000.0
            x = q1 / 10000.0
            y = q2 / 10000.0
            z = q3 / 10000.0

            now = time.time()
            intervals.append((now - last_packet_time) * 1000.0)
            last_packet_time = now

            valid_packets += 1
            if valid_packets % 100 == 0:
                print(f"[{valid_packets}/{TARGET_PACKETS}] Captured Q: [{w:.3f}, {x:.3f}, {y:.3f}, {z:.3f}]")

            # Remove verified packet from buffer
            del buffer[:PACKET_SIZE]

    ser.close()
    total_time = time.time() - start_time

    # Calculate validation metrics
    avg_interval = sum(intervals[1:]) / len(intervals[1:]) if len(intervals) > 1 else 0
    measured_hz = 1000.0 / avg_interval if avg_interval > 0 else 0
    total_errors = checksum_errors + sync_errors
    integrity_rate = (valid_packets / (valid_packets + total_errors)) * 100.0 if (valid_packets + total_errors) > 0 else 0

    print("\n" + "=" * 60)
    print("HIL VALIDATION TEST REPORT")
    print("=" * 60)
    print(f"Total Valid Packets:   {valid_packets}")
    print(f"Sync Desync Events:    {sync_errors}")
    print(f"Checksum Failures:     {checksum_errors}")
    print(f"Data Integrity Rate:   {integrity_rate:.2f}%")
    print(f"Average Frame Period:  {avg_interval:.2f} ms (Target: 20.0 ms)")
    print(f"Effective Frequency:   {measured_hz:.2f} Hz (Target: 50.0 Hz)")
    print(f"Total Test Duration:   {total_time:.2f} seconds")
    print("=" * 60)
    if integrity_rate >= 99.0 and 48.0 <= measured_hz <= 52.0:
        print("[PASS] Firmware meets professional embedded stability standards.")
    else:
        print("[WARN] Timing or packet drop anomalies detected.")

if __name__ == "__main__":
    run_hil_validation()