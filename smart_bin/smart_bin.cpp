/*
 * SMART WASTE BIN SYSTEM v3.1
 * smart_bin.cpp - full implementation
 *
 * Per-bin calibration:
 *   BIO    empty=95cm  full=10cm  usable=85cm
 *   NONBIO empty=50cm  full=10cm  usable=40cm
 *
 * Servo: OPEN=0deg, LOCKED=90deg
 */

#include "smart_bin.h"

/* -------------------------------------------
   HARDWARE OBJECT DEFINITIONS
   ------------------------------------------- */
TinyGPSPlus        gps;
LiquidCrystal_I2C  lcd1(0x27, 16, 2);
LiquidCrystal_I2C  lcd2(0x25, 16, 2);
BH1750             lightMeter;
SoftwareSerial     sim800(PIN_SIM_RX, PIN_SIM_TX);
Servo              servoBio;
Servo              servoNon;
MFRC522            rfidBio(PIN_RFID_BIO_SS, PIN_RFID_RST);
MFRC522            rfidNonBio(PIN_RFID_NON_SS, PIN_RFID_RST);

/* -------------------------------------------
   STATE VARIABLE DEFINITIONS
   ------------------------------------------- */
bool          lightSensorOK  = false;
float         currentLux     = 0.0f;
bool          ambientLEDOn   = false;
unsigned long lastLuxRead    = 0;

bool          bioLocked      = false;
bool          nonLocked      = false;

long          bioDist        = BIO_DEPTH_CM;
long          nonDist        = NON_DEPTH_CM;
unsigned long lastUSRead     = 0;

int           bioFullCnt     = 0;
int           bioEmptyCnt    = 0;
int           nonFullCnt     = 0;
int           nonEmptyCnt    = 0;

unsigned long bioLastSMSTime = 0;
unsigned long nonLastSMSTime = 0;
int           bioSMSCount    = 0;
int           nonSMSCount    = 0;
unsigned long dayStart       = 0;
unsigned long lastDailySMS   = 0;

/* -------------------------------------------
   HELPER: SEND SMS
   ------------------------------------------- */
void sendSMS(const char* msg)
{
    sim800.println(F("AT+CMGF=1"));   delay(300);
    sim800.print(F("AT+CMGS=\""));
    sim800.print(PHONE);
    sim800.println(F("\""));          delay(300);
    sim800.print(msg);
    sim800.write(26);
    delay(5000);
    if (DEBUG_MODE) {
        Serial.print(F("[SMS] "));
        Serial.println(msg);
    }
}

/* -------------------------------------------
   HELPER: GPS STRING
   ------------------------------------------- */
String gpsStr()
{
    if (gps.location.isValid()) {
        return String(gps.location.lat(), 6) + F(",") +
               String(gps.location.lng(), 6);
    }
    return F("NoFix");
}

/* -------------------------------------------
   HELPER: SIGNAL STRENGTH
   ------------------------------------------- */
int getSignal()
{
    sim800.println(F("AT+CSQ")); delay(400);
    String r = "";
    while (sim800.available()) r += (char)sim800.read();
    int i = r.indexOf(F("+CSQ:"));
    if (i != -1) return r.substring(i + 6, r.indexOf(',', i)).toInt();
    return 0;
}

/* -------------------------------------------
   HELPER: READ MEDIAN DISTANCE
   5 pulses -> bubble sort -> return median
   Improved timeout handling for close objects
   ------------------------------------------- */
