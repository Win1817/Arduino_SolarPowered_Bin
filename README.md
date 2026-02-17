# Smart Waste Bin System v3.1

An Arduino-based dual-bin waste management system with automatic fill detection, RFID access control, SMS alerts, GPS tracking, and ambient light sensing.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Wiring / Pin Map](#wiring--pin-map)
- [File Structure](#file-structure)
- [Configuration](#configuration)
- [How It Works](#how-it-works)
- [LCD Display Layout](#lcd-display-layout)
- [SMS Alerts](#sms-alerts)
- [RFID Access](#rfid-access)
- [Calibration](#calibration)
- [Serial Debug Output](#serial-debug-output)
- [Libraries Required](#libraries-required)
- [Upload Instructions](#upload-instructions)
- [Troubleshooting](#troubleshooting)

---

## Overview

The Smart Waste Bin System monitors two separate bins — **BIO** and **NON-BIO** — using ultrasonic sensors mounted on the lid pointing downward. When a bin fills up, the servo-controlled lock engages automatically, an SMS alert is sent, and the LCD displays the status. Authorized RFID cards can unlock the bins manually.

---

## Features

| Feature | Details |
|---|---|
| Dual bin monitoring | BIO and NON-BIO bins tracked independently |
| Ultrasonic fill detection | Median of 5 readings every 3 seconds |
| Confirmation filter | 3 consecutive readings required before state change |
| Hysteresis unlock | Bin must read >= 15cm before auto-unlocking |
| Servo lock/unlock | Physical lock engaged on full, released on empty or RFID |
| RFID access control | 2 authorized card UIDs, per-bin reader |
| SMS on full | Instant alert when bin confirmed full |
| SMS repeat | Up to 3 reminders per day (every 8 hours) while still full |
| SMS on RFID unlock | Notification sent when bin unlocked via card |
| Daily status report | SMS every 24 hours with both bin states |
| GPS location | Coordinates included in all SMS messages |
| Ambient light sensor | BH1750 (optional) controls LED relay when dark |
| LCD status display | 16x2 I2C LCD per bin showing label, %, bar, and distance |
| Debug serial output | Full status every 5 seconds via Serial Monitor |

---

## Hardware Requirements

| Component | Quantity | Notes |
|---|---|---|
| Arduino Uno | 1 | Main controller |
| HC-SR04 Ultrasonic Sensor | 2 | One per bin, lid-mounted |
| SG90 Servo Motor | 2 | One per bin lock mechanism |
| MFRC522 RFID Reader | 2 | One per bin |
| RFID Cards/Tags | 2+ | For authorized access |
| SIM800L GSM Module | 1 | SMS + signal strength |
| NEO-6M GPS Module | 1 | Location tracking |
| BH1750 Light Sensor | 1 | Optional ambient light |
| 16x2 I2C LCD Display | 2 | I2C addr 0x27 (BIO), 0x25 (NON-BIO) |
| 5V Relay Module | 1 | Controls LED strip |
| Active Buzzer | 1 | Audio feedback |
| LED Strip | 1 | Ambient/status lighting |
| Power Supply | 1 | 5V 2A minimum recommended |

---

## Wiring / Pin Map

```
Arduino Pin   Component
-----------   -----------------------------------------
D0            GPS TX      ** DISCONNECT BEFORE UPLOAD **
D1            GPS RX      ** DISCONNECT BEFORE UPLOAD **
D2            Buzzer (active)
D3            Relay module (LED strip)
D4            SIM800L RX  (SoftwareSerial)
D5            SIM800L TX  (SoftwareSerial)
D6            Servo BIO   (signal wire)
D7            Servo NON-BIO (signal wire)
D8            RFID BIO    SS (Slave Select)
D9            RFID        RST (shared between both RFID)
D10           RFID NON-BIO SS (Slave Select)
D11           SPI MOSI    (shared SPI bus)
D12           SPI MISO    (shared SPI bus)
D13           SPI SCK     (shared SPI bus)
A0            Ultrasonic BIO     TRIG
A1            Ultrasonic BIO     ECHO
A2            Ultrasonic NON-BIO TRIG
A3            Ultrasonic NON-BIO ECHO
A4            I2C SDA     (LCD1, LCD2, BH1750)
A5            I2C SCL     (LCD1, LCD2, BH1750)
```

### I2C Addresses

| Device | Address |
|---|---|
| LCD BIO (lcd1) | 0x27 |
| LCD NON-BIO (lcd2) | 0x25 |
| BH1750 Light Sensor | 0x23 (default) |

### Servo Wiring

| Wire Color | Connection |
|---|---|
| Brown / Black | GND |
| Red | 5V |
| Orange / Yellow | Signal pin (D6 or D7) |

---

## File Structure

```
smart_bin/
├── smart_bin.ino     Arduino IDE entry point (includes header only)
├── smart_bin.h       All configuration, pin definitions, declarations
└── smart_bin.cpp     Full implementation - setup(), loop(), all functions
```

All three files must be in a folder named `smart_bin` for Arduino IDE to compile correctly.

---

## Configuration

All user-adjustable settings are at the top of `smart_bin.h`:

```cpp
// Bin depth calibration (sensor top-down from lid)
#define BIN_DEPTH_CM    30    // sensor reading when bin is completely empty
#define FULL_CM         10    // sensor reading when bin is full (lock triggers)
#define EMPTY_CM        15    // sensor reading to auto-unlock (hysteresis)

// Ultrasonic
#define US_INTERVAL_MS  3000  // how often to read sensor (milliseconds)
#define CONFIRM_NEEDED  3     // consecutive matching reads before acting

// SMS schedule
#define SMS_INTERVAL_MS 28800000  // 8 hours between reminder SMS
#define MAX_SMS_PER_DAY 3         // maximum reminders per day
#define DAY_RESET_MS    86400000  // 24 hour reset period

// Light sensor
#define LUX_THRESHOLD   50.0  // below this lux = turn on LED relay
```

### RFID Card UIDs

Update in `smart_bin.h` with your scanned card UIDs:

```cpp
static const char AUTH_UID1[] = "43 FE B5 38";
static const char AUTH_UID2[] = "F3 37 B3 39";
```

To find your card UID, enable `DEBUG_MODE` and scan any card — the UID prints to Serial Monitor.

### Phone Number

```cpp
static const char PHONE[] = "+639567669410";
```

### Servo Angles

```cpp
static const int SERVO_LOCKED   = 90;   // angle that physically locks the bin
static const int SERVO_UNLOCKED = 0;    // angle that physically opens the bin
```

Test your specific servo/lock mechanism and adjust these values if needed.

---

## How It Works

### Fill Detection

The ultrasonic sensor is mounted on the underside of the lid, pointing down into the bin.

```
Lid (sensor here)
|
| <-- measures this distance
|
Trash level
|
Bin bottom
```

- When the bin is **empty**, the sensor reads ~30cm (far from bottom)
- When the bin is **full**, the sensor reads ~10cm (trash close to sensor)
- Smaller distance = more full

### Level Percentage Formula

```
Level % = (BIN_DEPTH_CM - distance) / (BIN_DEPTH_CM - FULL_CM) x 100
         = (30 - distance) / 20 x 100
```

| Distance | Level |
|---|---|
| 30cm | 0% (empty) |
| 25cm | 25% |
| 20cm | 50% |
| 15cm | 75% |
| 10cm | 100% (full) |

### State Machine

```
UNLOCKED
    |
    | bioDist <= FULL_CM (3x confirmed)
    v
  LOCKED  <-------+
    |              |
    | bioDist >= EMPTY_CM (3x confirmed)
    | OR authorized RFID card scanned
    v              |
UNLOCKED ----------+
```

### Confirmation Filter

To avoid false triggers from sensor noise, the system requires **3 consecutive readings** in agreement before changing state. A single spike will not lock or unlock the bin.

### Hysteresis

The lock threshold (10cm) and unlock threshold (15cm) are intentionally different. This prevents rapid toggling if trash level is right at the boundary.

---

## LCD Display Layout

Each bin has its own 16x2 LCD.

### Normal operation

```
Line 0:  BIO  WASTE   75%
Line 1:  [======  ]  15cm
```

```
Line 0:  NON-BIO      0%
Line 1:  [        ]  30cm
```

### When bin is full

```
Line 0:  BIO  WASTE  100%
Line 1:  [========] FULL
```

### On RFID unlock

```
Line 0:    BIO  WASTE
Line 1:     UNLOCKED
```

### On unauthorized card

```
Line 0:    UNAUTHORIZED
Line 1:    ACCESS DENIED
```

---

## SMS Alerts

All SMS messages include GPS coordinates when a fix is available, or `NoFix` when GPS has no signal.

### Alert types

| Trigger | Message |
|---|---|
| Bin confirmed full | `ALERT: BIO bin FULL! Level:100% GPS:lat,lng` |
| Reminder (8h later) | `REMINDER 2/3: BIO bin still FULL! GPS:lat,lng` |
| RFID unlock | `AUTH: BIO bin unlocked via RFID. GPS:lat,lng` |
| Daily report | `DAILY REPORT Bio:FULL NonBio:OK Sig:X GPS:lat,lng` |

### SMS schedule per bin

```
Bin goes full  ->  SMS 1 sent immediately
  + 8 hours   ->  SMS 2 "REMINDER 2/3" (if still full)
  + 8 hours   ->  SMS 3 "REMINDER 3/3" (if still full)
Bin emptied   ->  SMS counter resets, cycle starts fresh
```

---

## RFID Access

- Any authorized card scanned at the **BIO reader** unlocks only the **BIO bin**
- Any authorized card scanned at the **NON-BIO reader** unlocks only the **NON-BIO bin**
- Unauthorized cards trigger a rejection tone and `ACCESS DENIED` on screen
- On successful unlock, an SMS is sent with timestamp and GPS location

### Buzzer tones

| Event | Pattern |
|---|---|
| Bin full detected | 3 short beeps at 1500Hz |
| Authorized card | Two-tone rising beep (2000Hz then 2500Hz) |
| Unauthorized card | Single low beep at 400Hz |
| Bin auto-emptied | Two-tone falling beep (2500Hz then 2000Hz) |
| System boot ready | Two-tone rising beep |

---

## Calibration

### Step 1 — Find your bin depth

Empty the bin completely. Note the distance shown in Serial Monitor — this is your `BIN_DEPTH_CM`.

### Step 2 — Find your full threshold

Fill the bin to the level you want to trigger locking. Note the distance — this is your `FULL_CM`.

### Step 3 — Set hysteresis

Set `EMPTY_CM` to about 5cm more than `FULL_CM` to create a safe gap.

### Step 4 — Find servo angles

Upload this test sketch to find your lock/unlock angles:

```cpp
#include <Servo.h>
Servo s;
void setup() {
    Serial.begin(9600);
    s.attach(6);
    Serial.println("0 deg");   s.write(0);   delay(2000);
    Serial.println("90 deg");  s.write(90);  delay(2000);
    Serial.println("180 deg"); s.write(180); delay(2000);
}
void loop() {}
```

Update `SERVO_LOCKED` and `SERVO_UNLOCKED` in `smart_bin.h` with the correct angles.

---

## Serial Debug Output

With `DEBUG_MODE true`, the Serial Monitor (9600 baud) shows:

```
===========================
   SMART BIN v3.1 READY
===========================
Empty =  30cm (0%)
Full  =  10cm (100%)
Unlock>= 15cm
Usable: 20cm range
Confirm: 3x reads
Interval: 3s
===========================

BIO 28cm 10% open  | NON-BIO 15cm 75% open
Bio SMS today: 0  Non SMS today: 0
Lux: 45.83

Card: 43 FE B5 38
AUTH -> BIO UNLOCKED + SMS sent
[SMS] AUTH: BIO bin unlocked via RFID. GPS:NoFix
```

To disable debug output and save memory, set `DEBUG_MODE false` in `smart_bin.h`.

---

## Libraries Required

Install all libraries via **Arduino IDE > Sketch > Include Library > Manage Libraries**:

| Library | Author | Install name |
|---|---|---|
| TinyGPS++ | Mikal Hart | `TinyGPSPlus` |
| LiquidCrystal I2C | Frank de Brabander | `LiquidCrystal I2C` |
| BH1750 | Christopher Laws | `BH1750` |
| Servo | Arduino | built-in |
| MFRC522 | GithubCommunity | `MFRC522` |
| SoftwareSerial | Arduino | built-in |
| Wire | Arduino | built-in |
| SPI | Arduino | built-in |

---

## Upload Instructions

1. **Disconnect D0 and D1** (GPS wires) before uploading — they share the serial port
2. Place all three files (`smart_bin.ino`, `smart_bin.h`, `smart_bin.cpp`) in a folder named exactly `smart_bin`
3. Open `smart_bin.ino` in Arduino IDE — both `.h` and `.cpp` tabs will appear automatically
4. Select **Board: Arduino Uno** and the correct **Port**
5. Click **Upload**
6. Reconnect D0 and D1 after upload completes
7. Open Serial Monitor at **9600 baud** to verify startup

---

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---|---|---|
| Servo only wiggles, won't open | Servo thinks it is already at open angle | `servoForceOpen()` drives to locked first then open — check `SERVO_LOCKED` and `SERVO_UNLOCKED` values match your physical mechanism |
| Sensor always reads 999cm | Wiring issue or no echo received | Check TRIG/ECHO pins, ensure 5V power to sensor |
| Level always shows 0% | `BIN_DEPTH_CM` too small for actual bin | Measure real empty-bin reading and update `BIN_DEPTH_CM` |
| Bin locks/unlocks erratically | Sensor noise | Increase `CONFIRM_NEEDED` to 4 or 5 |
| SMS not sending | SIM800L not initialized | Check SIM card inserted, antenna connected, 4V power supply (SIM800L needs separate power) |
| RFID card not recognized | UID mismatch | Enable DEBUG_MODE, scan card, copy printed UID into `AUTH_UID1` or `AUTH_UID2` |
| LCD shows garbage | Wrong I2C address | Scan I2C bus — try addresses 0x27, 0x26, 0x25, 0x3F |
| GPS always shows NoFix | No satellite lock | Place near window, wait 1-2 minutes for first fix |
| Upload fails | D0/D1 connected during upload | Disconnect GPS wires from D0/D1 before uploading |
| Stray character compile errors | Non-ASCII characters in source | Ensure files are saved as plain ASCII, no special symbols in comments |
