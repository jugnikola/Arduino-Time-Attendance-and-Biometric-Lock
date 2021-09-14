 /*	Version 0.1 - working release (:

  Project:
  Biometric lock and attendance system which uses a fingerprint sensor, RFID card reader, RTC module and SD card module.
	
	Authors: Nikola Jug, Karlo Rusovan
	2021.

	PINOUT

    Arduino
    PIN -> Module
	1 -> lcd(RS)
    2 -> fingerprint (Tx) - yellow
    3 -> fingerprint (Rx) - white

    4 -> rfid(SDA/CS)
    5 -> lcd(D5)
    6 -> lcd (D6)
    7 -> lcd(D7)
    8 -> lcd(Enable)

    9 -> rfid(RST)

    10 -> sd(CS)
    11 -> MOSI
    12 -> MISO
    13 -> SCK

    A0 -> Solenoid lock
    A4 -> rtc(SDA)
    A5 -> rtc(SCK)
	*/

/* RTC Library functions

  rtc.getDay()
  rtc.getMonth()
  rtc.getYear()
  rtc.getHour()
  rtc.getMinute()
  rtc.getSecond()
*/
/*
   Nikola - finger ID:
    1 - right thumb
    2 - right index
    3 - right middle

    Karlo - finger ID:
    11 - right thumb
    12 - right index
    13 - right middle
*/
// Libraries
#include "SPI.h" // SPI Library
#include <MFRC522.h> // RFID reader Library
#include <PCF85063A.h> // RTC Library
#include <LiquidCrystal.h> // LCD Library
#include <Adafruit_Fingerprint.h> // Fingerprint sensor Library
#include "SD.h"  // SD card Library

/* Constants for module pins
	rfid_rst = RST (module MFRC522)
	rfid_cs = CS (module MFRC522)
 	sd_cs = CS (SD module) */
#define rfid_rst 9
#define rfid_cs 4
#define sd_cs 10
#define lock A0

MFRC522 mfrc522(rfid_cs, rfid_rst); // Initializing needed for RFID module
PCF85063A rtc; // RTC chip model
LiquidCrystal lcd(1, 8, 0, 5, 6, 7); // Creating an lcd object, parameters: (rs, enable, d4, d5, d6, d7)
SoftwareSerial mySerial(2, 3); // Defining software serial pins for fingerprint sensor
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

/* Declaring variables
   Finger IDs
   Strings that cards and fobs return
*/

