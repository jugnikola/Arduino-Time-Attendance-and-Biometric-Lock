/*  "FINALNA VERZIJA"

    Projekt:
    Sustav autorizacije korisnika i zabilježbe vremena dolaska.
    Koristi senzor otiska prsta, čitač RFID kartica, RTC modul i modul za rad s SD karticom.

    Autori: Nikola Jug, Karlo Rusovan
    2021.
*/

/* Spajanje - PINOUT

    Arduino
    PIN -> Modul
	1 -> lcd(RS)
    2 -> fingerprint (Tx) - žuta
    3 -> fingerprint (Rx) - bijela

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

    A0 -> brava
    A4 -> rtc(SDA)
    A5 -> rtc(SCK)
	*/

/*Funkcije biblioteke RTC-a

  rtc.getDay()
  rtc.getMonth()
  rtc.getYear()
  rtc.getHour()
  rtc.getMinute()
  rtc.getSecond()
*/
/*
   Nikola - ID prsta:
    1 - desni palac
    2 - desni kažiprst
    3 - desni srednjak
    4 - desni prstenjak
    5 - desni mali prst
    6 - lijevi palac
    7 - lijevi kažiprst
    8 - lijevi srednjak -- IZBRISANO
    9 - lijevi prstenjak
    10 - lijevi mali prst

    Karlo - ID prsta:
    11 - desni palac
    12 - desni kažiprst
    13 - desni srednjak
    14 - desni prstenjak
    15 - desni mali prst
    16 - lijevi palac -- IZBRISANO
    17 - lijevi kažiprst
    18 - lijevi srednjak
    19 - lijevi prstenjak
    20 - lijevi mali prst
*/
// Biblioteke
#include "SPI.h" // Biblioteka za SPI komunikaciju
#include <MFRC522.h> // Biblioteka RFID čitača
#include <PCF85063A.h> // Biblioteka RTC modula
#include <LiquidCrystal.h> // Biblioteka LCDa
#include <Adafruit_Fingerprint.h> // Biblioteka senzora otiska prsta
#include "SD.h"  // Biblioteka za rad s SD karticom


// Definiranje konstanti pinova na koje su spojeni RFID i SD modul
// rfid_rst = RST (modul MFRC522)
// rfid_cs = CS (modul MFRC522)
// sd_cs = CS (modul SD kartice)
#define rfid_rst 9
#define rfid_cs 4
#define sd_cs 10
#define brava A0

MFRC522 mfrc522(rfid_cs, rfid_rst); // Potrebno za rad s RFID čitačem
PCF85063A rtc; // potrebno zbog biblioteke RTC modula
LiquidCrystal lcd(1, 8, 0, 5, 6, 7); // Kreiranje LC objekta, parametri: (rs, enable, d4, d5, d6, d7)
SoftwareSerial mySerial(2, 3); // Senzor otiska prsta komunicira putem softverske serijalne veze, definiranje pinova
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

/* deklariranje varijabli
   ID prsta
   String vrijednosti koje vraćaju određene RFID kartice
*/

