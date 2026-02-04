# SMART WASTE BIN - PIN ASSIGNMENT REFERENCE
## Complete Pin Configuration

### DIGITAL PINS

| Pin | Component | Function | Direction | Notes |
|-----|-----------|----------|-----------|-------|
| D2  | GPS Module | RX | Input | SoftwareSerial - receives from GPS TX |
| D3  | GPS Module | TX | Output | SoftwareSerial - transmits to GPS RX |
| D4  | Bio Ultrasonic (Human) | Trigger | Output | Sends ultrasonic pulse |
| D5  | Bio Ultrasonic (Human) | Echo | Input | Receives ultrasonic echo |
| D6  | Non-Bio Ultrasonic (Human) | Trigger | Output | Sends ultrasonic pulse |
| D7  | Non-Bio Ultrasonic (Human) | Echo | Input | Receives ultrasonic echo |
| D8  | LED Relay | Control | Output | HIGH=OFF, LOW=ON |
| D9  | SIM800A GSM | RX | Input | **FIXED** - SoftwareSerial RX |
| D10 | SIM800A GSM | TX | Output | **FIXED** - SoftwareSerial TX |
| D11 | Biodegradable Servo | PWM Signal | Output | Lid control servo |
| D12 | Non-Biodegradable Servo | PWM Signal | Output | Lid control servo |
| D13 | (Available) | - | - | Reserved for status LED if needed |

### ANALOG PINS (used as Digital)

| Pin | Component | Function | Direction | Notes |
|-----|-----------|----------|-----------|-------|
| A0  | Bio Ultrasonic (Full) | Trigger | Output | Bin fullness detection |
| A1  | Bio Ultrasonic (Full) | Echo | Input | Bin fullness detection |
| A2  | Non-Bio Ultrasonic (Full) | Trigger | Output | Bin fullness detection |
| A3  | Non-Bio Ultrasonic (Full) | Echo | Input | Bin fullness detection |
| A4  | (Reserved for I2C SDA) | - | - | Used by BH1750 & LCDs |
| A5  | (Reserved for I2C SCL) | - | - | Used by BH1750 & LCDs |

### I2C BUS (Shared)

| Device | Address | Pins | Function |
|--------|---------|------|----------|
| LCD 1 (Bio) | 0x27 | SDA/SCL | 16x2 Display - Biodegradable bin |
| LCD 2 (Non-Bio) | 0x25 | SDA/SCL | 16x2 Display - Non-biodegradable bin |
| BH1750 Light Sensor | 0x23 (default) | SDA/SCL | Ambient light detection |

### POWER CONNECTIONS

| Component | VCC | GND | Notes |
|-----------|-----|-----|-------|
| GPS Module | 3.3V-5V | GND | Check module specs |
| SIM800A | 3.7V-4.2V | GND | **Requires separate power supply (2A)** |
| Ultrasonic Sensors (4x) | 5V | GND | HC-SR04 compatible |
| Servos (2x) | 5V | GND | May need external 5V supply if drawing >500mA |
| BH1750 | 3.3V-5V | GND | I2C pull-ups included |
| LCD Modules (2x) | 5V | GND | I2C backpack included |
| Relay Module | 5V | GND | Controls LED strip/lights |

---

## KEY FIXES FROM ORIGINAL CODE

### ❌ **ORIGINAL CONFLICT:**
```cpp
const int echoNonBio = 7;          // Pin 7
const int relayLED = 8;             // Pin 8
SoftwareSerial sim800(7, 8);        // Pin 7 & 8 - CONFLICT!
```

### ✅ **FIXED ASSIGNMENT:**
```cpp
const int echoNonBio = 7;          // Pin 7 - Ultrasonic only
const int relayLED = 8;             // Pin 8 - Relay only
SoftwareSerial sim800(9, 10);       // Pin 9 & 10 - SIM800A (NO CONFLICT)
```

---

## WIRING GUIDE

### GPS Module (Neo-6M or similar)
```
GPS VCC  → Arduino 5V (or 3.3V depending on module)
GPS GND  → Arduino GND
GPS TX   → Arduino Pin 2 (RX)
GPS RX   → Arduino Pin 3 (TX)
```

### SIM800A GSM Module
```
SIM800A VCC → External 3.7-4.2V Power Supply (2A capable)
SIM800A GND → Common Ground with Arduino
SIM800A RXD → Arduino Pin 10 (TX)
SIM800A TXD → Arduino Pin 9 (RX)
```
**⚠️ IMPORTANT:** SIM800A draws high current (up to 2A during transmission). Use external power supply!