long readDist(uint8_t trig, uint8_t echo)
{
    const uint8_t N = 5;
    long v[N];
    int validCount = 0;
    
    for (uint8_t i = 0; i < N; i++) {
        digitalWrite(trig, LOW);  delayMicroseconds(5);
        digitalWrite(trig, HIGH); delayMicroseconds(10);
        digitalWrite(trig, LOW);
        
        // Longer timeout for far objects, but also handle very close
        long duration = pulseIn(echo, HIGH, 40000UL);
        
        if (duration > 0) {
            long dist = (duration * 34L) / 2000L;
            // Valid range: 2cm to 400cm
            if (dist >= 2 && dist <= 400) {
                v[i] = dist;
                validCount++;
            } else if (dist < 2) {
                // Object is extremely close (touching sensor)
                v[i] = 2;  // Minimum readable distance
                validCount++;
            } else {
                v[i] = 999L;  // Out of range
            }
        } else {
            // Timeout - could mean very close or no echo
            // Try one more quick read
            digitalWrite(trig, LOW);  delayMicroseconds(2);
            digitalWrite(trig, HIGH); delayMicroseconds(10);
            digitalWrite(trig, LOW);
            duration = pulseIn(echo, HIGH, 10000UL);
            
            if (duration > 0 && duration < 200) {
                // Very close object
                v[i] = 2;
                validCount++;
            } else {
                v[i] = 999L;
            }
        }
        delay(10);
    }
    
    // If we got at least 3 valid readings, use median
    if (validCount >= 3) {
        // Bubble sort
        for (uint8_t i = 0; i < N - 1; i++)
            for (uint8_t j = i + 1; j < N; j++)
                if (v[i] > v[j]) { long t = v[i]; v[i] = v[j]; v[j] = t; }
        return v[N / 2];
    }
    
    // If mostly invalid, return the best valid reading we got
    long minValid = 999L;
    for (uint8_t i = 0; i < N; i++) {
        if (v[i] < 999L && v[i] < minValid) {
            minValid = v[i];
        }
    }
    
    if (DEBUG_MODE && minValid == 999L) {
        Serial.print(F("WARNING: Sensor timeout on pin "));
        Serial.println(trig);
    }
    
    return minValid;
}

/* -------------------------------------------
   LEVEL % - separate formula per bin
   BIO:    (95 - dist) / 85 * 100
   NONBIO: (50 - dist) / 40 * 100
   Both clamped 0-100%
   ------------------------------------------- */
int bioPct()
{
    long d      = constrain(bioDist, (long)BIO_FULL_CM, (long)BIO_DEPTH_CM);
    long filled = BIO_DEPTH_CM - d;
    return (int)((filled * 100L) / BIO_USABLE_CM);
}

int nonPct()
{
    long d      = constrain(nonDist, (long)NON_FULL_CM, (long)NON_DEPTH_CM);
    long filled = NON_DEPTH_CM - d;
    return (int)((filled * 100L) / NON_USABLE_CM);
}

/* -------------------------------------------
   LEVEL BAR: 8 segments from pct value
   e.g. pct=50 -> [====    ]
   ------------------------------------------- */
String levelBar(int pct)
{
    int segs = (pct * 8) / 100;
    String b = F("[");
    for (int i = 0; i < 8; i++) b += (i < segs ? '=' : ' ');
    b += ']';
    return b;
}

/* -------------------------------------------
   SERVO: FORCE OPEN
   Goes to LOCKED(90) first so motor always
   travels the full arc to OPEN(0).
   ------------------------------------------- */
void servoForceOpen(Servo &srv)
{
    srv.write(SERVO_LOCKED);    // 90 deg
    delay(700);
    srv.write(SERVO_UNLOCKED);  // 0 deg
    delay(700);
}

/* -------------------------------------------
   ULTRASONIC: INTERVAL + HYSTERESIS + CONFIRM
   Each bin uses its own thresholds
   ------------------------------------------- */