uint8_t nikola_id[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
uint8_t karlo_id[] = {11, 12, 13, 14, 15, 16, 17, 18, 19, 20};

String read_rfid = "";
String rfid_nikola = "c0555732";
String rfid_karlo = "a9b51eb3";

// Deklariranje objekta File koji služi za zapis na SD karticu
File datoteka;

void setup() {
  // serijska komunikacija, koristi se samo za debuggiranje
  //Serial.begin(9600);

  // definiranje izlaznih Chip Select pinova za RFID i SD module
  pinMode(rfid_cs, OUTPUT);
  pinMode(sd_cs, OUTPUT);

  // definiranje izlaznog pina koji aktivira elektroničku bravu
  pinMode(brava, OUTPUT);

  // Inicijalizacija SPI komunikacije, RFID modula i LCDa
  SPI.begin();
  mfrc522.PCD_Init();
  lcd.begin(16, 2);  // Inicijalizacija LCDa - definiranje dimenzija ekrana
  finger.begin(57600); // Inicijalizacija komunikacije sa senzorom otiska prsta

  // Provjera ako je otisak prsta ispravno povezan i ako je lozinka ispravna - lozinka se postavlja u datoteci biblioteke za rad sa senzorom
  if (finger.verifyPassword()) {
    lcd.print("Senzor otiska");
    lcd.setCursor(0, 1);
    lcd.print("povezan.");
    delay(1000);
  } else {
    //ako je lozinka netočna ili nije pronađen senzor ispisujemo poruku i ostajemo u beskonačnoj while petlji
    lcd.print("Senzor otiska");
    lcd.setCursor(0, 1);
    lcd.print("nije povezan.");
    delay(1000);
    while (1) while (1) {
        delay(1);
      }
  }
  // Inicijalizacija SD kartice
  lcd.clear();
  lcd.print("Inicijalizacija");
  lcd.setCursor(0, 1);
  lcd.print("SD kartice");
  //Serial.println("Inicijalizacija SD kartice...");
  delay(2000);
  if (!SD.begin(sd_cs)) {
    //Serial.println("Inicijalizacija nije uspjela!");
    lcd.clear();
    lcd.print("nije");
    lcd.setCursor(0, 1);
    lcd.print("uspjela");
    delay(2000);
    lcd.clear();
    lcd.print("Molim");
    lcd.setCursor(0, 1);
    lcd.print("resetirajte");
    delay(5600);
    return;
  }
  //Serial.println("initialization done.");
  lcd.clear();
  lcd.print("Uspjesno");
  lcd.setCursor(0, 1);
  lcd.print("inicijalizirana");
  delay(1500);
  lcd.clear();
}


// Funkcija koja čita vrijednosti na RFID kartici i vraća jedinstveni string(hex kod) za svaku karticu
void dump_byte_array(byte * buffer, byte bufferSize) {
  read_rfid = "";
  for (byte i = 0; i < bufferSize; i++) {
    read_rfid += String(buffer[i], HEX);
  }
}

// Funkcija koja se poziva za provjeru autorizacije korisnika
void autoriziraj(String read_rfid) {

  lcd.print("Otisak:");
  delay(1000);

  // deklariranje varijable u koju se sprema ID prsta
  uint8_t id;
  id = getFingerprintIDez();

  // pohrana trenutne vrijednosti sekunda
  uint8_t vrijeme = rtc.getSecond();

  while (id == 255) {
    // sve dok senzor vraća 255 (-1 u unsigned tipu varijable) - dohvati novi ID sa senzora
    id = getFingerprintIDez();
    delay(50);
    // ova varijabla sluzi za usporedbu vremena
    uint8_t vrijeme2 = rtc.getSecond();

    // ovaj dio koda ceka 10 sekundi na otisak prsta, te nakon isteka vremena izade iz petlje
    if ((vrijeme2 - vrijeme) > 10) {
      lcd.clear();
      lcd.print("Otisak nije");
      lcd.setCursor(0, 1);
      lcd.print("pronaden");
      delay(3000);
      lcd.clear();
      break;
    }
  }

  lcd.clear();
  // Ispis ID-a otiska koji je prislonjen na senzor
  lcd.print("ID otiska:");
  lcd.setCursor(0, 1);
  lcd.print(id);
  delay(1500);

  // logika određivanja identiteta i autorizacije
  for (int i = 0; i < 21; i++) {
    if ( id == i && i < 11 && read_rfid == rfid_nikola ) {
      lcd.clear();
      lcd.print("Dobrodosao");
      lcd.setCursor(0, 1);
      lcd.print("Nikola!");
      digitalWrite(brava, LOW);
      delay(4000);
      digitalWrite(brava, HIGH);
      delay(2000);
      ispisVremena();
      zapisSD();
      break;
    }
    else if ( id == i && i > 10 && read_rfid == rfid_karlo) {
      lcd.clear();
      lcd.print("Dobrodosao");
      lcd.setCursor(0, 1);
      lcd.print("Karlo!");
      digitalWrite(brava, LOW);
      delay(4000);
      digitalWrite(brava, HIGH);
      delay(2000);
      ispisVremena();
      zapisSD();
      break;
    }
    else if ( (id == i && i > 10 && read_rfid == rfid_nikola) ||  ( id == i && i < 11 && read_rfid == rfid_karlo ) ) {
      lcd.clear();
      lcd.print("Otisak ne");
      lcd.setCursor(0, 1);
      lcd.print("odgovara kartici");
      delay(2000);
      lcd.clear();
      break;
    }
  }
}

void ispisVremena() {
  lcd.clear();
  lcd.print("Vrijeme:");
  lcd.setCursor(0, 1);
  lcd.print(rtc.getHour());
  lcd.print(":");
  lcd.print(rtc.getMinute());
  delay(3000);
}


void odbijPristup() {
  lcd.clear();
  lcd.print("Kartica nije");
  lcd.setCursor(0, 1);
  lcd.print("prepoznata");
  delay(3000);
  lcd.clear();
}

// Funkcija koja vraća integer vrijednost otiska prsta koji je prislonjen na senzor
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) return -1;

  // Serial.print("Pronadjen ID #");
  // Serial.print(finger.fingerID); //ispisujemo ID otiska prsta
  // Serial.print(" s pouzdanjem ");
  // Serial.println(finger.confidence);//ispisujemo koliko je pouzdan pročitan otisak s određenim otiskom iz baze
  return finger.fingerID; //funkcija vraća ID otiska prsta
}

void zapisSD() {

  // Postavljanje chip select pina modula SD kartice na LOW - uključuje se komunikacija sa SD modulom
  digitalWrite(rfid_cs, HIGH);
  digitalWrite(sd_cs, LOW);
  delay(10);
  // Otvaranje datoteke
  datoteka = SD.open("podaci.csv", FILE_WRITE);
  delay(10);

  // Ako je datoteka uspjesno otvorena, u nju zapisujemo podatke
  if (datoteka) {
    datoteka.print(read_rfid);
    datoteka.print(", ");
    datoteka.print(rtc.getYear());
    datoteka.print("-");
    datoteka.print(rtc.getMonth());
    datoteka.print("-");
    datoteka.print(rtc.getDay());
    datoteka.print(", ");
    datoteka.print(rtc.getHour());
    datoteka.print(":");
    datoteka.print(rtc.getMinute());
    datoteka.print(":");
    datoteka.println(rtc.getSecond());
    datoteka.close();
    lcd.clear();
    lcd.print("Podaci");
    lcd.setCursor(0, 1);
    lcd.print("zapisani");
    delay(2000);
  } else {
    //Serial.println("Greška pri otvaranju datoteke.");
    lcd.clear();
    lcd.print("Zapis nije");
    lcd.setCursor(0, 1);
    lcd.print("uspio");
    delay(2000);
  }
  // Postavljanje CS pina modula SD kartice na HIGH - isključuje se komunikacija sa SD modulom
  digitalWrite(sd_cs, HIGH);
  digitalWrite(rfid_cs, LOW);
}


void loop() {
  digitalWrite(brava, HIGH);
  lcd.print("Kartica:");
  //Serial.println("Kartica:");
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
    odbijPristup();
  }

  lcd.clear();
}
