#ifndef SMART_BIN_H
#define SMART_BIN_H

/*
 * SMART WASTE BIN SYSTEM v3.1
 * Header - config, pins, externs
 *
 * Sensor mounted TOP-DOWN from lid
 * Empty = 30cm (sensor to bin bottom)
 * Full  = 10cm (trash close to sensor)
 */

#include <Arduino.h>
#include <TinyGPS++.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <BH1750.h>
#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>

/* -------------------------------------------
   CONFIG
   ------------------------------------------- */
#define DEBUG_MODE          true

// Sensor reads 30cm when empty, 10cm when full
#define BIN_DEPTH_CM        30      // distance when bin is empty
#define FULL_CM             10      // <= this -> FULL (lock)
#define EMPTY_CM            15      // >= this -> EMPTY again (hysteresis)

// Level % formula: (30 - dist) / (30 - 10) * 100
// dist=30 -> 0%,  dist=10 -> 100%
#define USABLE_CM           (BIN_DEPTH_CM - FULL_CM)   // 20cm usable range

#define US_INTERVAL_MS      3000UL
#define CONFIRM_NEEDED      3

#define SMS_INTERVAL_MS     28800000UL
#define MAX_SMS_PER_DAY     3
#define DAY_RESET_MS        86400000UL

#define LUX_THRESHOLD       50.0f

/* -------------------------------------------
   PIN MAP
   ------------------------------------------- */
static const uint8_t PIN_SERVO_BIO   = 6;
static const uint8_t PIN_SERVO_NON   = 7;
static const uint8_t PIN_BUZZER      = 2;
static const uint8_t PIN_RELAY_LED   = 3;
static const uint8_t PIN_SIM_RX      = 4;
static const uint8_t PIN_SIM_TX      = 5;
static const uint8_t PIN_RFID_BIO_SS = 8;
static const uint8_t PIN_RFID_NON_SS = 10;
static const uint8_t PIN_RFID_RST    = 9;
static const uint8_t PIN_TRIG_BIO    = A0;
static const uint8_t PIN_ECHO_BIO    = A1;
static const uint8_t PIN_TRIG_NON    = A2;
static const uint8_t PIN_ECHO_NON    = A3;

/* -------------------------------------------
   SERVO POSITIONS
   0  = OPEN (unlocked)
   90 = CLOSED (locked)
   ------------------------------------------- */
static const int SERVO_LOCKED   = 90;
static const int SERVO_UNLOCKED = 0;

/* -------------------------------------------
   RFID - authorized card UIDs
   ------------------------------------------- */
static const char AUTH_UID1[] = "43 FE B5 38";
static const char AUTH_UID2[] = "F3 37 B3 39";

/* -------------------------------------------
   PHONE NUMBER
   ------------------------------------------- */
static const char PHONE[] = "+639567669410";

/* -------------------------------------------
   HARDWARE OBJECT DECLARATIONS
   ------------------------------------------- */
extern TinyGPSPlus        gps;
extern LiquidCrystal_I2C  lcd1;
extern LiquidCrystal_I2C  lcd2;
extern BH1750             lightMeter;
extern SoftwareSerial     sim800;
extern Servo              servoBio;
extern Servo              servoNon;
extern MFRC522            rfidBio;
extern MFRC522            rfidNonBio;

/* -------------------------------------------
   STATE VARIABLE DECLARATIONS
   ------------------------------------------- */
extern bool           lightSensorOK;
extern float          currentLux;
extern bool           ambientLEDOn;
extern unsigned long  lastLuxRead;

extern bool           bioLocked;
extern bool           nonLocked;

extern long           bioDist;
extern long           nonDist;
extern unsigned long  lastUSRead;

extern int            bioFullCnt;
extern int            bioEmptyCnt;
extern int            nonFullCnt;
extern int            nonEmptyCnt;

extern unsigned long  bioLastSMSTime;
extern unsigned long  nonLastSMSTime;
extern int            bioSMSCount;
extern int            nonSMSCount;
extern unsigned long  dayStart;
extern unsigned long  lastDailySMS;

/* -------------------------------------------
   FUNCTION DECLARATIONS
   ------------------------------------------- */
void    sendSMS(const char* msg);
String  gpsStr();
int     getSignal();
long    readDist(uint8_t trig, uint8_t echo);
int     levelPct(long dist);
String  levelBar(long dist);
void    servoForceOpen(Servo &srv);

void    updateDistances();
void    checkRepeatSMS();

String  getUID(MFRC522 &r);
void    processCard(String uid, LiquidCrystal_I2C &lcd, bool isBioReader);
void    checkRFID();

void    updateLight();
void    updateLCD();

#endif // SMART_BIN_H