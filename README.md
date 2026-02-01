# Smart Garbage Bin – Pin Assignment Documentation

## 1. Arduino Board Overview

| Peripheral                   | Arduino Pin         | Notes |
|-------------------------------|-------------------|-------|
| GPS Module (TinyGPS++)         | RX → 2, TX → 3    | SoftwareSerial; GPS baud: 9600 |
| LCD – Biodegradable            | I2C SDA → A4, SCL → A5 | I2C address: 0x27, 16x2 |
| LCD – Non-Biodegradable        | I2C SDA → A4, SCL → A5 | I2C address: 0x25, 16x2 |
| Light Sensor (BH1750)          | I2C SDA → A4, SCL → A5 | Continuous high-resolution mode |
| Relay for LED                  | 8                 | Active LOW to turn ON |
| Servo – Biodegradable bin      | 11                | 0° open, 90° stop, 180° close |
| Servo – Non-Biodegradable bin  | 12                | 0° open, 90° stop, 180° close |

---

## 2. Ultrasonic Sensors

### Human Detection Sensors

| Sensor                          | Trig Pin | Echo Pin | Notes |
|---------------------------------|----------|----------|-------|
| Biodegradable bin human detect   | 4        | 5        | Detect human presence |
| Non-Biodegradable bin human detect | 6      | 7        | Detect human presence |

### Bin Full Detection Sensors

| Sensor                          | Trig Pin | Echo Pin | Notes |
|---------------------------------|----------|----------|-------|
| Biodegradable bin full           | A0       | A1       | Detect bin full (threshold: 100 cm) |
| Non-Biodegradable bin full       | A2       | A3       | Detect bin full (threshold: 100 cm) |

---

## 3. SIM800A GSM Module

| Function                  | Arduino Pin | Notes |
|----------------------------|------------|-------|
| SIM800A RX (Arduino TX)    | 8          | SoftwareSerial TX → SIM RX |
| SIM800A TX (Arduino RX)    | 7          | SoftwareSerial RX ← SIM TX |
| Power                      | External 3.7–4.2V battery or 5V 2A supply | DO NOT use Arduino 5V |
| GND                        | GND        | Common ground with Arduino |
| Antenna                    | SIM800A Antenna Pin | Required for network |

---

## 4. Operational Notes

1. **Servos**
   - 0° → Open, 90° → Stop, 180° → Close
   - Opening delay: 1 second, Closing delay: 1 second
   - Close delay after human leaves: 3 seconds

2. **Ultrasonic Sensors**
   - Human detection threshold: 50 cm
   - Full detection threshold: 100 cm
   - Moving average buffer of 5 samples for stable detection

3. **Relay / LED**
   - Relay is **active LOW**: `LOW` turns the LED ON, `HIGH` turns OFF

4. **SIM800A SMS**
   - Sends alert when bin is FULL
   - SMS phone number is manually configured: `+639567669410`
   - Ensure proper power (2A peak) and SIM card with active GSM service

5. **I2C Devices**
   - Both LCDs and BH1750 share the **same I2C bus** (SDA → A4, SCL → A5)
   - Unique I2C addresses: 0x27 (Biodegradable LCD), 0x25 (Non-Biodegradable LCD), BH1750 auto-address detection
