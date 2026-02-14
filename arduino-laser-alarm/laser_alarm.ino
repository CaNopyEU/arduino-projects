// Laser Security Alarm System
// MEGA2560 + Laser + Photoresistor + PIR + RFID RC522 + LCD1602 + RGB LED + Buzzer
//
// Features:
//   - Laser beam tripwire (laser → photoresistor)
//   - PIR motion detection (second layer)
//   - RFID card to arm/disarm the system
//   - LCD shows status and events
//   - RGB LED: green=disarmed, blue=armed, red=alarm
//   - Buzzer sounds on intrusion

#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>

// --- Pin definitions ---

// RFID RC522 (SPI)
const int RFID_SS   = 53;  // SDA
const int RFID_RST  = 5;

// LCD1602 (4-bit mode)
const int LCD_RS = 22;
const int LCD_EN = 23;
const int LCD_D4 = 24;
const int LCD_D5 = 25;
const int LCD_D6 = 26;
const int LCD_D7 = 27;

// Sensors
const int PHOTO_PIN  = A0;   // photoresistor (analog)
const int PIR_PIN    = 2;    // HC-SR501 (digital)
const int LASER_PIN  = 8;    // laser module on/off

// Outputs
const int BUZZER_PIN = 3;
const int LED_R      = 9;
const int LED_G      = 10;
const int LED_B      = 11;

// --- Configuration ---
const int LASER_THRESHOLD   = 800;  // below this = beam broken (calibrate!)
const unsigned long ALARM_DURATION_MS  = 5000;
const unsigned long LCD_REFRESH_MS     = 250;

// --- Authorized RFID UIDs ---
// Add your card UIDs here (up to 4 cards)
// To find your UID: arm the system, scan card, check Serial Monitor
const int MAX_CARDS = 4;
byte authorizedUIDs[MAX_CARDS][4] = {
  {0xDE, 0xAD, 0xBE, 0xEF},  // Card 1 - replace with your UID
  {0x00, 0x00, 0x00, 0x00},   // Card 2
  {0x00, 0x00, 0x00, 0x00},   // Card 3
  {0x00, 0x00, 0x00, 0x00},   // Card 4
};
int registeredCards = 1;  // how many cards are actually registered

// --- State ---
enum SystemState { DISARMED, ARMED, ALARM };
SystemState state = DISARMED;

unsigned long alarmStartTime  = 0;
unsigned long lastLCDRefresh  = 0;
bool laserTripped = false;
bool pirTripped   = false;

// --- Objects ---
MFRC522 rfid(RFID_SS, RFID_RST);
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// Custom lock icons
byte lockOpen[8]   = {0b01110, 0b10000, 0b10000, 0b11111, 0b11111, 0b11111, 0b11111, 0b01110};
byte lockClosed[8] = {0b01110, 0b10001, 0b10001, 0b11111, 0b11111, 0b11111, 0b11111, 0b01110};
byte alertIcon[8]  = {0b00100, 0b01110, 0b01110, 0b01110, 0b11111, 0b11111, 0b00100, 0b00100};