void updateDistances()
{
    if (millis() - lastUSRead < US_INTERVAL_MS) return;
    lastUSRead = millis();

    bioDist = readDist(PIN_TRIG_BIO, PIN_ECHO_BIO);
    nonDist = readDist(PIN_TRIG_NON, PIN_ECHO_NON);

    // ---- BIO BIN ----
    if (!bioLocked) {
        if (bioDist <= BIO_FULL_CM) { bioFullCnt++;  bioEmptyCnt = 0; }
        else                        { bioFullCnt = 0; }

        if (bioFullCnt >= CONFIRM_NEEDED) {
            bioFullCnt = 0;
            servoBio.write(SERVO_LOCKED);
            bioLocked = true;
            for (uint8_t i = 0; i < 3; i++) { tone(PIN_BUZZER, 1500, 150); delay(250); }
            String msg = F("ALERT: BIO bin FULL!\nLevel:100%\nGPS:");
            msg += gpsStr();
            sendSMS(msg.c_str());
            bioLastSMSTime = millis();
            bioSMSCount    = 1;
            if (DEBUG_MODE) Serial.println(F(">>> BIO LOCKED"));
        }
    } else {
        if (bioDist >= BIO_EMPTY_CM) { bioEmptyCnt++;  bioFullCnt = 0; }
        else                         { bioEmptyCnt = 0; }

        if (bioEmptyCnt >= CONFIRM_NEEDED) {
            bioEmptyCnt = 0;
            servoForceOpen(servoBio);
            bioLocked   = false;
            bioSMSCount = 0;
            tone(PIN_BUZZER, 2500, 100); delay(120);
            tone(PIN_BUZZER, 2000, 100);
            if (DEBUG_MODE) Serial.println(F(">>> BIO UNLOCKED (emptied)"));
        }
    }

    // ---- NON-BIO BIN ----
    if (!nonLocked) {
        if (nonDist <= NON_FULL_CM) { nonFullCnt++;  nonEmptyCnt = 0; }
        else                        { nonFullCnt = 0; }

        if (nonFullCnt >= CONFIRM_NEEDED) {
            nonFullCnt = 0;
            servoNon.write(SERVO_LOCKED);
            nonLocked = true;
            for (uint8_t i = 0; i < 3; i++) { tone(PIN_BUZZER, 1500, 150); delay(250); }
            String msg = F("ALERT: NON-BIO bin FULL!\nLevel:100%\nGPS:");
            msg += gpsStr();
            sendSMS(msg.c_str());
            nonLastSMSTime = millis();
            nonSMSCount    = 1;
            if (DEBUG_MODE) Serial.println(F(">>> NON-BIO LOCKED"));
        }
    } else {
        if (nonDist >= NON_EMPTY_CM) { nonEmptyCnt++;  nonFullCnt = 0; }
        else                         { nonEmptyCnt = 0; }

        if (nonEmptyCnt >= CONFIRM_NEEDED) {
            nonEmptyCnt = 0;
            servoForceOpen(servoNon);
            nonLocked   = false;
            nonSMSCount = 0;
            tone(PIN_BUZZER, 2500, 100); delay(120);
            tone(PIN_BUZZER, 2000, 100);
            if (DEBUG_MODE) Serial.println(F(">>> NON-BIO UNLOCKED (emptied)"));
        }
    }
}

/* -------------------------------------------
   SMS REPEAT: 3x PER DAY WHILE STILL FULL
   ------------------------------------------- */
void checkRepeatSMS()
{
    unsigned long now = millis();

    if (now - dayStart >= DAY_RESET_MS) {
        dayStart    = now;
        bioSMSCount = bioLocked ? bioSMSCount : 0;
        nonSMSCount = nonLocked ? nonSMSCount : 0;
        if (DEBUG_MODE) Serial.println(F("[SMS] Day counter reset"));
    }

    if (bioLocked && bioSMSCount < MAX_SMS_PER_DAY) {
        if (now - bioLastSMSTime >= SMS_INTERVAL_MS) {
            bioSMSCount++;
            String msg = F("REMINDER ");
            msg += bioSMSCount; msg += F("/"); msg += MAX_SMS_PER_DAY;
            msg += F(": BIO bin still FULL!\nGPS:"); msg += gpsStr();
            sendSMS(msg.c_str());
            bioLastSMSTime = now;
            if (DEBUG_MODE) {
                Serial.print(F("[SMS] Bio reminder "));
                Serial.print(bioSMSCount); Serial.print('/');
                Serial.println(MAX_SMS_PER_DAY);
            }
        }
    }

    if (nonLocked && nonSMSCount < MAX_SMS_PER_DAY) {
        if (now - nonLastSMSTime >= SMS_INTERVAL_MS) {
            nonSMSCount++;
            String msg = F("REMINDER ");
            msg += nonSMSCount; msg += F("/"); msg += MAX_SMS_PER_DAY;
            msg += F(": NON-BIO bin still FULL!\nGPS:"); msg += gpsStr();
            sendSMS(msg.c_str());
            nonLastSMSTime = now;
            if (DEBUG_MODE) {
                Serial.print(F("[SMS] NonBio reminder "));
                Serial.print(nonSMSCount); Serial.print('/');
                Serial.println(MAX_SMS_PER_DAY);
            }
        }
    }

    if (now - lastDailySMS >= DAY_RESET_MS) {
        String msg = F("DAILY REPORT\nBIO:");
        msg += (bioLocked ? F("FULL") : F("OK"));
        msg += F("\nNON-BIO:");
        msg += (nonLocked ? F("FULL") : F("OK"));
        msg += F("\nSig:"); msg += getSignal();
        msg += F("\nGPS:"); msg += gpsStr();
        sendSMS(msg.c_str());
        lastDailySMS = now;
        if (DEBUG_MODE) Serial.println(F("[SMS] Daily report sent"));
    }
}

