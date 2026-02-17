# 🗑️ Smart Waste Bin System — v3.0

Arduino Uno · Dual Bin · RFID · Ultrasonic · GSM · GPS · LCD · Servo

---

## What's New in v3.0

| Enhancement | Detail |
|-------------|--------|
| **Stable ultrasonic** | Reads every **3 seconds** (not constantly) |
| **Confirmation count** | Must read the same state **3× in a row** before acting |
| **Hysteresis** | Full threshold ≠ Empty threshold — prevents bouncing |
| **Instant SMS on full** | Sent the moment bin is confirmed full |
| **Repeat SMS 3×/day** | Every 8 hours while bin remains full |
| **Auto-stop reminders** | SMS stops the moment bin is emptied |
| **Fresh start on empty** | Counter resets — next fill starts the 3× cycle again |
| **Admin RFID** | One card that unlocks both bins + resets SMS counters |
| **BH1750 optional** | System works without light sensor (no crash) |
| **Daily report** | Status SMS every 24 hours |

---

## How the SMS System Works

```
Bin becomes full (confirmed 3× readings)
        │
        ▼
  Immediate SMS sent ──► Counter = 1 / 3
        │
   Still full after 8 hours?
        │ YES
        ▼
  REMINDER 2/3 sent ──► Counter = 2 / 3
        │
   Still full after another 8 hours?
        │ YES
        ▼
  REMINDER 3/3 sent ──► Counter = 3 / 3
        │
   Max reached — no more SMS today
        │
   Bin emptied? (confirmed 3× readings)
        │ YES
        ▼
  Counter resets to 0
  Next time bin fills: cycle starts fresh
```

### SMS Examples

**Immediate alert:**
```
ALERT: Bio bin FULL!
Level:100%
GPS:7.123456,125.654321
```

**8-hour reminder:**
```
REMINDER 2/3: Bio bin still FULL!
GPS:7.123456,125.654321
```

**Daily status:**
```
DAILY REPORT
Bio:FULL
NonBio:OK
Sig:18
GPS:7.123456,125.654321
```

---

## How Stable Ultrasonic Works

```
PROBLEM before:          SOLUTION now:
──────────────────        ──────────────────────────────────────
Read every 100ms    →     Read every 3000ms (3 seconds)
One reading = act   →     3 matching readings in a row = act
One threshold       →     Two thresholds (full ≠ empty)
Flickering values   →     Stable, noise-filtered values
```

### Hysteresis gap prevents bouncing

```
        Sensor reads (cm from top)
        0 ──────────────────────────── BIN_DEPTH
              │         │
              8cm        25cm
           FULL_CM    EMPTY_CM

If dist ≤ 8cm  AND confirmed 3× → LOCK
If dist ≥ 25cm AND confirmed 3× → UNLOCK

Gap = 25 - 8 = 17cm
Bin must be significantly emptied before unlocking
```

---

## Hardware

| Component | Qty | Notes |
|-----------|-----|-------|
| Arduino Uno | 1 | |
| MFRC522 RFID | 2 | 3.3V only! |
| HC-SR04 Ultrasonic | 2 | 5V |
| 16×2 I2C LCD | 2 | Different I2C addresses |
| SG90/MG90S Servo | 2 | |
| SIM800A GSM | 1 | Needs separate 4V @ 2A |
| NEO-6M GPS | 1 | 3.3V |
| BH1750 | 1 | Optional |
| Relay module | 1 | For LED strip |
| Active buzzer | 1 | |
| 10µF 16V capacitor | 2 | One per RFID module |

---

## Pin Map

| Pin | Connected to |
|-----|-------------|
| D0 | GPS TX ⚠ disconnect before upload |
| D1 | GPS RX |
| D2 | Buzzer + |
| D3 | Relay IN |
| D4 | SIM800A TX |
| D5 | SIM800A RX |
| D6 | Servo Bio signal |
| D7 | Servo NonBio signal |
| D8 | RFID Bio SS |
| D9 | RFID shared RST |
| D10 | RFID NonBio SS |
| D11 | SPI MOSI (both RFIDs) |
| D12 | SPI MISO (both RFIDs) |
| D13 | SPI SCK (both RFIDs) |
| A0 | HC-SR04 Bio TRIG |
| A1 | HC-SR04 Bio ECHO |
| A2 | HC-SR04 NonBio TRIG |
| A3 | HC-SR04 NonBio ECHO |
| A4 | SDA — LCD1, LCD2, BH1750 (shared) |
| A5 | SCL — LCD1, LCD2, BH1750 (shared) |

---

## Configuration

All settings at the top of `smart_bin_v3.cpp`:

