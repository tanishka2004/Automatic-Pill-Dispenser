#include <Wire.h>
#include <DS3231.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

DS3231 myRTC;
Servo compartmentServo; // MG996R (360-degree) — rotates compartments
Servo gateServo;        // SG90 Servo #1 — opens/closes gate to pill tray
Servo missedServo;      // SG90 Servo #2 — opens/closes gate to missed compartment
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int buzzerPin = 8;
const int ledPin = 7;
const int trigPin = 6;
const int echoPin = 5;
const int buttonPin = 4;

const int gateServoPin = 9;
const int missedServoPin = 10;
const int compartmentServoPin = 3;

// Dose Timings
const int targetSecond1 = 10;
const int targetSecond2 = 20;
const int targetSecond3 = 35;
const int targetSecond4 = 45;

bool hasDispensed1 = false;
bool hasDispensed2 = false;
bool hasDispensed3 = false;
bool hasDispensed4 = false;

void setup() {
  Serial.begin(9600);
  Wire.begin();

  gateServo.attach(gateServoPin);
  missedServo.attach(missedServoPin);
  compartmentServo.attach(compartmentServoPin);

  gateServo.write(0);
  missedServo.write(0);

  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  lcd.begin(16, 2);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Pill Dispenser");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();
}

long readDistanceCM() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  long distance = duration * 0.034 / 2;
  return distance;
}

bool waitForConfirmation() {
  digitalWrite(ledPin, HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Take your meds");

  unsigned long startTime = millis();
  bool confirmed = false;

  while (millis() - startTime < 25000) {
    unsigned long elapsed = millis() - startTime;

    if (elapsed < 5000) {
      tone(buzzerPin, 2000);
      delay(300);
      noTone(buzzerPin);
      delay(200);
    } else if (elapsed < 20000) {
      tone(buzzerPin, 1000);
      delay(100);
      noTone(buzzerPin);
      delay(600);
    } else {
      delay(500);
    }

    long distance = readDistanceCM();
    bool buttonPressed = digitalRead(buttonPin) == LOW;

    if ((distance > 0 && distance < 10) || buttonPressed) {
      confirmed = true;
      break;
    }
  }

  if (!confirmed) {
    tone(buzzerPin, 2000);
    delay(500);
    noTone(buzzerPin);
  }

  digitalWrite(ledPin, LOW);
  lcd.clear();

  if (confirmed) {
    lcd.setCursor(0, 0);
    lcd.print("Meds taken!     ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    delay(2000);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Missed Dose :(  ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    delay(2000);
  }

  return confirmed;
}

void dispensePill() {
  // Rotate compartment (MG996R)
  compartmentServo.write(90);
  delay(1000);
  compartmentServo.write(0);

  // Open gate to drop pill (SG90 Servo #1)
  gateServo.write(180);
  delay(500);
  gateServo.write(0);

  bool taken = waitForConfirmation();

  if (!taken) {
    missedServo.write(180);
    delay(500);
    missedServo.write(0);
  }
}

void displayDateTime() {
  bool h12, pm;
  int second = myRTC.getSecond();
  int minute = myRTC.getMinute();
  int hour = myRTC.getHour(h12, pm);
  int day = myRTC.getDate();
  bool century;
  int month = myRTC.getMonth(century);
  int year = myRTC.getYear();

  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("                ");

  char buffer[17];
  sprintf(buffer, "%02d:%02d:%02d", hour, minute, second);
  lcd.setCursor(0, 0);
  lcd.print(buffer);

  sprintf(buffer, "%02d/%02d/20%02d", day, month, year);
  lcd.setCursor(0, 1);
  lcd.print(buffer);
}

void loop() {
  int currentSecond = myRTC.getSecond();

  if (currentSecond == targetSecond1 && !hasDispensed1) {
    hasDispensed1 = true;
    dispensePill();
  }
  if (currentSecond == targetSecond2 && !hasDispensed2) {
    hasDispensed2 = true;
    dispensePill();
  }
  if (currentSecond == targetSecond3 && !hasDispensed3) {
    hasDispensed3 = true;
    dispensePill();
  }
  if (currentSecond == targetSecond4 && !hasDispensed4) {
    hasDispensed4 = true;
    dispensePill();
  }

  if (currentSecond != targetSecond1) hasDispensed1 = false;
  if (currentSecond != targetSecond2) hasDispensed2 = false;
  if (currentSecond != targetSecond3) hasDispensed3 = false;
  if (currentSecond != targetSecond4) hasDispensed4 = false;

  displayDateTime();
  delay(500);
}
