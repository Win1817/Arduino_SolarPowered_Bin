# Smart Bin System - Complete Pin Assignments & Assembly Guide

## Table of Contents
1. [Pin Assignments](#pin-assignments)
2. [Component Wiring Instructions](#component-wiring-instructions)
3. [System Configuration](#system-configuration)
4. [Testing & Calibration](#testing--calibration)
5. [Troubleshooting](#troubleshooting)

---

## Pin Assignments

### Arduino Mega 2560 Pin Configuration

### Digital Pins

| Pin | Component | Function | Notes |
|-----|-----------|----------|-------|
| 2 | GPS Module | RX | SoftwareSerial |
| 3 | GPS Module | TX | SoftwareSerial |
| 4 | Weight Sensor Bio | DOUT | HX711 Data |
| 5 | RFID | RST | MFRC522 Reset |
| 6 | Weight Sensor Bio | SCK | HX711 Clock |
| 7 | Weight Sensor Non-Bio | DOUT | HX711 Data |
| 8 | Relay Module | LED Control | Ambient lighting |
| 9 | SIM800A | RX | SoftwareSerial |
| 10 | SIM800A | TX | SoftwareSerial |
| 11 | Servo Bio | Control | Lock mechanism |
| 12 | Servo Non-Bio | Control | Lock mechanism |

### SPI Pins (RFID MFRC522)

| Pin | Component | Function | Notes |
|-----|-----------|----------|-------|
| 50 | RFID | MISO | SPI Hardware |
| 51 | RFID | MOSI | SPI Hardware |
| 52 | RFID | SCK | SPI Hardware |
| 53 | RFID | SS/SDA | Chip Select |

### Analog Pins

| Pin | Component | Function | Notes |
|-----|-----------|----------|-------|
| A0 | Ultrasonic Bio | TRIG | Bin level detection |
| A1 | Ultrasonic Bio | ECHO | Bin level detection |
| A2 | Ultrasonic Non-Bio | TRIG | Bin level detection |
| A3 | Ultrasonic Non-Bio | ECHO | Bin level detection |
| A4 | Weight Sensor Non-Bio | SCK | HX711 Clock |

### I2C Pins (Default SDA/SCL)

| Pin | Component | Function | I2C Address | Notes |
|-----|-----------|----------|-------------|-------|
| 20 (SDA) | LCD1 | Data | 0x27 | Biodegradable display |
| 21 (SCL) | LCD1 | Clock | 0x27 | Biodegradable display |
| 20 (SDA) | LCD2 | Data | 0x25 | Non-Biodegradable display |
| 21 (SCL) | LCD2 | Clock | 0x25 | Non-Biodegradable display |
| 20 (SDA) | BH1750 | Data | 0x23 (default) | Light sensor |
| 21 (SCL) | BH1750 | Clock | 0x23 (default) | Light sensor |

### Power Connections

| Component | VCC | GND | Notes |
|-----------|-----|-----|-------|
| GPS Module | 5V | GND | |
| RFID MFRC522 | 3.3V | GND | **Important: Use 3.3V** |
| LCD1 (0x27) | 5V | GND | |
| LCD2 (0x25) | 5V | GND | |
| BH1750 | 5V or 3.3V | GND | |
| Ultrasonic Sensors | 5V | GND | All 4 sensors |
| Weight Sensors (HX711) | 5V | GND | Both Bio and Non-Bio |
| Servos | 5V (separate) | GND | Use external power if needed |
| Relay Module | 5V | GND | |
| SIM800A | 3.7-4.2V | GND | **Use separate power supply** |

## Component Specifications

### 1. GPS Module (NEO-6M or similar)
- **Baud Rate:** 9600
- **Communication:** UART via SoftwareSerial
- **Purpose:** Location tracking for bin management

### 2. RFID Reader (MFRC522)
- **Protocol:** SPI
- **Frequency:** 13.56 MHz
- **Purpose:** User authentication for bin access
- **Voltage:** 3.3V (Important!)

### 3. Weight Sensors (HX711 Load Cells)
- **Type:** 2x HX711 modules with load cells
- **Calibration Factor:** -7050.0 (adjust based on your load cell)
- **Threshold:** 5000g (5kg) for "FULL" status
- **Purpose:** Measure waste weight

### 4. Ultrasonic Sensors (HC-SR04 or similar)
- **Quantity:** 2 (one per bin)
- **Range:** 2cm - 400cm
- **Purpose:** Bin level detection
- **Full Threshold:** 100cm

### 5. LCD Displays (16x2 I2C)
- **Quantity:** 2
- **Addresses:** 0x27 (Bio), 0x25 (Non-Bio)
- **Purpose:** Real-time status display

### 6. Light Sensor (BH1750)
- **I2C Address:** 0x23 (default)
- **Threshold:** 50 lux
- **Purpose:** Ambient light control for LED

### 7. Servos (SG90 or MG90S)
- **Quantity:** 2
- **Lock Position:** 0°
- **Unlock Position:** 90°
- **Purpose:** Lock/unlock mechanism

### 8. SIM800A GSM Module
- **Baud Rate:** 9600
- **Power:** 3.7-4.2V (use separate LiPo battery or 5V-4V converter)
- **Purpose:** SMS alerts when bins are full

### 9. Relay Module
- **Control Pin:** Pin 8
- **Logic:** Active LOW
- **Purpose:** Ambient LED control

## System Features

1. **Smart Lock System (AUTOMATIC)**
   - Bins are **UNLOCKED by default**
   - Locks **ONLY trigger when bin is FULL**
   - Auto-unlocks when bin is emptied
   - No RFID required for access

2. **RFID User Logging**
   - Optional RFID scan for user tracking
   - Logs authorized and unknown users
   - Does NOT control bin access
   - Useful for waste accountability

3. **Dual Monitoring System**
   - Weight-based detection (HX711 load cells)
   - Distance-based detection (Ultrasonic sensors)
   - Bin marked "FULL" if **EITHER** threshold exceeded:
     - Weight ≥ 5000g (5kg) **OR**
     - Distance ≤ 100cm

4. **SMS Alerts**
   - Automatic SMS when bin is full
   - Includes weight information
   - Sent to: +639567669410

5. **Ambient Lighting**
   - Automatic LED control based on light level
   - Threshold: 50 lux
   - LED turns ON when dark

6. **Real-time Display**
   - Lock status (LOCKED/OPEN)
   - Current weight in grams
   - Full status indicator

7. **GPS Tracking**
   - Records bin location
   - Useful for fleet management

## Important Notes

⚠️ **Power Requirements:**
- RFID MFRC522 must use **3.3V** (not 5V!)
- SIM800A requires **separate power supply** (high current draw)
- Consider external power for servos if using heavy-duty models

⚠️ **Calibration Required:**
- Weight sensors: Adjust `BIO_CALIBRATION_FACTOR` and `NONBIO_CALIBRATION_FACTOR`
- Run tare operation with empty bins
- Test with known weights

⚠️ **RFID Setup:**
- Update `authorizedUIDs[]` array with your card UIDs
- Scan cards and copy UID from Serial Monitor
- Format: "AA BB CC DD" (uppercase, space-separated)

## Wiring Tips

1. Keep I2C wires short and twisted together
2. Use separate power rails for high-current devices (SIM800A, servos)
3. Add decoupling capacitors near power pins of sensitive components
4. Use shielded cable for ultrasonic sensors if experiencing noise
5. Ground all components to common ground

---

## Component Wiring Instructions

### 1. GPS Module (NEO-6M)

**Connections:**
```
GPS Module          Arduino Mega
----------          ------------
VCC        -------> 5V
GND        -------> GND
TX         -------> Pin 2 (RX)
RX         -------> Pin 3 (TX)
```

**Notes:**
- Use SoftwareSerial library
- Baud rate: 9600
- Position GPS antenna with clear sky view for best signal

---

### 2. RFID Reader (MFRC522)

**Connections:**
```
MFRC522            Arduino Mega
-------            ------------
SDA        -------> Pin 53 (SS)
SCK        -------> Pin 52 (SCK)
MOSI       -------> Pin 51 (MOSI)
MISO       -------> Pin 50 (MISO)
IRQ        -------> Not connected
GND        -------> GND
RST        -------> Pin 5
3.3V       -------> 3.3V (IMPORTANT!)
```

**⚠️ CRITICAL WARNING:**
- **MUST use 3.3V, NOT 5V!**
- Using 5V will damage the MFRC522 module
- Do not use Arduino's 5V pin for this module

**Setup Steps:**
1. Install MFRC522 library from Arduino Library Manager
2. Upload code and open Serial Monitor
3. Scan your RFID cards/tags
4. Copy the UID shown (format: "AA BB CC DD")
5. Update `authorizedUIDs[]` array in code with your UIDs

---

### 3. Weight Sensors (HX711 Load Cells)

#### Biodegradable Bin Weight Sensor

**Connections:**
```
HX711 Module       Arduino Mega
------------       ------------
VCC        -------> 5V
GND        -------> GND
DT (DOUT)  -------> Pin 4
SCK        -------> Pin 6
```

**Load Cell to HX711:**
```
Load Cell Wire     HX711
--------------     -----
Red        -------> E+
Black      -------> E-
White      -------> A-
Green      -------> A+
```

#### Non-Biodegradable Bin Weight Sensor

**Connections:**
```
HX711 Module       Arduino Mega
------------       ------------
VCC        -------> 5V
GND        -------> GND
DT (DOUT)  -------> Pin 7
SCK        -------> Pin A4
```

**Load Cell to HX711:** (Same wiring as above)

**Installation Tips:**
- Mount load cells at the base of each bin
- Ensure load cell is level and stable
- Keep wires away from motors and high-current lines
- Use 4-wire load cells for better accuracy

---

### 4. Ultrasonic Sensors (HC-SR04)

#### Biodegradable Bin Level Sensor

**Connections:**
```
HC-SR04            Arduino Mega
-------            ------------
VCC        -------> 5V
GND        -------> GND
TRIG       -------> Pin A0
ECHO       -------> Pin A1
```

#### Non-Biodegradable Bin Level Sensor

**Connections:**
```
HC-SR04            Arduino Mega
-------            ------------
VCC        -------> 5V
GND        -------> GND
TRIG       -------> Pin A2
ECHO       -------> Pin A3
```

**Mounting Instructions:**
1. Mount sensor at the **TOP** of each bin
2. Point sensor **downward** toward waste
3. Keep sensor away from bin walls (min 5cm clearance)
4. Ensure sensor is level and secure
5. Avoid condensation on sensor surface

**Distance Calculation:**
- Empty bin (sensor to bottom): ~300cm
- Full bin (sensor to waste): ~100cm or less
- System triggers "FULL" when distance ≤ 100cm

---

### 5. LCD Displays (16x2 I2C)

#### LCD1 - Biodegradable Bin Display

**Connections:**
```
LCD I2C            Arduino Mega
-------            ------------
VCC        -------> 5V
GND        -------> GND
SDA        -------> Pin 20 (SDA)
SCL        -------> Pin 21 (SCL)
```

**I2C Address:** 0x27

#### LCD2 - Non-Biodegradable Bin Display

**Connections:**
```
LCD I2C            Arduino Mega
-------            ------------
VCC        -------> 5V
GND        -------> GND
SDA        -------> Pin 20 (SDA)
SCL        -------> Pin 21 (SCL)
```

**I2C Address:** 0x25

**I2C Address Configuration:**
- Most I2C LCD modules have address jumpers on the back
- Default addresses: 0x27 or 0x3F
- To change address, solder jumper pads on PCB
- Use I2C scanner sketch to verify addresses

**Troubleshooting:**
- If LCD doesn't display, check I2C address
- Adjust contrast potentiometer on back of module
- Ensure both LCDs have different addresses

---

### 6. Light Sensor (BH1750)

**Connections:**
```
BH1750             Arduino Mega
------             ------------
VCC        -------> 5V (or 3.3V)
GND        -------> GND
SDA        -------> Pin 20 (SDA)
SCL        -------> Pin 21 (SCL)
ADDR       -------> GND (or VCC)
```

**I2C Address:** 
- 0x23 (when ADDR pin connected to GND)
- 0x5C (when ADDR pin connected to VCC)

**Function:**
- Measures ambient light in lux
- Controls relay for LED lighting
- LED turns ON when lux < 50 (dark environment)

---

### 7. Servo Motors (SG90 or MG90S)

#### Biodegradable Bin Servo (Lock Mechanism)

**Connections:**
```
Servo              Arduino Mega
-----              ------------
Brown/Black ------> GND
Red        -------> 5V (or external 5V)
Orange/Yellow ----> Pin 11
```

#### Non-Biodegradable Bin Servo (Lock Mechanism)

**Connections:**
```
Servo              Arduino Mega
-----              ------------
Brown/Black ------> GND
Red        -------> 5V (or external 5V)
Orange/Yellow ----> Pin 12
```

**Power Considerations:**
- For SG90 (small servos): Arduino 5V pin OK
- For MG90S or larger: Use **external 5V power supply**
- Share common GND between Arduino and external power
- Add 100µF capacitor across servo power pins

**Servo Positions:**
- **0°** = LOCKED (bin lid closed/blocked)
- **90°** = UNLOCKED (bin lid open/accessible)

**Mechanical Setup:**
1. Attach servo horn to bin lid mechanism
2. Test movement range (0° to 90°)
3. Adjust mechanical linkage as needed
4. Secure servo with screws or hot glue

---

### 8. SIM800A GSM Module

**Connections:**
```
SIM800A            Arduino Mega
-------            ------------
RXD        -------> Pin 10 (TX)
TXD        -------> Pin 9 (RX)
GND        -------> GND (common ground)
VCC        -------> External 5V supply
```

**⚠️ CRITICAL POWER REQUIREMENTS:**
- **DO NOT power from Arduino 5V pin!**
- Requires 2A current during transmission
- Use one of these options:
  1. Dedicated 5V 2A power adapter
  2. 3.7V LiPo battery (recommended)
  3. 5V to 4.2V buck converter from external supply

**SIM Card Setup:**
1. Insert activated SIM card (PIN disabled)
2. Ensure SIM has credit for SMS
3. Update `phoneNumber[]` in code with recipient number
4. Include country code (e.g., "+639567669410")

**Antenna:**
- Connect GSM antenna to SIM800A
- Position antenna vertically for best signal

**Testing:**
1. Power on SIM800A
2. Wait for network LED to blink (every 3 seconds = connected)
3. Check Serial Monitor for "SMS SENT" messages

---

### 9. Relay Module (for LED Control)

**Connections:**
```
Relay Module       Arduino Mega
------------       ------------
VCC        -------> 5V
GND        -------> GND
IN         -------> Pin 8
COM        -------> LED Power Supply (+)
NO         -------> LED Strip (+)
```

**LED Strip Connections:**
```
LED Strip          Power/Relay
---------          -----------
(+)        -------> Relay NO
(-)        -------> Power Supply (-)
```

**Logic:**
- Active LOW relay (common for modules with optocouplers)
- `digitalWrite(8, LOW)` = LED ON
- `digitalWrite(8, HIGH)` = LED OFF
- System turns LED ON when ambient light < 50 lux

**LED Specifications:**
- Use 12V LED strips with separate 12V power supply
- Do NOT connect LED directly to Arduino pins
- Relay rated for LED current (typically 10A max)

---

## System Configuration

### Required Arduino Libraries

Install these libraries from Arduino Library Manager:

1. **TinyGPS++** - GPS parsing
2. **LiquidCrystal_I2C** - I2C LCD control
3. **BH1750** - Light sensor
4. **Servo** - Servo motor control (built-in)
5. **MFRC522** - RFID reader
6. **HX711** - Weight sensor (load cell amplifier)
7. **Wire** - I2C communication (built-in)
8. **SPI** - SPI communication (built-in)
9. **SoftwareSerial** - Software serial (built-in)

### Code Configuration

**1. Phone Number (SMS Alerts):**
```cpp
const char phoneNumber[] = "+639567669410"; // Your number with country code
```

**2. Weight Thresholds:**
```cpp
const float BIO_WEIGHT_THRESHOLD = 5000.0;    // 5 kg in grams
const float NONBIO_WEIGHT_THRESHOLD = 5000.0; // 5 kg in grams
```

**3. Distance Threshold:**
```cpp
const long FULL_THRESHOLD = 100; // cm - bin considered full
```

**4. Light Threshold:**
```cpp
const float LUX_THRESHOLD = 50.0; // LED turns on below this
```

**5. RFID Authorized Users:**
```cpp
String authorizedUIDs[] = {
  "AA BB CC DD",  // Replace with your card UID
  "11 22 33 44"   // Add more UIDs as needed
};
const int numAuthorizedUIDs = 2; // Update count
```

---

## Testing & Calibration

### 1. Weight Sensor Calibration

**Step 1: Tare (Zero) the Scale**
```cpp
// In setup(), this line zeros the scale:
scaleBio.tare();
scaleNonBio.tare();
```

**Step 2: Find Calibration Factor**
1. Upload test sketch (read raw values)
2. Place known weight (e.g., 1 kg)
3. Calculate: `calibration_factor = raw_value / known_weight`
4. Update in code:
```cpp
const float BIO_CALIBRATION_FACTOR = -7050.0;     // Adjust this
const float NONBIO_CALIBRATION_FACTOR = -7050.0;  // Adjust this
```

**Step 3: Verify**
1. Place 1 kg → Should read ~1000g
2. Place 2 kg → Should read ~2000g
3. Fine-tune calibration factor if needed

### 2. Ultrasonic Sensor Testing

**Verify Distance Readings:**
1. Open Serial Monitor (9600 baud)
2. Check debug output:
```
BioDist: 250cm | NonBioDist: 180cm
```
3. Place hand above sensor → distance should decrease
4. Remove hand → distance should increase

**Adjust Threshold if Needed:**
- If bin triggers "FULL" too early → increase threshold
- If bin doesn't detect full → decrease threshold

### 3. Servo Lock Testing

**Test Lock Mechanism:**
1. Watch servo on startup → should be at 90° (UNLOCKED)
2. Manually trigger "FULL" condition → servo moves to 0° (LOCKED)
3. Remove full condition → servo returns to 90° (UNLOCKED)

**Adjust Servo Angles if Needed:**
```cpp
const int SERVO_LOCKED = 0;    // Change if needed (0-180)
const int SERVO_UNLOCKED = 90; // Change if needed (0-180)
```

### 4. RFID User Logging Test

**Scan Test Cards:**
1. Hold RFID card near reader
2. Check Serial Monitor for UID
3. LCD should show "User logged" or "Unknown user"
4. Copy UID and add to `authorizedUIDs[]` array

### 5. SMS Alert Testing

**Send Test SMS:**
1. Manually set bin to FULL (add weight or cover sensor)
2. Wait for SMS (may take 10-30 seconds)
3. Check phone for message
4. Verify message content includes weight

**Troubleshooting SMS:**
- No SMS? Check SIM800A power and antenna
- Check SIM card has credit
- Verify phone number format includes country code
- Check Serial Monitor for "SMS SENT" confirmation

### 6. LCD Display Check

**Verify Display Output:**
```
Row 1: "Biodegradable" or "Non-Biodegrad."
Row 2: "OPEN 1234g" or "LOCKED 5678g FULL"
```

**If Display Not Working:**
- Run I2C scanner to find address
- Adjust contrast potentiometer
- Check wiring (SDA/SCL)

---

## Troubleshooting

### GPS Not Working
- Check antenna connection
- Ensure clear view of sky
- Verify baud rate (9600)
- Wait 1-2 minutes for fix

### RFID Not Reading Cards
- Check 3.3V power (NOT 5V!)
- Verify SPI connections
- Ensure card is compatible (13.56 MHz)
- Hold card flat against reader

### Weight Sensor Reading Zero
- Check load cell wiring (color coding)
- Verify HX711 connections
- Run tare operation
- Adjust calibration factor

### Ultrasonic Sensor Showing "---"
- Check power connections
- Verify TRIG/ECHO pins
- Ensure sensor not blocked
- Check timeout in code

### Servos Not Moving
- Check power supply (external for large servos)
- Verify signal wire connection
- Test with servo sweep example
- Check servo positions in code

### SMS Not Sending
- Verify SIM800A has separate power (2A)
- Check SIM card insertion and credit
- Verify antenna connection
- Check network LED (should blink every 3s)
- Update phone number format

### LCD Not Displaying
- Run I2C scanner sketch
- Check I2C address in code
- Verify SDA/SCL connections
- Adjust contrast pot

### LED Not Turning On
- Check relay connections
- Verify LED power supply
- Test relay manually
- Check light threshold value

---

## System Operation Flow

### Normal Operation:

```
1. System starts → Bins UNLOCKED
   ↓
2. User approaches bin
   ↓
3. [Optional] Scan RFID card → User logged
   ↓
4. User disposes waste → Weight increases
   ↓
5. System monitors continuously:
   - Weight sensor: Current weight
   - Ultrasonic: Fill level
   - Display: Shows status
   ↓
6. When FULL (weight ≥ 5kg OR distance ≤ 100cm):
   - Servo LOCKS (moves to 0°)
   - LCD shows "LOCKED ... FULL"
   - SMS sent to admin
   ↓
7. Admin empties bin:
   - Weight decreases
   - Distance increases
   ↓
8. System detects not full:
   - Servo UNLOCKS (moves to 90°)
   - LCD shows "OPEN"
   - Ready for use again
```

### Lock Logic Summary:

| Condition | Bio Bin | Non-Bio Bin |
|-----------|---------|-------------|
| Weight < 5kg AND Distance > 100cm | UNLOCKED | UNLOCKED |
| Weight ≥ 5kg OR Distance ≤ 100cm | LOCKED | LOCKED |

**Note:** Each bin operates independently. One bin can be LOCKED while the other is UNLOCKED.

---

## Safety & Maintenance

### Safety Precautions:
1. ⚠️ **Never connect 5V to 3.3V components** (RFID, SIM800A power)
2. ⚠️ **Use separate power for SIM800A** (minimum 2A capability)
3. ⚠️ **Keep water away from electronics**
4. ⚠️ **Secure all wiring** to prevent shorts
5. ⚠️ **Test servos before final installation**

### Regular Maintenance:
- Clean ultrasonic sensor surfaces weekly
- Check load cell mounting monthly
- Verify GPS antenna position
- Test SMS functionality monthly
- Inspect wiring for damage
- Clean RFID reader surface

### Recommended Upgrades:
- Add waterproof enclosure for electronics
- Use terminal blocks for easier maintenance
- Add status LEDs for visual feedback
- Implement data logging to SD card
- Add battery backup for SIM800A

---

## Support & Resources

### Useful Links:
- TinyGPS++ Documentation: http://arduiniana.org/libraries/tinygpsplus/
- HX711 Calibration Guide: https://learn.sparkfun.com/tutorials/load-cell-amplifier-hx711-breakout-hookup-guide
- MFRC522 Library: https://github.com/miguelbalboa/rfid
- SIM800A AT Commands: https://www.elecrow.com/wiki/index.php?title=SIM800_Series_AT_Command_Manual

### Common Arduino Mega Pin References:
- SPI: 50 (MISO), 51 (MOSI), 52 (SCK), 53 (SS)
- I2C: 20 (SDA), 21 (SCL)
- PWM: 2-13, 44-46
- Analog: A0-A15
