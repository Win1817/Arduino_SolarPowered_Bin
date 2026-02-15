#include <TinyGPS++.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <BH1750.h>
#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>

/* ===================== CONFIG ===================== */
#define DEBUG_MODE true
#define LUX_THRESHOLD 50.0
#define FULL_THRESHOLD 10
#define DAY_INTERVAL 86400000UL // 24h
#define DIST_SAMPLES 7

/* ===================== GPS ===================== */
TinyGPSPlus gps;

/* ===================== LCD ===================== */
LiquidCrystal_I2C lcd1(0x27,16,2);
LiquidCrystal_I2C lcd2(0x25,16,2);

/* ===================== BH1750 ===================== */
BH1750 lightMeter;
float currentLux = 0;
bool ambientLEDOn = false;
unsigned long lastLuxRead = 0;

/* ===================== RFID ===================== */
#define SHARED_RST_PIN 9
#define RFID_BIO_SS_PIN 8
#define RFID_NONBIO_SS_PIN 10
MFRC522 rfidBio(RFID_BIO_SS_PIN, SHARED_RST_PIN);
MFRC522 rfidNonBio(RFID_NONBIO_SS_PIN, SHARED_RST_PIN);

String authorizedUIDs[] = {"AA BB CC DD","11 22 33 44"};
String adminUID = "FF EE DD CC";

/* ===================== Ultrasonic ===================== */
const int trigBio = A0, echoBio = A1;
const int trigNon = A2, echoNon = A3;

long bioBuffer[DIST_SAMPLES]; int bioIndex=0;
long nonBuffer[DIST_SAMPLES]; int nonIndex=0;

/* ===================== Relay & Buzzer ===================== */
const int relayLED = 3;
const int buzzerPin = 2;

/* ===================== Servo ===================== */
Servo servoBio, servoNon;
const int SERVO_LOCKED=0;
const int SERVO_UNLOCKED=90;
bool bioLocked=false, nonLocked=false;

/* ===================== GSM ===================== */
SoftwareSerial sim800(4,5); // RX,TX
const char phoneNumber[] = "+639567669410";

/* ===================== Daily SMS ===================== */
unsigned long lastDailySMS = 0;

/* ===================== HELPER FUNCTIONS ===================== */
void sendSMS(const char* number,const char* message){
  sim800.println("AT+CMGF=1"); delay(200);
  sim800.print("AT+CMGS=\""); sim800.print(number); sim800.println("\""); delay(200);
  sim800.print(message); delay(200);
  sim800.write(26); delay(5000);
}

String getGPS(){
  if(gps.location.isValid()){
    return String(gps.location.lat(),6) + "," + String(gps.location.lng(),6);
  }
  return "NoGPS";
}

int getSignal(){
  sim800.println("AT+CSQ"); delay(300);
  String r=""; while(sim800.available()) r+=char(sim800.read());
  int i=r.indexOf("+CSQ:");
  if(i!=-1){
    int c=r.indexOf(",",i);
    return r.substring(i+6,c).toInt();
  }
  return 0;
}

String getUID(MFRC522 &rfid){
  String s="";
  for(byte i=0;i<rfid.uid.size;i++){
    s += String(rfid.uid.uidByte[i]<0x10?" 0":" ");
    s += String(rfid.uid.uidByte[i],HEX);
  }
  s.trim(); s.toUpperCase();
  return s;
}

void processCard(String uid){
  if(uid == adminUID){
    servoBio.write(SERVO_UNLOCKED);
    servoNon.write(SERVO_UNLOCKED);
    bioLocked=false; nonLocked=false;
    tone(buzzerPin,3000,200);
    if(DEBUG_MODE) Serial.println("ADMIN OPEN ALL BINS");
    return;
  }
  for(int i=0;i<2;i++){
    if(uid==authorizedUIDs[i]){
      tone(buzzerPin,2000,100);
      if(DEBUG_MODE) Serial.println("AUTHORIZED USER");
      return;
    }
  }
  tone(buzzerPin,500,200);
  if(DEBUG_MODE) Serial.println("UNAUTHORIZED USER");
}

void checkRFID(){
  if(rfidBio.PICC_IsNewCardPresent() && rfidBio.PICC_ReadCardSerial()){
    processCard(getUID(rfidBio));
    rfidBio.PICC_HaltA(); rfidBio.PCD_StopCrypto1();
  }
  if(rfidNonBio.PICC_IsNewCardPresent() && rfidNonBio.PICC_ReadCardSerial()){
    processCard(getUID(rfidNonBio));
    rfidNonBio.PICC_HaltA(); rfidNonBio.PCD_StopCrypto1();
  }
}

long readDistance(int trig,int echo){
  long readings[DIST_SAMPLES];
  for(int i=0;i<DIST_SAMPLES;i++){
    digitalWrite(trig,LOW); delayMicroseconds(2);
    digitalWrite(trig,HIGH); delayMicroseconds(10);
    digitalWrite(trig,LOW);
    long dur = pulseIn(echo,HIGH,30000);
    readings[i] = (dur>0)? dur*0.034/2 : 999;
    delay(5);
  }
  // median
  for(int i=0;i<DIST_SAMPLES-1;i++)
    for(int j=i+1;j<DIST_SAMPLES;j++)
      if(readings[i]>readings[j]){long t=readings[i]; readings[i]=readings[j]; readings[j]=t;}
  return readings[DIST_SAMPLES/2];
}