void setup() {
  Serial.begin(9600);

  // Pins
  pinMode(LASER_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  // Init RFID
  SPI.begin();
  rfid.PCD_Init();

  // Init LCD
  lcd.begin(16, 2);
  lcd.createChar(0, lockOpen);
  lcd.createChar(1, lockClosed);
  lcd.createChar(2, alertIcon);

  // Startup
  setLED(0, 255, 0);  // green = disarmed
  digitalWrite(LASER_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SECURITY SYSTEM");
  lcd.setCursor(0, 1);
  lcd.print("Scan card...");

  Serial.println("=== Laser Security System ===");
  Serial.println("Scan RFID card to arm/disarm.");
  Serial.println("Unknown cards will print their UID.");

  // Wait for PIR to stabilize (takes ~30-60s, we wait 5s minimum)
  delay(3000);
}

void loop() {
  checkRFID();

  if (state == ARMED) {
    checkSensors();
  }

  if (state == ALARM) {
    runAlarm();
  }

  updateLCD();
}

// --- RFID ---

void checkRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  // Print UID to Serial (for registration)
  Serial.print("Card scanned: UID = ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    Serial.print(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) Serial.print(":");
  }
  Serial.println();

  if (isAuthorized()) {
    toggleArm();
  } else {
    rejectCard();
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

bool isAuthorized() {
  for (int c = 0; c < registeredCards; c++) {
    bool match = true;
    for (byte i = 0; i < 4; i++) {
      if (rfid.uid.uidByte[i] != authorizedUIDs[c][i]) {
        match = false;
        break;
      }
    }
    if (match) return true;
  }
  return false;
}

void toggleArm() {
  if (state == ALARM || state == ARMED) {
    // Disarm
    state = DISARMED;
    digitalWrite(LASER_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    setLED(0, 255, 0);  // green
    laserTripped = false;
    pirTripped = false;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(byte(0));  // lock open icon
    lcd.print(" DISARMED");
    lcd.setCursor(0, 1);
    lcd.print("System off");

    Serial.println(">> DISARMED");
    tone(BUZZER_PIN, 800, 100);
    delay(200);
    tone(BUZZER_PIN, 600, 100);
    delay(200);
    noTone(BUZZER_PIN);
  } else {
    // Arm - countdown
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ARMING...");

    for (int i = 3; i > 0; i--) {
      lcd.setCursor(0, 1);
      lcd.print("in ");
      lcd.print(i);
      lcd.print(" sec   ");
      tone(BUZZER_PIN, 1000, 100);
      delay(1000);
    }
    noTone(BUZZER_PIN);

    state = ARMED;
    digitalWrite(LASER_PIN, HIGH);  // turn on laser
    setLED(0, 0, 255);  // blue
    laserTripped = false;
    pirTripped = false;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(byte(1));  // lock closed icon
    lcd.print(" ARMED");
    lcd.setCursor(0, 1);
    lcd.print("Monitoring...");

    Serial.println(">> ARMED - Laser ON, monitoring...");
    tone(BUZZER_PIN, 1200, 200);
    delay(200);
    noTone(BUZZER_PIN);
  }
}

void rejectCard() {
  Serial.println(">> ACCESS DENIED - unknown card");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ACCESS DENIED");
  lcd.setCursor(0, 1);
  lcd.print("Unknown card!");

  setLED(255, 80, 0);  // orange flash
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 200, 150);
    delay(250);
  }
  noTone(BUZZER_PIN);
  delay(1000);

  // Restore previous LED
  if (state == ARMED) setLED(0, 0, 255);
  else setLED(0, 255, 0);
}

// --- Sensors ---

void checkSensors() {
  // Laser tripwire
  int lightLevel = analogRead(PHOTO_PIN);
  if (lightLevel < LASER_THRESHOLD) {
    if (!laserTripped) {
      laserTripped = true;
      Serial.print("!! LASER BEAM BROKEN - light level: ");
      Serial.println(lightLevel);
      triggerAlarm("LASER TRIP");
    }
  }

  // PIR motion
  if (digitalRead(PIR_PIN) == HIGH) {
    if (!pirTripped) {
      pirTripped = true;
      Serial.println("!! MOTION DETECTED by PIR");
      triggerAlarm("MOTION DET");
    }
  }
}

void triggerAlarm(const char* source) {
  state = ALARM;
  alarmStartTime = millis();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(byte(2));  // alert icon
  lcd.print(" INTRUSION!");
  lcd.setCursor(0, 1);
  lcd.print(source);

  Serial.print("!! ALARM TRIGGERED: ");
  Serial.println(source);
}

void runAlarm() {
  // Alternating red flash + siren
  unsigned long elapsed = millis() - alarmStartTime;
  bool tick = (elapsed / 200) % 2;

  if (tick) {
    setLED(255, 0, 0);
    tone(BUZZER_PIN, 2000);
  } else {
    setLED(0, 0, 0);
    tone(BUZZER_PIN, 1500);
  }

  // Auto-reset after duration, go back to ARMED
  if (elapsed > ALARM_DURATION_MS) {
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);
    state = ARMED;
    setLED(0, 0, 255);
    laserTripped = false;
    pirTripped = false;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(byte(1));
    lcd.print(" ARMED");
    lcd.setCursor(0, 1);
    lcd.print("Reset - watching");

    Serial.println(">> Alarm reset, back to ARMED");
  }
}

// --- Display ---

void updateLCD() {
  if (state != ARMED) return;

  unsigned long now = millis();
  if (now - lastLCDRefresh < LCD_REFRESH_MS) return;
  lastLCDRefresh = now;

  int lightLevel = analogRead(PHOTO_PIN);

  lcd.setCursor(0, 1);
  lcd.print("L:");
  lcd.print(lightLevel);
  lcd.print("  ");

  // Show PIR state
  lcd.setCursor(9, 1);
  lcd.print("PIR:");
  lcd.print(digitalRead(PIR_PIN) ? "!" : ".");
  lcd.print(" ");
}

// --- Helpers ---

void setLED(int r, int g, int b) {
  analogWrite(LED_R, r);
  analogWrite(LED_G, g);
  analogWrite(LED_B, b);
}