/* -------------------------------------------
   RFID: GET UID STRING
   ------------------------------------------- */
String getUID(MFRC522 &r)
{
    String s = "";
    for (uint8_t i = 0; i < r.uid.size; i++) {
        s += (r.uid.uidByte[i] < 0x10 ? F(" 0") : F(" "));
        s += String(r.uid.uidByte[i], HEX);
    }
    s.trim();
    s.toUpperCase();
    return s;
}

/* -------------------------------------------
   RFID: PROCESS CARD
   Authorized -> servoForceOpen + SMS
   Unauthorized -> reject tone + LCD
   ------------------------------------------- */
void processCard(String uid, LiquidCrystal_I2C &lcd, bool isBioReader)
{
    if (DEBUG_MODE) {
        Serial.print(F("Card: "));
        Serial.println(uid);
    }

    if (uid == String(AUTH_UID1) || uid == String(AUTH_UID2)) {
        tone(PIN_BUZZER, 2000, 100); delay(120);
        tone(PIN_BUZZER, 2500, 100);

        if (isBioReader) {
            servoForceOpen(servoBio);
            bioLocked   = false;
            bioSMSCount = 0;
            lcd.setCursor(0, 0);
            lcd.print(F("  BIO  WASTE    "));
            lcd.setCursor(0, 1);
            lcd.print(F("    UNLOCKED    "));
            String msg = F("AUTH: BIO bin unlocked via RFID.\nGPS:");
            msg += gpsStr();
            sendSMS(msg.c_str());
            if (DEBUG_MODE) Serial.println(F("AUTH -> BIO UNLOCKED + SMS sent"));
        } else {
            servoForceOpen(servoNon);
            nonLocked   = false;
            nonSMSCount = 0;
            lcd.setCursor(0, 0);
            lcd.print(F(" NON-BIO WASTE  "));
            lcd.setCursor(0, 1);
            lcd.print(F("    UNLOCKED    "));
            String msg = F("AUTH: NON-BIO bin unlocked via RFID.\nGPS:");
            msg += gpsStr();
            sendSMS(msg.c_str());
            if (DEBUG_MODE) Serial.println(F("AUTH -> NON-BIO UNLOCKED + SMS sent"));
        }
        delay(2000);

    } else {
        tone(PIN_BUZZER, 400, 300);
        lcd.setCursor(0, 0);
        lcd.print(F("  UNAUTHORIZED  "));
        lcd.setCursor(0, 1);
        lcd.print(F("  ACCESS DENIED "));
        delay(2000);
        if (DEBUG_MODE) Serial.println(F("UNAUTHORIZED"));
    }
}

/* -------------------------------------------
   RFID: CHECK BOTH READERS
   ------------------------------------------- */
void checkRFID()
{
    if (rfidBio.PICC_IsNewCardPresent() && rfidBio.PICC_ReadCardSerial()) {
        processCard(getUID(rfidBio), lcd1, true);
        rfidBio.PICC_HaltA();
        rfidBio.PCD_StopCrypto1();
    }
    if (rfidNonBio.PICC_IsNewCardPresent() && rfidNonBio.PICC_ReadCardSerial()) {
        processCard(getUID(rfidNonBio), lcd2, false);
        rfidNonBio.PICC_HaltA();
        rfidNonBio.PCD_StopCrypto1();
    }
}

/* -------------------------------------------
   LIGHT SENSOR (optional - skipped if absent)
   ------------------------------------------- */