uint8_t nikola_id[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
uint8_t karlo_id[] = {11, 12, 13, 14, 15, 16, 17, 18, 19, 20};

String read_rfid = "";
String rfid_nikola = "c0555732";
String rfid_karlo = "a9b51eb3";

// Instancing a new File object for writing to SD
File file;

void setup() {
  // serial communication useful for debugging
  // Serial.begin(9600);

  // Chip Select pins of the RFDI and SD module
  pinMode(rfid_cs, OUTPUT);
  pinMode(sd_cs, OUTPUT);

  // output pin which activates the lock
  pinMode(lock, OUTPUT);

  // Initializing SPI comms, RFID and LCD
  SPI.begin();
  mfrc522.PCD_Init();
  lcd.begin(16, 2);  // Dimensions of the LCD
  finger.begin(57600); // Initializing the communication with the fingerprint reader

  // Checks if the fingerprint sensor is connected and if the password is correct - password is set in the library file.
  if (finger.verifyPassword()) {
    lcd.print("Fingerp. sensor");
    lcd.setCursor(0, 1);
    lcd.print("connected.");
    delay(1000);
  } else {
	// if the password is incorrect, print a message and stay in an endless loop
    lcd.print("Fingerp. sensor");
    lcd.setCursor(0, 1);
    lcd.print("not connected.");
    delay(1000);
    while (1) while (1) {
        delay(1);
      }
  }
  // Initializing SD card
  lcd.clear();
  lcd.print("Initializing");
  lcd.setCursor(0, 1);
  lcd.print("SD card");
  //Serial.println("Initializing SD card...");
  delay(2000);
  if (!SD.begin(sd_cs)) {
    //Serial.println("Initialization failed!");
    lcd.clear();
    lcd.print("Initialization");
    lcd.setCursor(0, 1);
    lcd.print("failed");
    delay(2000);
    lcd.clear();
    lcd.print("Please");
    lcd.setCursor(0, 1);
    lcd.print("reset device");
    delay(5600);
    return;
  }
  //Serial.println("Initialization done.");
  lcd.clear();
  lcd.print("Initialization");
  lcd.setCursor(0, 1);
  lcd.print("successful");
  delay(1500);
  lcd.clear();
}

// Function which reads the value on an RFID card and returns a unique string in HEX for each card
void dump_byte_array(byte * buffer, byte bufferSize) {
  read_rfid = "";
  for (byte i = 0; i < bufferSize; i++) {
    read_rfid += String(buffer[i], HEX);
  }
}

// Function which is called to authorize the user
void autoriziraj(String read_rfid) {

  lcd.print("Finger:");
  delay(1000);

  // A variable to store the finger ID
  uint8_t id;
  id = getFingerprintIDez();

  // Storing the current seconds value
  uint8_t time = rtc.getSecond();

  while (id == 255) {
	// while the sensor returns 255 (or -1 in unsinged) - fetch a new ID off the sensor
    id = getFingerprintIDez();
    delay(50);
	// this variable is used for comparing the time
 	uint8_t time2 = rtc.getSecond();

	// wait 10 seconds for finger, after the time period exit the loop
    if ((time2 - time) > 10) {
      lcd.clear();
      lcd.print("Fingerp.");
      lcd.setCursor(0, 1);
      lcd.print("not found.");
      delay(3000);
      lcd.clear();
      break;
    }
  }

  lcd.clear();
  // Prints the finger ID
  lcd.print("Print ID:");
  lcd.setCursor(0, 1);
  lcd.print(id);
  delay(1500);

// Authorization logic
  for (int i = 0; i < 21; i++) {
    if ( id == i && i < 11 && read_rfid == rfid_nikola ) {
      lcd.clear();
      lcd.print("Welcome,");
      lcd.setCursor(0, 1);
      lcd.print("Nikola!");
      digitalWrite(lock, LOW);
      delay(4000);
      digitalWrite(lock, HIGH);
      delay(2000);
      printTime();
      writeSD();
      break;
    }
    else if ( id == i && i > 10 && read_rfid == rfid_karlo) {
      lcd.clear();
      lcd.print("Welcome");
      lcd.setCursor(0, 1);
      lcd.print("Karlo!");
      digitalWrite(lock, LOW);
      delay(4000);
      digitalWrite(lock, HIGH);
      delay(2000);
      printTime();
      writeSD();
      break;
    }
    else if ( (id == i && i > 10 && read_rfid == rfid_nikola) ||  ( id == i && i < 11 && read_rfid == rfid_karlo ) ) {
      lcd.clear();
      lcd.print("Print does not");
      lcd.setCursor(0, 1);
      lcd.print("match card.");
      delay(2000);
      lcd.clear();
      break;
    }
  }
}

void printTime() {
  lcd.clear();
  lcd.print("Time:");
  lcd.setCursor(0, 1);
  lcd.print(rtc.getHour());
  lcd.print(":");
  lcd.print(rtc.getMinute());
  delay(3000);
}

void denyAccess() {
  lcd.clear();
  lcd.print("Card not");
  lcd.setCursor(0, 1);
  lcd.print("recognized.");
  delay(3000);
  lcd.clear();
}

// Function which returns an integer value of the fingerprint on the sensor
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return -1;

  // Serial.print("Found ID #");
  // Serial.print(finger.fingerID); //prints the finger ID
  // Serial.print(" with confidence ");
  // Serial.println(finger.confidence); //prints the confidence of the match
  return finger.fingerID; //return the finger ID
}

void writeSD() {

  // Set the Chip Select pin of the SD on LOW - starting communication with the SD module 
  digitalWrite(rfid_cs, HIGH);
  digitalWrite(sd_cs, LOW);
  delay(10);
  // Open a file
  file = SD.open("podaci.csv", FILE_WRITE);
  delay(10);

  // If the file is successfully open, write
  if (file) {
    file.print(read_rfid);
    file.print(", ");
    file.print(rtc.getYear());
    file.print("-");
    file.print(rtc.getMonth());
    file.print("-");
    file.print(rtc.getDay());
    file.print(", ");
    file.print(rtc.getHour());
    file.print(":");
    file.print(rtc.getMinute());
    file.print(":");
    file.println(rtc.getSecond());
    file.close();
    lcd.clear();
    lcd.print("Podaci");
    lcd.setCursor(0, 1);
    lcd.print("zapisani");
    delay(2000);
  } else {
    //Serial.println("Error while opening file.");
    lcd.clear();
    lcd.print("Zapis nije");
    lcd.setCursor(0, 1);
    lcd.print("uspio");
    delay(2000);
  }
  // Set the CS pin on HIGH - stopping communication with the SD module
  digitalWrite(sd_cs, HIGH);
  digitalWrite(rfid_cs, LOW);
}

void loop() {
  digitalWrite(lock, HIGH);
  lcd.print("Card:");
  // Serial.println("Card:");
  delay(1000);
  lcd.clear();
  if ( ! mfrc522.PICC_IsNewCardPresent())
    return;

  if ( ! mfrc522.PICC_ReadCardSerial())
    return;

  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);

  if (read_rfid == rfid_nikola || read_rfid == rfid_karlo) {
    autoriziraj(read_rfid);
  }
  else {
    denyAccess();
  }

  lcd.clear();
}
