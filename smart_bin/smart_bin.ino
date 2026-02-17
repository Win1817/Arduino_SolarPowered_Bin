/*
 * SMART WASTE BIN SYSTEM v3.1
 * smart_bin.ino - Arduino IDE entry point
 *
 * PIN MAP:
 *   D0      GPS TX  ** DISCONNECT BEFORE UPLOAD **
 *   D1      GPS RX
 *   D2      Buzzer
 *   D3      Relay (LED strip)
 *   D4      SIM800 RX (SoftwareSerial)
 *   D5      SIM800 TX (SoftwareSerial)
 *   D6      Servo Bio
 *   D7      Servo NonBio
 *   D8      RFID Bio SS
 *   D9      RFID Shared RST
 *   D10     RFID NonBio SS
 *   D11     SPI MOSI
 *   D12     SPI MISO
 *   D13     SPI SCK
 *   A0/A1   Ultrasonic Bio Trig/Echo
 *   A2/A3   Ultrasonic NonBio Trig/Echo
 *   A4/A5   I2C (LCD1, LCD2, BH1750)
 *
 * All logic is in smart_bin.cpp / smart_bin.h
 * setup() and loop() are defined in smart_bin.cpp
 */

#include "smart_bin.h"