### Ultrasonic Sensors (HC-SR04)

**Biodegradable Bin - Human Detection:**
```
VCC  → 5V
GND  → GND
TRIG → Pin 4
ECHO → Pin 5
```

**Non-Biodegradable Bin - Human Detection:**
```
VCC  → 5V
GND  → GND
TRIG → Pin 6
ECHO → Pin 7
```

**Biodegradable Bin - Fullness Detection:**
```
VCC  → 5V
GND  → GND
TRIG → Pin A0
ECHO → Pin A1
```

**Non-Biodegradable Bin - Fullness Detection:**
```
VCC  → 5V
GND  → GND
TRIG → Pin A2
ECHO → Pin A3
```

### Servo Motors (SG90 or similar)

**Biodegradable Lid Servo:**
```
Red (VCC)    → 5V (or external 5V supply if both servos draw >500mA)
Brown (GND)  → GND
Orange (PWM) → Pin 11
```

**Non-Biodegradable Lid Servo:**
```
Red (VCC)    → 5V (or external 5V supply)
Brown (GND)  → GND
Orange (PWM) → Pin 12
```

### BH1750 Light Sensor (I2C)
```
VCC → 5V (or 3.3V)
GND → GND
SDA → A4 (SDA)
SCL → A5 (SCL)
```

### LCD Modules with I2C Backpack

**Both LCDs share I2C bus (different addresses):**
```
VCC → 5V
GND → GND
SDA → A4 (SDA)
SCL → A5 (SCL)
```

### LED Relay Module
```
VCC → 5V
GND → GND
IN  → Pin 8
COM → LED Power Supply (+)
NO  → LED Strip (+)
```

---

## CODE IMPROVEMENTS MADE

1. ✅ **Fixed pin conflict** - SIM800A moved to pins 9 & 10
2. ✅ **Non-blocking servo control** - State machine replaces blocking delays
3. ✅ **GPS location in SMS** - Adds coordinates to alert messages
4. ✅ **Error handling** - BH1750 initialization check, SMS response verification
5. ✅ **Throttled debug output** - Reduces serial spam to 1Hz
6. ✅ **Improved LCD formatting** - Better spacing and display
7. ✅ **Input validation** - Distance checks for valid range
8. ✅ **Startup diagnostics** - Prints pin assignments at boot

---

## TESTING CHECKLIST

- [ ] GPS receives valid satellite fix
- [ ] Both LCDs display correctly (check I2C addresses with I2C scanner)
- [ ] All 4 ultrasonic sensors read distances accurately
- [ ] Servo lids open when hand approaches (<50cm)
- [ ] Servo lids close 3 seconds after hand removed
- [ ] LED relay activates when ambient light < 50 lux
- [ ] SMS sends successfully when bin becomes full
- [ ] SMS includes GPS coordinates
- [ ] No serial conflicts or random resets

---

## POWER BUDGET ESTIMATION

| Component | Current Draw | Notes |
|-----------|--------------|-------|
| Arduino Uno | ~50mA | Base consumption |
| GPS Module | ~30-50mA | Active tracking |
| **SIM800A** | **350mA idle, 2A peak** | **Needs external PSU!** |
| Ultrasonic x4 | ~60mA total | ~15mA each |
| Servos x2 | ~100-500mA each | Depends on load |
| LCDs x2 | ~60mA total | With backlight |
| BH1750 | ~0.12mA | Negligible |
| LED Strip | Varies | Via relay |

**Total (without servos/LEDs): ~540mA + 2A peak (SIM800A)**

⚠️ **CRITICAL:** Do NOT power SIM800A from Arduino 5V pin - use separate 3.7-4.2V supply rated for 2A+

---

## RECOMMENDED HARDWARE

- **Arduino:** Uno or Mega
- **GPS:** Neo-6M / Neo-7M
- **GSM:** SIM800A / SIM800L with external power supply
- **Ultrasonic:** HC-SR04 (x4)
- **Servos:** SG90 or MG90S (x2)
- **Light Sensor:** BH1750 (I2C)
- **LCDs:** 16x2 with I2C backpack (PCF8574)
- **Relay:** 5V Single Channel
- **Power:** 5V/3A for Arduino + peripherals, 3.7-4.2V/2A for SIM800A