void updateLight()
{
    if (!lightSensorOK) return;
    if (millis() - lastLuxRead < 1000UL) return;
    lastLuxRead  = millis();
    currentLux   = lightMeter.readLightLevel();
    ambientLEDOn = (currentLux < LUX_THRESHOLD);
}

/* -------------------------------------------
   LCD LAYOUT (16x2) - Alternating Display
   Cycle 1: Line 0: "BIO          75%"
            Line 1: "[======  ]  45cm"
   Cycle 2: Line 0: "GPS: 10.31234"
            Line 1: "     121.98765"
   ------------------------------------------- */
void updateLCD()
{
    static unsigned long lastCycle = 0;
    static bool showGPS = false;
    
    // Toggle between normal display and GPS every 3 seconds
    if (millis() - lastCycle >= 3000UL) {
        lastCycle = millis();
        showGPS = !showGPS;
    }

    if (showGPS && gps.location.isValid()) {
        // ---- SHOW GPS ON BOTH LCDs ----
        String lat = String(gps.location.lat(), 5);
        String lng = String(gps.location.lng(), 5);
        
        // BIO LCD - GPS
        lcd1.clear();
        lcd1.setCursor(0, 0);
        lcd1.print(F("GPS:"));
        lcd1.print(lat.substring(0, 11));
        
        lcd1.setCursor(0, 1);
        lcd1.print(F("    "));
        lcd1.print(lng.substring(0, 11));
        
        // NON-BIO LCD - GPS
        lcd2.clear();
        lcd2.setCursor(0, 0);
        lcd2.print(F("GPS:"));
        lcd2.print(lat.substring(0, 11));
        
        lcd2.setCursor(0, 1);
        lcd2.print(F("    "));
        lcd2.print(lng.substring(0, 11));
        
    } else {
        // ---- SHOW NORMAL BIN STATUS ----
        int bp = bioPct();
        int np = nonPct();

        // ---- BIO LCD (lcd1) ----
        lcd1.clear();
        lcd1.setCursor(0, 0);
        lcd1.print(F("BIO         "));  // Removed "WASTE"
        if      (bp < 10)  lcd1.print(F("  "));
        else if (bp < 100) lcd1.print(F(" "));
        lcd1.print(bp);
        lcd1.print('%');

        lcd1.setCursor(0, 1);
        lcd1.print(levelBar(bp));           // 10 chars [========]
        if (bioLocked) {
            lcd1.print(F(" FULL "));
        } else {
            lcd1.print(F(" "));
            if      (bioDist < 10)  lcd1.print(F("  "));
            else if (bioDist < 100) lcd1.print(F(" "));
            lcd1.print(bioDist);
            lcd1.print(F("cm"));
        }

        // ---- NON-BIO LCD (lcd2) ----
        lcd2.clear();
        lcd2.setCursor(0, 0);
        lcd2.print(F("NON-BIO     "));
        if      (np < 10)  lcd2.print(F("  "));
        else if (np < 100) lcd2.print(F(" "));
        lcd2.print(np);
        lcd2.print('%');

        lcd2.setCursor(0, 1);
        lcd2.print(levelBar(np));
        if (nonLocked) {
            lcd2.print(F(" FULL "));
        } else {
            lcd2.print(F(" "));
            if      (nonDist < 10)  lcd2.print(F("  "));
            else if (nonDist < 100) lcd2.print(F(" "));
            lcd2.print(nonDist);
            lcd2.print(F("cm"));
        }
    }
}

/* -------------------------------------------
   SETUP
   ------------------------------------------- */
