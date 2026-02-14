# Smart Waste Bin System - Arduino Uno Edition

## Complete Assembly Guide & Documentation

---

## Table of Contents
1. [System Overview](#system-overview)
2. [Component List](#component-list)
3. [Pin Assignments](#pin-assignments)
4. [Wiring Instructions](#wiring-instructions)
5. [Power System](#power-system)
6. [Software Setup](#software-setup)
7. [Calibration & Testing](#calibration--testing)
8. [System Operation](#system-operation)
9. [Troubleshooting](#troubleshooting)

---

## System Overview

### Features
✅ **Automatic Smart Lock System**
- Bins unlocked by default
- Auto-locks when bin reaches full threshold
- Auto-unlocks when bin is emptied

✅ **Dual RFID User Tracking**
- Separate RFID reader for each bin
- Tracks user access per bin type
- Authorized user verification
- Audio feedback (buzzer)

✅ **Ultrasonic Level Detection**
- Real-time bin fill monitoring
- Stable readings with moving average
- Customizable full threshold

✅ **SMS Alerts**
- Automatic notification when bins are full
- GPS location included in alerts
- Sent to designated phone number

✅ **Ambient LED Control**
- Automatic LED strip activation in darkness
- Light sensor threshold: 50 lux
- Relay-controlled LED strip

✅ **Real-time LCD Display**
- Separate display for each bin
- Shows: Lock status, distance, full indicator
- User feedback messages

✅ **GPS Tracking**
- Location data for bin management
- Included in SMS alerts

---

## Component List

### Required Components

| Qty | Component | Specification | Purpose |
|-----|-----------|---------------|---------|
| 1 | Arduino Uno | Rev3 or compatible | Main controller |
| 2 | Ultrasonic Sensor | HC-SR04 | Bin level detection |
| 2 | LCD Display | 16x2 I2C (0x27, 0x25) | Status display |
| 2 | Servo Motor | SG90 or MG90S | Lock mechanism |
| 2 | RFID Module | MFRC522 (13.56MHz) | User identification |
| 2 | RFID Tags/Cards | 13.56MHz compatible | User access cards |
| 1 | Light Sensor | BH1750 | Ambient light detection |
| 1 | GPS Module | NEO-6M or compatible | Location tracking |
| 1 | GSM Module | SIM800A | SMS notifications |
| 1 | Buzzer | Active or Passive | Audio feedback |
| 1 | Relay Module | 5V, Single Channel | LED control |
| 1 | LED Strip | 12V (optional) | Ambient lighting |
| 1 | Battery | 12V rechargeable | Main power source |
| 1 | BMS | 12V Battery Management | Battery protection |
| 1 | Buck Converter | 12V to 5V, 3A+ | Arduino/module power |
| 1 | Breadboard | Full size | Prototyping |
| - | Jumper Wires | Male-Male, Male-Female | Connections |

---

## Pin Assignments

### Arduino Uno Pin Configuration

#### Digital Pins

| Pin | Component | Function | Notes |
|-----|-----------|----------|-------|
| 0 | GPS Module | RX | Hardware Serial (conflicts with USB) |
| 1 | GPS Module | TX | Hardware Serial (conflicts with USB) |
| 2 | Buzzer | Audio Output | Active or Passive buzzer |
| 3 | Relay Module | LED Control | PWM capable (not required) |
| 4 | SIM800A | RX | SoftwareSerial |
| 5 | SIM800A | TX | SoftwareSerial |
| 6 | Servo Bio | Control | PWM for smooth movement |
| 7 | Servo Non-Bio | Control | Standard digital pin |
| 8 | RFID Bio | SS/SDA | Chip Select |
| 9 | RFID Shared | RST | Reset for both RFID modules |
| 10 | RFID Non-Bio | SS/SDA | Chip Select |
| 11 | RFID (Both) | MOSI | SPI Hardware |
| 12 | RFID (Both) | MISO | SPI Hardware |
| 13 | RFID (Both) | SCK | SPI Hardware |

#### Analog Pins

| Pin | Component | Function | Notes |
|-----|-----------|----------|-------|
| A0 | Ultrasonic Bio | TRIG | Level detection trigger |
| A1 | Ultrasonic Bio | ECHO | Level detection echo |
| A2 | Ultrasonic Non-Bio | TRIG | Level detection trigger |
| A3 | Ultrasonic Non-Bio | ECHO | Level detection echo |
| A4 | I2C | SDA | LCD1, LCD2, BH1750 |
| A5 | I2C | SCL | LCD1, LCD2, BH1750 |

### I2C Device Addresses

| Device | Address | Purpose |
|--------|---------|---------|
| LCD1 (Biodegradable) | 0x27 | Primary display |
| LCD2 (Non-Biodegradable) | 0x25 | Secondary display |
| BH1750 Light Sensor | 0x23 | Light level monitoring |

---

## Wiring Instructions

### ⚠️ IMPORTANT NOTES BEFORE WIRING

1. **GPS uses Hardware Serial (Pins 0, 1)**
   - Serial Monitor will NOT work when GPS is connected
   - For debugging, temporarily disconnect GPS TX/RX
   - Reconnect GPS after uploading code

2. **RFID modules share RST pin**
   - Both RFID modules connected to pin 9 for reset
   - Separate SS pins (8 and 10) for chip select
   - This saves one Arduino pin

3. **Power Requirements**
   - SIM800A requires 2A current (separate power!)
   - Use 12V battery → BMS → Buck Converter → 5V
   - Never power SIM800A from Arduino 5V pin

---

### 1. GPS Module (NEO-6M)

```
GPS Module      Arduino Uno
----------      -----------
VCC        →    5V
GND        →    GND
TX         →    Pin 0 (RX)
RX         →    Pin 1 (TX)
```

**⚠️ WARNING:** 
- Disconnect GPS when uploading code or using Serial Monitor
- GPS uses Hardware Serial shared with USB
- For debugging, comment out GPS code temporarily

---

### 2. Dual RFID Modules (MFRC522)

#### Biodegradable Bin RFID

```
MFRC522         Arduino Uno
-------         -----------
SDA        →    Pin 8 (SS)
SCK        →    Pin 13 (SCK)
MOSI       →    Pin 11 (MOSI)
MISO       →    Pin 12 (MISO)
IRQ        →    Not connected
GND        →    GND
RST        →    Pin 9 (SHARED)
3.3V       →    3.3V ⚠️ IMPORTANT!
```

#### Non-Biodegradable Bin RFID

```
MFRC522         Arduino Uno
-------         -----------
SDA        →    Pin 10 (SS)
SCK        →    Pin 13 (SCK) - SHARED
MOSI       →    Pin 11 (MOSI) - SHARED
MISO       →    Pin 12 (MISO) - SHARED
IRQ        →    Not connected
GND        →    GND
RST        →    Pin 9 (SHARED)
3.3V       →    3.3V ⚠️ IMPORTANT!
```

**⚠️ CRITICAL:**
- **MUST use 3.3V, NOT 5V!**
- Using 5V will damage RFID modules
- Both RFID modules share SPI bus (MOSI, MISO, SCK)
- Both share RST pin (Pin 9)
- Each has unique SS pin (8 and 10)

---

### 3. Ultrasonic Sensors (HC-SR04)

#### Biodegradable Bin Sensor

```
HC-SR04         Arduino Uno
-------         -----------
VCC        →    5V
GND        →    GND
TRIG       →    Pin A0
ECHO       →    Pin A1
```

#### Non-Biodegradable Bin Sensor

```
HC-SR04         Arduino Uno
-------         -----------
VCC        →    5V
GND        →    GND
TRIG       →    Pin A2
ECHO       →    Pin A3
```

**Mounting:**
- Install at **TOP** of bin, pointing **DOWNWARD**
- Keep 5cm+ clearance from bin walls
- Secure firmly to prevent movement
- Protect from moisture/condensation

---

### 4. LCD Displays (16x2 I2C)

#### LCD1 - Biodegradable Bin

```
LCD I2C         Arduino Uno
-------         -----------
VCC        →    5V
GND        →    GND
SDA        →    Pin A4 (SDA)
SCL        →    Pin A5 (SCL)
```
**I2C Address:** 0x27

#### LCD2 - Non-Biodegradable Bin

```
LCD I2C         Arduino Uno
-------         -----------
VCC        →    5V
GND        →    GND
SDA        →    Pin A4 (SDA) - SHARED
SCL        →    Pin A5 (SCL) - SHARED
```
**I2C Address:** 0x25

**I2C Address Configuration:**
- Check address jumpers on back of LCD module
- Use I2C scanner sketch to verify addresses
- Ensure both LCDs have different addresses

---

### 5. Light Sensor (BH1750)

```
BH1750          Arduino Uno
------          -----------
VCC        →    5V (or 3.3V)
GND        →    GND
SDA        →    Pin A4 (SDA) - SHARED
SCL        →    Pin A5 (SCL) - SHARED
ADDR       →    GND (for 0x23 address)
```

**I2C Address:** 0x23 (ADDR to GND)

---

### 6. Servo Motors

#### Biodegradable Bin Servo

```
Servo           Arduino Uno
-----           -----------
Brown/Black →   GND
Red         →   5V (or external 5V)
Orange      →   Pin 6
```

#### Non-Biodegradable Bin Servo

```
Servo           Arduino Uno
-----           -----------
Brown/Black →   GND
Red         →   5V (or external 5V)
Orange      →   Pin 7
```

**Servo Positions:**
- 0° = LOCKED (bin blocked)
- 90° = UNLOCKED (bin accessible)

**Power Considerations:**
- SG90 (small): Can use Arduino 5V
- MG90S or larger: Use external 5V power supply
- Add 100µF capacitor across power pins

---

### 7. SIM800A GSM Module

```
SIM800A         Arduino Uno
-------         -----------
RXD        →    Pin 5 (TX)
TXD        →    Pin 4 (RX)
GND        →    Common GND
VCC        →    External 4.2V (NOT Arduino!)
```

**⚠️ CRITICAL POWER:**
- **DO NOT power from Arduino 5V pin!**
- Requires 2A during transmission
- Use 3.7V LiPo battery OR
- Use separate 5V→4.2V buck converter

**SIM Card Setup:**
1. Insert activated SIM card (disable PIN)
2. Ensure SIM has credit for SMS
3. Connect GSM antenna
4. Update phone number in code

---

### 8. Buzzer

```
Buzzer          Arduino Uno
------          -----------
Positive   →    Pin 2
Negative   →    GND
```

**Buzzer Types:**
- Active buzzer: Simpler, fixed frequency
- Passive buzzer: Requires tone() function, variable frequency

**Buzzer Patterns:**
- Success: Two ascending tones
- Error: Two low tones
- Full: Three short beeps
- Short: Single quick beep

---

### 9. Relay Module (LED Control)

```
Relay Module    Arduino Uno
------------    -----------
VCC        →    5V
GND        →    GND
IN         →    Pin 3
COM        →    LED Power (+)
NO         →    LED Strip (+)
```

**LED Strip Connection:**
```
LED Strip       Power/Relay
---------       -----------
(+)        →    Relay NO
(-)        →    Power Supply (-)
```

**Logic:**
- Active LOW relay
- Pin 3 LOW = LED ON
- Pin 3 HIGH = LED OFF

---

## Power System

### System Architecture

```
12V Battery
    ↓
Battery Management System (BMS)
    ↓
12V Output
    ├→ LED Strip (12V)
    └→ 5V Buck Converter (3A+)
        ├→ Arduino Uno (5V)
        ├→ GPS Module (5V)
        ├→ RFID Modules (3.3V via Arduino)
        ├→ LCD Displays (5V)
        ├→ Ultrasonic Sensors (5V)
        ├→ Servo Motors (5V)
        ├→ BH1750 (5V)
        ├→ Buzzer (5V)
        ├→ Relay Module (5V)
        └→ SIM800A (4.2V via separate converter)
```

### Power Requirements

| Component | Voltage | Current | Notes |
|-----------|---------|---------|-------|
| Arduino Uno | 5V | 50mA | Via buck converter |
| GPS | 5V | 50mA | Continuous |
| RFID (x2) | 3.3V | 26mA each | Via Arduino regulator |
| LCD (x2) | 5V | 20mA each | With backlight |
| Ultrasonic (x2) | 5V | 15mA each | When active |
| Servo (x2) | 5V | 100-500mA | Peak during movement |
| BH1750 | 5V | 0.12mA | Very low |
| Buzzer | 5V | 30mA | When active |
| Relay | 5V | 70mA | Coil |
| SIM800A | 3.7-4.2V | 2A | **Peak during transmission!** |
| LED Strip | 12V | Varies | Depends on length |

**Total Estimated:** ~3-4A @ 5V (with SIM800A separate)

### Recommended Power Supply

**Option 1: Battery System (Recommended)**
- 12V 7Ah+ rechargeable battery
- 12V BMS (10A rating)
- 12V→5V buck converter (5A rating)
- Separate 5V→4.2V converter for SIM800A (3A)

**Option 2: AC Adapter**
- 12V 5A wall adapter
- Same buck converters as above

---

## Software Setup

### Required Arduino Libraries

Install from Arduino Library Manager:

1. **TinyGPS++** (by Mikal Hart)
2. **LiquidCrystal_I2C** (by Frank de Brabander)
3. **BH1750** (by Christopher Laws)
4. **MFRC522** (by GithubCommunity)
5. **Servo** (built-in)
6. **Wire** (built-in)
7. **SPI** (built-in)
8. **SoftwareSerial** (built-in)

### Configuration Settings

**1. Phone Number (SMS Alerts)**
```cpp
const char phoneNumber[] = "+639567669410"; // Include country code
```

**2. Distance Threshold**
```cpp
const long FULL_THRESHOLD = 10; // cm - closer = bin fuller
```

**3. Light Threshold**
```cpp
const float LUX_THRESHOLD = 50.0; // LED turns on when lux < 50
```

**4. RFID Authorized Users**
```cpp
String authorizedUIDs[] = {
  "AA BB CC DD",  // Replace with actual UID
  "11 22 33 44"   // Add more as needed
};
const int numAuthorizedUIDs = 2; // Update count
```

**5. Servo Positions**
```cpp
const int SERVO_LOCKED = 0;    // Adjust if needed (0-180)
const int SERVO_UNLOCKED = 90; // Adjust if needed (0-180)
```

### Getting RFID UIDs

1. Upload code to Arduino
2. Disconnect GPS temporarily
3. Open Serial Monitor (9600 baud)
4. Scan your RFID cards
5. Copy UID shown (format: "AA BB CC DD")
6. Update `authorizedUIDs[]` array
7. Reconnect GPS

---

## Calibration & Testing

### 1. Ultrasonic Sensor Testing

**Empty Bin Test:**
1. Mount sensor at top of empty bin
2. Power on system
3. Disconnect GPS RX/TX
4. Open Serial Monitor (9600 baud) - Will show debug info in actual deployment without GPS disconnect
5. Check distance reading (should be ~200-300cm for empty bin)

**Full Bin Test:**
1. Place object/hand 10cm from sensor
2. Distance should read ~10cm or less
3. System should trigger "FULL" status
4. Servo should lock
5. Buzzer should beep (3 times)
6. LCD should show "LOCKED ... FULL"

**Adjust Threshold:**
```cpp
const long FULL_THRESHOLD = 10; // Decrease for earlier trigger, increase for later
```

---

### 2. RFID User Testing

**Scan Test:**
1. Hold RFID card/tag near Biodegradable bin reader
2. Buzzer should beep twice (success) or once (error)
3. LCD1 should show "User: OK" or "Unknown user"
4. Repeat for Non-Biodegradable bin reader

**Add New Users:**
1. Scan unknown card
2. Note UID from LCD or Serial Monitor
3. Add UID to `authorizedUIDs[]` array
4. Re-upload code
5. Test again - should now show "User: OK"

---

### 3. Servo Lock Testing

**Lock/Unlock Test:**
1. Cover ultrasonic sensor (simulate full bin)
2. Servo should move to 0° (LOCKED)
3. Buzzer beeps 3 times
4. LCD shows "LOCKED"
5. Remove hand from sensor
6. Servo returns to 90° (UNLOCKED)
7. Buzzer beeps twice (success)
8. LCD shows "OPEN"

**Adjust Servo Angles:**
```cpp
const int SERVO_LOCKED = 0;    // Try 10, 20, etc.
const int SERVO_UNLOCKED = 90; // Try 80, 100, etc.
```

---

### 4. SMS Alert Testing

**Test Message:**
1. Trigger full condition (cover sensor)
2. Wait 10-30 seconds
3. Check phone for SMS
4. Message should include:
   - Bin type (Biodegradable or Non-Biodegradable)
   - Distance reading
   - GPS coordinates (if available)

**Troubleshooting SMS:**
- Check SIM800A power (separate 4.2V supply)
- Verify SIM card has credit
- Ensure antenna is connected
- Check network LED (blinks every 3 seconds when connected)
- Verify phone number format: "+[country code][number]"

---

### 5. LCD Display Check

**Expected Display:**

**Biodegradable LCD (0x27):**
```
Row 1: Biodegradable
Row 2: OPEN   25cm
```

or when full:
```
Row 1: Biodegradable
Row 2: LOCKED 8cm FULL
```

**If LCD not working:**
1. Check I2C address with scanner
2. Verify SDA/SCL connections
3. Adjust contrast potentiometer on back
4. Ensure both LCDs have different addresses

---

### 6. LED Strip & Light Sensor

**Test Automatic Control:**
1. Cover light sensor (simulate darkness)
2. LED strip should turn ON
3. Uncover sensor (simulate daylight)
4. LED strip should turn OFF

**Adjust Light Threshold:**
```cpp
const float LUX_THRESHOLD = 50.0; // Increase for later LED turn-on
```

---

### 7. GPS Testing

**Note:** GPS testing requires disconnecting from Serial Monitor

**Verify GPS Lock:**
1. Upload code with GPS connected
2. Disconnect USB
3. Power via battery/external supply
4. Wait 1-2 minutes outdoors
5. GPS should acquire satellite lock
6. Location data included in SMS alerts

**GPS Not Working:**
- Ensure clear view of sky
- Wait longer for satellite acquisition (2-5 minutes)
- Check antenna connection
- Verify baud rate (9600)

---

## System Operation

### Normal Operation Flow

```
STEP 1: System Startup
    ↓
Bins are UNLOCKED (servos at 90°)
LCD shows "Ready - OPEN"
Buzzer gives startup beep
    ↓
STEP 2: User Approaches Bin
    ↓
[Optional] User scans RFID card
    ├→ Authorized: Buzzer beeps twice, LCD shows "User: OK"
    └→ Unknown: Buzzer beeps once (low), LCD shows "Unknown user"
    ↓
STEP 3: User Disposes Waste
    ↓
Ultrasonic sensor monitors fill level
LCD shows current distance
    ↓
STEP 4: Bin Fills Up
    ↓
Distance ≤ 10cm (FULL threshold reached)
    ├→ Servo LOCKS (moves to 0°)
    ├→ Buzzer beeps 3 times
    ├→ LCD shows "LOCKED ... FULL"
    └→ SMS sent to admin with GPS location
    ↓
STEP 5: Bin Remains Locked
    ↓
User cannot access full bin
System continues monitoring
    ↓
STEP 6: Admin Empties Bin
    ↓
Distance increases (waste removed)
Distance > 10cm (no longer full)
    ├→ Servo UNLOCKS (moves to 90°)
    ├→ Buzzer beeps twice (success)
    ├→ LCD shows "OPEN"
    └→ Ready for use again
```

### Lock Logic Table

| Distance | Bio Bin | Non-Bio Bin |
|----------|---------|-------------|
| > 10cm | UNLOCKED | UNLOCKED |
| ≤ 10cm | LOCKED | LOCKED |

**Note:** Each bin operates independently

---

## Troubleshooting

### GPS Issues

**Problem:** GPS not acquiring fix
- **Solution:** 
  - Position antenna with clear sky view
  - Wait 2-5 minutes outdoors
  - Check antenna connection
  - Verify baud rate (9600)

**Problem:** Serial Monitor not working
- **Solution:** 
  - Disconnect GPS TX (Pin 1) when debugging
  - Comment out GPS code temporarily
  - Or accept that Serial Monitor won't work with GPS

---

### RFID Issues

**Problem:** Cards not detected
- **Solution:**
  - Verify 3.3V power (NOT 5V!)
  - Check SPI connections (11, 12, 13)
  - Ensure SS pins correct (8 and 10)
  - Verify RST pin (9) connected to both
  - Hold card flat against reader
  - Check card compatibility (13.56MHz)

**Problem:** Only one RFID works
- **Solution:**
  - Check SS pins are different (8 vs 10)
  - Verify both RFID modules share RST pin 9
  - Ensure SPI bus shared correctly
  - Test each module individually

---

### Ultrasonic Sensor Issues

**Problem:** Distance shows MAX (300+cm) always
- **Solution:**
  - Check TRIG/ECHO pin connections
  - Verify 5V power
  - Ensure sensor not blocked
  - Check timeout in code (50000µs)
  - Test with known distance

**Problem:** Erratic readings
- **Solution:**
  - Secure sensor firmly
  - Keep away from bin walls (5cm+ clearance)
  - Avoid condensation on sensor
  - Increase DIST_SAMPLES if needed

---

### Servo Issues

**Problem:** Servos not moving
- **Solution:**
  - Check power supply (external for MG90S)
  - Verify signal wire (Pin 6 and 7)
  - Test with servo sweep example
  - Ensure servo.attach() called
  - Check servo angle limits (0-180)

**Problem:** Servo jitters
- **Solution:**
  - Use external 5V power supply
  - Add 100µF capacitor across power
  - Check for power supply noise
  - Reduce servo speed in code

---

### SMS/SIM800A Issues

**Problem:** No SMS sent
- **Solution:**
  - Verify separate power supply (4.2V, 2A)
  - Check SIM card inserted and has credit
  - Verify antenna connected
  - Check network LED (should blink every 3s)
  - Update phone number format (+country code)
  - Check SoftwareSerial pins (RX=4, TX=5)

**Problem:** Module resets randomly
- **Solution:**
  - Insufficient power - use separate supply
  - Add 1000µF capacitor near SIM800A
  - Check battery/power supply capacity

---

### LCD Issues

**Problem:** LCD not displaying
- **Solution:**
  - Run I2C scanner to find address
  - Check SDA/SCL connections (A4, A5)
  - Verify 5V power
  - Adjust contrast pot on back
  - Ensure addresses different (0x27 vs 0x25)

**Problem:** Garbled text
- **Solution:**
  - Check I2C address in code
  - Verify 16x2 LCD (not 20x4)
  - Slow down I2C if needed
  - Check for loose connections

---

### Buzzer Issues

**Problem:** No sound
- **Solution:**
  - Check Pin 2 connection
  - Verify buzzer polarity (+/-)
  - Test with simple tone() sketch
  - Try different buzzer (active vs passive)

**Problem:** Wrong frequency/pattern
- **Solution:**
  - Passive buzzer needs tone() function
  - Active buzzer has fixed frequency
  - Check beep functions in code

---

### LED Strip Issues

**Problem:** LED not turning on
- **Solution:**
  - Check relay connections
  - Verify 12V power to LED strip
  - Test relay manually (digitalWrite)
  - Check light threshold value
  - Ensure relay rated for LED current

**Problem:** LED always on/off
- **Solution:**
  - Cover/uncover light sensor
  - Check BH1750 I2C connection
  - Verify lux threshold (50)
  - Test with Serial Monitor (lux reading)

---

### Power Issues

**Problem:** System resets randomly
- **Solution:**
  - Check power supply capacity (5A+)
  - SIM800A on separate power
  - Add bulk capacitors (1000µF)
  - Check battery charge level
  - Verify BMS functioning

**Problem:** Arduino won't power on
- **Solution:**
  - Check buck converter output (5V)
  - Verify BMS output
  - Test battery voltage (12V)
  - Check all GND connections

---

## Safety & Maintenance

### Safety Precautions

⚠️ **Electrical Safety:**
1. Never connect 5V to 3.3V components (RFID)
2. Use separate power for SIM800A (2A minimum)
3. Ensure all connections properly insulated
4. Keep electronics away from water
5. Secure all wiring to prevent shorts

⚠️ **Mechanical Safety:**
1. Test servo lock mechanism before installation
2. Ensure servo cannot pinch users
3. Secure all components against vibration
4. Use appropriate enclosures

### Regular Maintenance

**Weekly:**
- Clean ultrasonic sensor surfaces
- Check LCD display functionality
- Test RFID readers
- Verify LED strip operation

**Monthly:**
- Test SMS functionality
- Check GPS satellite acquisition
- Inspect all wiring for damage
- Test servo lock mechanism
- Verify battery charge level

**Quarterly:**
- Full system calibration
- Replace battery if needed
- Clean all sensors
- Update RFID authorized list

### Recommended Upgrades

**Reliability:**
- Waterproof enclosure for electronics
- Backup power (UPS module)
- Watchdog timer for auto-reset

**Usability:**
- Status LEDs for visual feedback
- SD card data logging
- Wi-Fi module for remote monitoring
- Touch screen interface

**Scalability:**
- Multiple bin network
- Central monitoring system
- Cloud data storage
- Mobile app integration

---

## Technical Specifications

### System Capabilities

**Bins Supported:** 2 (Biodegradable, Non-Biodegradable)
**User Capacity:** Unlimited RFID cards
**Detection Range:** 2cm - 300cm (ultrasonic)
**Lock Response Time:** < 1 second
**SMS Delay:** 10-30 seconds
**GPS Accuracy:** ±5-10 meters (outdoor)
**LCD Update Rate:** 2 Hz (500ms)
**Power Consumption:** ~3-4A @ 5V

### Environmental Specifications

**Operating Temperature:** -10°C to +50°C
**Operating Humidity:** 10% - 90% (non-condensing)
**Storage Temperature:** -20°C to +60°C
**IP Rating:** IP40 (recommend IP65 enclosure)

---

## Pin Summary Quick Reference

```
DIGITAL PINS:
D0  - GPS RX (conflicts with Serial Monitor)
D1  - GPS TX (conflicts with Serial Monitor)
D2  - Buzzer
D3  - Relay (LED Control)
D4  - SIM800A RX
D5  - SIM800A TX
D6  - Servo Biodegradable
D7  - Servo Non-Biodegradable
D8  - RFID Bio SS
D9  - RFID Shared RST
D10 - RFID NonBio SS
D11 - SPI MOSI (both RFID)
D12 - SPI MISO (both RFID)
D13 - SPI SCK (both RFID)

ANALOG PINS:
A0  - Ultrasonic Bio TRIG
A1  - Ultrasonic Bio ECHO
A2  - Ultrasonic NonBio TRIG
A3  - Ultrasonic NonBio ECHO
A4  - I2C SDA (LCD1, LCD2, BH1750)
A5  - I2C SCL (LCD1, LCD2, BH1750)
```

---

## Support Resources

### Useful Links

- **Arduino Reference:** https://www.arduino.cc/reference/en/
- **TinyGPS++ Docs:** http://arduiniana.org/libraries/tinygpsplus/
- **MFRC522 Library:** https://github.com/miguelbalboa/rfid
- **SIM800A AT Commands:** https://www.elecrow.com/wiki/index.php?title=SIM800_Series_AT_Command_Manual

### I2C Scanner Code

```cpp
// Use this to find LCD addresses
#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(9600);
  Serial.println("I2C Scanner");
}

void loop() {
  for(byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found address: 0x");
      if (i<16) Serial.print('0');
      Serial.println(i, HEX);
    }
  }
  delay(5000);
}
```

---

## Version History

**v2.0 - Arduino Uno Edition**
- Migrated from Arduino Mega to Uno
- Added dual RFID modules (one per bin)
- Removed weight sensors (HX711)
- Added buzzer for audio feedback
- Optimized pin usage for Arduino Uno
- Updated power system documentation

**v1.0 - Arduino Mega Edition**
- Initial release with weight sensors
- Single RFID module
- Arduino Mega 2560

---

## License & Credits

**License:** MIT License - Free for educational and commercial use

**Credits:**
- Arduino Community
- Library Authors (TinyGPS++, MFRC522, BH1750, etc.)
- Smart Bin Project Team

---

## Contact & Support

For questions, issues, or improvements:
1. Check Troubleshooting section
2. Verify wiring matches pin assignments
3. Test components individually
4. Check library versions

**Remember:** GPS uses Hardware Serial (pins 0, 1) which conflicts with USB Serial Monitor. Disconnect GPS when debugging!

---

**END OF DOCUMENTATION**