long getStable(long *buffer,int &index,long newVal){
  buffer[index] = newVal;
  index = (index+1)%DIST_SAMPLES;
  long sum=0;
  for(int i=0;i<DIST_SAMPLES;i++) sum+=buffer[i];
  return sum/DIST_SAMPLES;
}

void updateLight(){
  if(millis()-lastLuxRead>1000){
    lastLuxRead=millis();
    currentLux=lightMeter.readLightLevel();
    ambientLEDOn = currentLux < LUX_THRESHOLD;
    if(DEBUG_MODE) Serial.print("Lux: "); Serial.println(currentLux);
  }
}

void sendDaily(){
  String msg="DAILY STATUS\n";
  msg+="Bio:"+(String)(bioLocked?"FULL":"OK")+"\n";
  msg+="NonBio:"+(String)(nonLocked?"FULL":"OK")+"\n";
  msg+="Sig:"+String(getSignal())+"\n";
  msg+="GPS:"+getGPS();
  sendSMS(phoneNumber,msg.c_str());
  if(DEBUG_MODE) Serial.println("Daily SMS sent");
}

/* ===================== SETUP ===================== */
void setup(){
  Serial.begin(9600);
  Wire.begin();

  lcd1.init(); lcd1.backlight();
  lcd2.init(); lcd2.backlight();

  SPI.begin();
  rfidBio.PCD_Init();
  rfidNonBio.PCD_Init();

  pinMode(buzzerPin,OUTPUT);
  pinMode(trigBio,OUTPUT); pinMode(echoBio,INPUT);
  pinMode(trigNon,OUTPUT); pinMode(echoNon,INPUT);
  pinMode(relayLED,OUTPUT); digitalWrite(relayLED,LOW);

  servoBio.attach(6); servoNon.attach(7);
  servoBio.write(SERVO_UNLOCKED); servoNon.write(SERVO_UNLOCKED);

  lightMeter.begin();

  sim800.begin(9600);
  delay(3000);
  sim800.println("AT"); delay(500);
  sim800.println("ATE0"); delay(500);
  sim800.println("AT+CMGF=1"); delay(500);
  sim800.println("AT+CSCS=\"GSM\""); delay(500);
  sim800.println("AT+CSMP=17,167,0,0"); delay(500);

  lcd1.clear(); lcd2.clear();
  lcd1.print("SYSTEM READY");
  lcd2.print("SYSTEM READY");
  if(DEBUG_MODE) Serial.println("SYSTEM READY");
}

/* ===================== LOOP ===================== */
void loop(){
  // GPS
  while(Serial.available()) gps.encode(Serial.read());

  checkRFID();
  updateLight();

  // Read distances
  long dBio = getStable(bioBuffer,bioIndex,readDistance(trigBio,echoBio));
  long dNon = getStable(nonBuffer,nonIndex,readDistance(trigNon,echoNon));

  // Determine full status
  bool bioFull = dBio <= FULL_THRESHOLD;
  bool nonFull = dNon <= FULL_THRESHOLD;

  // Servo control
  if(bioFull && !bioLocked){ servoBio.write(SERVO_LOCKED); bioLocked=true; sendSMS(phoneNumber,("BIO FULL\nGPS:"+getGPS()).c_str()); }
  if(!bioFull && bioLocked){ servoBio.write(SERVO_UNLOCKED); bioLocked=false; }
  if(nonFull && !nonLocked){ servoNon.write(SERVO_LOCKED); nonLocked=true; sendSMS(phoneNumber,("NONBIO FULL\nGPS:"+getGPS()).c_str()); }
  if(!nonFull && nonLocked){ servoNon.write(SERVO_UNLOCKED); nonLocked=false; }

  // LED strip control
  if(ambientLEDOn || bioLocked || nonLocked) digitalWrite(relayLED,HIGH);
  else digitalWrite(relayLED,LOW);

  // LCD update
  int sig = getSignal();
  lcd1.setCursor(0,0); lcd1.print("BIO "); lcd1.print(bioLocked?"FULL ":"OK   "); lcd1.print("S:"); lcd1.print(sig);
  lcd2.setCursor(0,0); lcd2.print("NON "); lcd2.print(nonLocked?"FULL ":"OK   "); lcd2.print("S:"); lcd2.print(sig);

  // Daily SMS
  if(millis()-lastDailySMS>DAY_INTERVAL){ sendDaily(); lastDailySMS=millis(); }

  // Debug
  if(DEBUG_MODE){
    Serial.print("Bio: "); Serial.print(bioLocked); Serial.print(" Non: "); Serial.print(nonLocked);
    Serial.print(" Lux: "); Serial.print(currentLux); Serial.print(" Sig: "); Serial.println(sig);
  }

  delay(1000);
}