void setup()
{
    Serial.begin(9600);
    Wire.begin();

    lcd1.init(); lcd1.backlight();
    lcd2.init(); lcd2.backlight();

    lcd1.clear();
    lcd1.setCursor(0, 0); lcd1.print(F("  BIO  WASTE    "));
    lcd1.setCursor(0, 1); lcd1.print(F(" Initializing.. "));

    lcd2.clear();
    lcd2.setCursor(0, 0); lcd2.print(F(" NON-BIO WASTE  "));
    lcd2.setCursor(0, 1); lcd2.print(F(" Initializing.. "));

    SPI.begin();
    rfidBio.PCD_Init();    delay(10);
    rfidNonBio.PCD_Init(); delay(10);

    pinMode(PIN_BUZZER,    OUTPUT);
    pinMode(PIN_RELAY_LED, OUTPUT); digitalWrite(PIN_RELAY_LED, LOW);
    pinMode(PIN_TRIG_BIO,  OUTPUT); pinMode(PIN_ECHO_BIO, INPUT);
    pinMode(PIN_TRIG_NON,  OUTPUT); pinMode(PIN_ECHO_NON, INPUT);

    // Boot: LOCKED(90) then OPEN(0) for guaranteed physical movement
    servoBio.attach(PIN_SERVO_BIO);
    servoBio.write(SERVO_LOCKED);   delay(800);
    servoBio.write(SERVO_UNLOCKED); delay(800);

    servoNon.attach(PIN_SERVO_NON);
    servoNon.write(SERVO_LOCKED);   delay(800);
    servoNon.write(SERVO_UNLOCKED); delay(800);

    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
        lightSensorOK = true;
        if (DEBUG_MODE) Serial.println(F("BH1750 OK"));
    } else {
        if (DEBUG_MODE) Serial.println(F("BH1750 not found - LED manual"));
    }

    sim800.begin(9600);
    delay(3000);
    sim800.println(F("AT"));                   delay(500);
    sim800.println(F("ATE0"));                delay(500);
    sim800.println(F("AT+CMGF=1"));           delay(500);
    sim800.println(F("AT+CSCS=\"GSM\""));     delay(500);
    sim800.println(F("AT+CSMP=17,167,0,0"));  delay(500);

    dayStart     = millis();
    lastDailySMS = millis();

    tone(PIN_BUZZER, 2000, 100); delay(120);
    tone(PIN_BUZZER, 2500, 100);

    lcd1.clear(); lcd2.clear();

    if (DEBUG_MODE) {
        Serial.println(F("==========================="));
        Serial.println(F("   SMART BIN v3.1 READY"));
        Serial.println(F("==========================="));
        Serial.print(F("BIO    empty=")); Serial.print(BIO_DEPTH_CM);
        Serial.print(F("cm  full=")); Serial.print(BIO_FULL_CM);
        Serial.print(F("cm  usable=")); Serial.print(BIO_USABLE_CM); Serial.println(F("cm"));
        Serial.print(F("NONBIO empty=")); Serial.print(NON_DEPTH_CM);
        Serial.print(F("cm  full=")); Serial.print(NON_FULL_CM);
        Serial.print(F("cm  usable=")); Serial.print(NON_USABLE_CM); Serial.println(F("cm"));
        Serial.print(F("Confirm: ")); Serial.print(CONFIRM_NEEDED); Serial.println(F("x reads"));
        Serial.print(F("Interval: ")); Serial.print(US_INTERVAL_MS / 1000); Serial.println(F("s"));
        Serial.println(F("==========================="));
    }
}

/* -------------------------------------------
   MAIN LOOP
   ------------------------------------------- */
void loop()
{
    while (Serial.available()) gps.encode(Serial.read());

    checkRFID();
    updateLight();
    updateDistances();
    checkRepeatSMS();

    digitalWrite(PIN_RELAY_LED,
                 (ambientLEDOn || bioLocked || nonLocked) ? HIGH : LOW);

    updateLCD();

#if DEBUG_MODE
    static unsigned long lastDbg = 0;
    if (millis() - lastDbg >= 5000UL) {
        lastDbg = millis();
        Serial.print(F("BIO "));
        Serial.print(bioDist); Serial.print(F("cm "));
        Serial.print(bioPct()); Serial.print(F("% "));
        Serial.print(bioLocked ? F("LOCKED") : F("open"));
        Serial.print(F("  | NON-BIO "));
        Serial.print(nonDist); Serial.print(F("cm "));
        Serial.print(nonPct()); Serial.print(F("% "));
        Serial.println(nonLocked ? F("LOCKED") : F("open"));
        Serial.print(F("Bio SMS today: ")); Serial.print(bioSMSCount);
        Serial.print(F("  Non SMS today: ")); Serial.println(nonSMSCount);
        if (lightSensorOK) { Serial.print(F("Lux: ")); Serial.println(currentLux); }
    }
#endif

    delay(100);
}