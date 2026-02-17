/*
 * SMART WASTE BIN SYSTEM v3.1
 * smart_bin.cpp - full implementation
 *
 * Sensor: top-down lid mount
 *   30cm = empty (0%)
 *   10cm = full (100%)
 *   level% = (30 - dist) / 20 * 100
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

long          bioDist        = BIN_DEPTH_CM;
long          nonDist        = BIN_DEPTH_CM;
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
   ------------------------------------------- */
long readDist(uint8_t trig, uint8_t echo)
{
    const uint8_t N = 5;
    long v[N];
    for (uint8_t i = 0; i < N; i++) {
        digitalWrite(trig, LOW);  delayMicroseconds(2);
        digitalWrite(trig, HIGH); delayMicroseconds(10);
        digitalWrite(trig, LOW);
        long d = pulseIn(echo, HIGH, 30000UL);
        v[i] = (d > 0) ? (d * 34L / 2000L) : 999L;
        delay(8);
    }
    for (uint8_t i = 0; i < N - 1; i++)
        for (uint8_t j = i + 1; j < N; j++)
            if (v[i] > v[j]) { long t = v[i]; v[i] = v[j]; v[j] = t; }
    return v[N / 2];
}

/* -------------------------------------------
   LEVEL %
   Sensor top-down, empty=30cm, full=10cm
   dist=30 -> 0%
   dist=10 -> 100%
   dist=20 -> 50%
   Formula: (BIN_DEPTH_CM - dist) / USABLE_CM * 100
   Clamped 0-100%
   ------------------------------------------- */
int levelPct(long dist)
{
    // clamp dist to valid range
    long d = constrain(dist, (long)FULL_CM, (long)BIN_DEPTH_CM);
    // filled = how much of the usable range is occupied
    long filled = BIN_DEPTH_CM - d;
    return (int)((filled * 100L) / USABLE_CM);
}

/* -------------------------------------------
   LEVEL BAR: 8 segments e.g. [====    ]
   ------------------------------------------- */
String levelBar(long dist)
{
    int pct  = levelPct(dist);
    int segs = (pct * 8) / 100;    // accurate segment count
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
   ------------------------------------------- */
void updateDistances()
{
    if (millis() - lastUSRead < US_INTERVAL_MS) return;
    lastUSRead = millis();

    bioDist = readDist(PIN_TRIG_BIO, PIN_ECHO_BIO);
    nonDist = readDist(PIN_TRIG_NON, PIN_ECHO_NON);

    // ---- BIO BIN ----
    if (!bioLocked) {
        if (bioDist <= FULL_CM) { bioFullCnt++;  bioEmptyCnt = 0; }
        else                    { bioFullCnt = 0; }

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
        if (bioDist >= EMPTY_CM) { bioEmptyCnt++;  bioFullCnt = 0; }
        else                     { bioEmptyCnt = 0; }

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
        if (nonDist <= FULL_CM) { nonFullCnt++;  nonEmptyCnt = 0; }
        else                    { nonFullCnt = 0; }

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
        if (nonDist >= EMPTY_CM) { nonEmptyCnt++;  nonFullCnt = 0; }
        else                     { nonEmptyCnt = 0; }

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
   LCD LAYOUT (16x2)
   Line 0:  "BIO  WASTE   XX%"
   Line 1:  "[=======  ] 15cm"
   ------------------------------------------- */
void updateLCD()
{
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate < 500UL) return;
    lastUpdate = millis();

    int bioPct = levelPct(bioDist);
    int nonPct = levelPct(nonDist);

    // ---- BIO LCD (lcd1) ----
    lcd1.setCursor(0, 0);
    lcd1.print(F("BIO  WASTE  "));
    if      (bioPct < 10)  lcd1.print(F("  "));
    else if (bioPct < 100) lcd1.print(F(" "));
    lcd1.print(bioPct);
    lcd1.print('%');

    lcd1.setCursor(0, 1);
    lcd1.print(levelBar(bioDist));          // 10 chars: [========]
    if (bioLocked) {
        lcd1.print(F(" FULL "));
    } else {
        lcd1.print(F(" "));
        if (bioDist < 10)       lcd1.print(F("  "));
        else if (bioDist < 100) lcd1.print(F(" "));
        lcd1.print(bioDist);
        lcd1.print(F("cm"));
    }

    // ---- NON-BIO LCD (lcd2) ----
    lcd2.setCursor(0, 0);
    lcd2.print(F("NON-BIO     "));
    if      (nonPct < 10)  lcd2.print(F("  "));
    else if (nonPct < 100) lcd2.print(F(" "));
    lcd2.print(nonPct);
    lcd2.print('%');

    lcd2.setCursor(0, 1);
    lcd2.print(levelBar(nonDist));
    if (nonLocked) {
        lcd2.print(F(" FULL "));
    } else {
        lcd2.print(F(" "));
        if (nonDist < 10)       lcd2.print(F("  "));
        else if (nonDist < 100) lcd2.print(F(" "));
        lcd2.print(nonDist);
        lcd2.print(F("cm"));
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

    // Boot: LOCKED(90) first then OPEN(0) for guaranteed movement
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
        Serial.print(F("Empty = ")); Serial.print(BIN_DEPTH_CM); Serial.println(F("cm (0%)"));
        Serial.print(F("Full  = ")); Serial.print(FULL_CM);      Serial.println(F("cm (100%)"));
        Serial.print(F("Unlock>= ")); Serial.print(EMPTY_CM);    Serial.println(F("cm"));
        Serial.print(F("Usable: ")); Serial.print(USABLE_CM);    Serial.println(F("cm range"));
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
        Serial.print(levelPct(bioDist)); Serial.print(F("% "));
        Serial.print(bioLocked ? F("LOCKED") : F("open"));
        Serial.print(F("  | NON-BIO "));
        Serial.print(nonDist); Serial.print(F("cm "));
        Serial.print(levelPct(nonDist)); Serial.print(F("% "));
        Serial.println(nonLocked ? F("LOCKED") : F("open"));
        Serial.print(F("Bio SMS today: ")); Serial.print(bioSMSCount);
        Serial.print(F("  Non SMS today: ")); Serial.println(nonSMSCount);
        if (lightSensorOK) { Serial.print(F("Lux: ")); Serial.println(currentLux); }
    }
#endif

    delay(100);
}