```cpp
// ── Bin calibration ──────────────────────
#define BIN_DEPTH_CM  30    // Empty bin: sensor to bottom (cm)
#define FULL_CM        8    // Lock when dist ≤ this
#define EMPTY_CM      25    // Unlock when dist ≥ this

// ── Stability ────────────────────────────
#define US_INTERVAL_MS   3000UL  // Read every 3 seconds
#define CONFIRM_NEEDED   3       // Confirm N times before acting

// ── SMS schedule ─────────────────────────
#define SMS_INTERVAL_MS  28800000UL  // 8 hours between reminders
#define MAX_SMS_PER_DAY  3           // Max reminders per day
#define DAY_RESET_MS     86400000UL  // Reset counters every 24h

// ── Contact ──────────────────────────────
const char phone[] = "+639567669410";  // ← Your number

// ── RFID UIDs ────────────────────────────
const char AUTH_UID1[] = "AA BB CC DD";  // ← Your card
const char AUTH_UID2[] = "11 22 33 44";  // ← Your card
const char ADMIN_UID[]  = "FF EE DD CC"; // ← Admin card
```

### Calibrating your bin

```
1. Upload calibration_COMPACT.cpp with bins empty
2. Note the distance shown = your BIN_DEPTH_CM
3. Set FULL_CM to how full before locking (e.g. 8cm from sensor)
4. Set EMPTY_CM higher than FULL_CM with a gap (e.g. 25cm)
   Gap prevents bouncing open/close at the threshold
```

---

## I2C Addresses

| Device | Address |
|--------|---------|
| LCD Bio | 0x27 |
| LCD NonBio | 0x25 |
| BH1750 (optional) | 0x23 |

Run `i2c_lcd_scanner.cpp` first to confirm your addresses.

---

## LCD Display

```
Bio OPEN   45%      ← status + level percentage
[====    ]          ← 8-segment bar

Bio LOCK  100%
[========] FULL
```

---

## Buzzer Sounds

| Sound | Trigger |
|-------|---------|
| 2× rising tones | System ready / authorized user |
| 3× medium beeps | Bin full — locking |
| Low single buzz | Unauthorized card |
| Long high tone | Admin card detected |

---

## Critical Wiring Notes

### RFID — 3.3V only
```
RFID VCC → Arduino 3.3V  (NOT 5V — will damage module!)
```

### RFID Capacitor
```
Place near each RFID module:

 RFID VCC ──(+)── 10µF 16V ──(−)── GND
            Long leg         Short leg / stripe
```

### SIM800A — External power
```
SIM800A needs 3.7–4.2V at 2A minimum
Do NOT power from Arduino 5V pin
Use a separate LiPo battery or dedicated regulator
Common GND between Arduino and SIM800A is required
```

### GPS — Disconnect before upload
```
GPS TX connects to D0 (Arduino hardware serial = same as USB)
MUST disconnect GPS TX wire before uploading code
Reconnect after upload completes
```

---

## Setup Checklist

**Before first upload:**

- [ ] Run `i2c_lcd_scanner.cpp` → confirm LCD addresses
- [ ] Run `rfid_lcd_debug_test.cpp` → get your card UIDs
- [ ] Run `calibration_COMPACT.cpp` → get your bin distances
- [ ] Update `phone[]` with your mobile number
- [ ] Update `AUTH_UID1`, `AUTH_UID2`, `ADMIN_UID` with your card UIDs
- [ ] Update `BIN_DEPTH_CM`, `FULL_CM`, `EMPTY_CM` from calibration
- [ ] SIM800A has separate 4V @ 2A power supply
- [ ] RFID modules wired to 3.3V (not 5V)
- [ ] Capacitors added (10µF 16V, one per RFID)
- [ ] GPS TX disconnected from D0

**Upload:**
- [ ] Disconnect GPS TX from D0
- [ ] Upload `smart_bin_v3.cpp`
- [ ] Reconnect GPS TX to D0

**Test:**
- [ ] Empty bins show 0%
- [ ] Adding waste increases %
- [ ] Blocking sensor (< 8cm) three times → locks + SMS received
- [ ] Clearing sensor (> 25cm) three times → unlocks
- [ ] Authorized card → two-tone beep
- [ ] Admin card → both bins open
- [ ] DEBUG_MODE serial shows distances every 5 seconds

---

## Troubleshooting

| Problem | Cause | Fix |
|---------|-------|-----|
| Upload fails | GPS TX on D0 | Disconnect GPS before upload |
| LCD blank / wrong chars | Wrong I2C address | Run I2C scanner |
| RFID not reading cards | Wrong voltage | Use 3.3V not 5V |
| RFID reads intermittently | No capacitor | Add 10µF between VCC and GND |
| No SMS sent | SIM800A power | Use separate 4V @ 2A supply |
| Bin locks on empty | FULL_CM too high | Reduce FULL_CM value |
| Bin unlocks immediately | EMPTY_CM too low | Increase EMPTY_CM value |
| Level % wrong | Uncalibrated | Run calibration tool, update BIN_DEPTH_CM |
| BH1750 error printed | Not connected | Fine — system works without it |
| Memory overflow | Code too large | Use F() macros, remove unused libs |
| SMS only sent once | Working correctly | Next reminder comes after 8 hours |
| GPS shows NoFix | No satellite lock | Move outdoors, wait 1–2 minutes |

---

**v3.0 — Stable · Reliable · SMS 3×/day until emptied 🎉**
