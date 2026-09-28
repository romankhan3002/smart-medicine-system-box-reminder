#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// =====================================================
// OBJECTS
// =====================================================

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);

Servo medicineServo1;
Servo medicineServo2;


// =====================================================
// PIN DEFINITIONS
// =====================================================

// Ultrasonic
const int TRIG_PIN = 8;
const int ECHO_PIN = 7;

// Servo 1
const int SERVO1_PIN = 9;

// Servo 2
const int SERVO2_PIN = 6;

// Buzzer
const int BUZZER_PIN = 10;

// LEDs
const int RED_LED = 12;
const int GREEN_LED = 11;

// Push button
const int BUTTON_PIN = 2;

// IR Sensors
const int IR1_PIN = 3;
const int IR2_PIN = 4;


// =====================================================
// SERVO POSITIONS
// =====================================================

// CLOSED = 0 degrees
// OPEN    = 90 degrees

const int SERVO1_CLOSED = 0;
const int SERVO1_OPEN = 90;

const int SERVO2_CLOSED = 0;
const int SERVO2_OPEN = 90;


// =====================================================
// MEDICINE TIME
// =====================================================

// Example: 14:27 = 2:27 PM

const int MEDICINE_HOUR = 14;
const int MEDICINE_MINUTE = 27;


// =====================================================
// ULTRASONIC SETTINGS
// =====================================================

const int HAND_DISTANCE = 10;


// =====================================================
// RED LED BLINK
// =====================================================

const unsigned long RED_BLINK_TIME = 500;

unsigned long lastRedBlinkTime = 0;

bool redLedState = false;


// =====================================================
// SERVO OPEN TIME
// =====================================================

// Servos stay at 90 degrees.
// This timer is used to track the first 10 seconds.

const unsigned long SERVO_OPEN_TIME = 10000;

unsigned long servoOpenStartTime = 0;


// =====================================================
// GREEN LED
// =====================================================

const unsigned long GREEN_LED_TIME = 3000;


// =====================================================
// SYSTEM STATES
// =====================================================

enum SystemState
{
  NORMAL,
  REMINDER,
  BOX_OPEN,
  MEDICINE_TAKEN
};

SystemState currentState = NORMAL;


// =====================================================
// DAILY REMINDER CONTROL
// =====================================================

int lastReminderDay = -1;
int lastReminderMonth = -1;
int lastReminderYear = -1;


// =====================================================
// IR SENSOR STATUS
// =====================================================

bool ir1Detected = false;
bool ir2Detected = false;


// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(9600);


  // ===================================================
  // ULTRASONIC
  // ===================================================

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);


  // ===================================================
  // BUZZER
  // ===================================================

  pinMode(BUZZER_PIN, OUTPUT);


  // ===================================================
  // LEDs
  // ===================================================

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);


  // ===================================================
  // PUSH BUTTON
  // ===================================================

  // INPUT_PULLUP:
  // Not pressed = HIGH
  // Pressed     = LOW

  pinMode(BUTTON_PIN, INPUT_PULLUP);


  // ===================================================
  // IR SENSORS
  // ===================================================

  pinMode(IR1_PIN, INPUT);
  pinMode(IR2_PIN, INPUT);


  // ===================================================
  // SERVOS
  // ===================================================

  medicineServo1.attach(SERVO1_PIN);
  medicineServo2.attach(SERVO2_PIN);

  // Start with box closed

  medicineServo1.write(SERVO1_CLOSED);
  medicineServo2.write(SERVO2_CLOSED);


  // ===================================================
  // LCD
  // ===================================================

  lcd.init();
  lcd.backlight();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("SMART MEDICINE");

  lcd.setCursor(0, 1);
  lcd.print("BOX STARTING...");

  delay(2000);


  // ===================================================
  // RTC
  // ===================================================

  if (!rtc.begin())
  {
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("RTC ERROR!");

    Serial.println("RTC NOT FOUND!");

    while (1);
  }


  // ===================================================
  // RTC BATTERY
  // ===================================================

  if (rtc.lostPower())
  {
    Serial.println("WARNING: RTC LOST POWER!");

    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("RTC LOST POWER");

    delay(2000);
  }


  // ===================================================
  // INITIAL OUTPUTS
  // ===================================================

  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);

  noTone(BUZZER_PIN);


  // ===================================================
  // READY SCREEN
  // ===================================================

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("SYSTEM READY");

  lcd.setCursor(0, 1);
  lcd.print("WAITING...");

  delay(2000);

  lcd.clear();


  // ===================================================
  // SERIAL
  // ===================================================

  Serial.println("==============================");
  Serial.println(" SMART MEDICINE BOX READY");
  Serial.println("==============================");
}


// =====================================================
// MAIN LOOP
// =====================================================

void loop()
{
  DateTime now = rtc.now();


  // ===================================================
  // CHECK MEDICINE TIME
  // ===================================================

  checkMedicineTime(now);


  // ===================================================
  // CURRENT STATE
  // ===================================================

  if (currentState == NORMAL)
  {
    showNormalScreen(now);
  }

  else if (currentState == REMINDER)
  {
    handleReminder();
  }

  else if (currentState == BOX_OPEN)
  {
    handleBoxOpen();
  }

  else if (currentState == MEDICINE_TAKEN)
  {
    handleMedicineTaken();
  }


  delay(50);
}


// =====================================================
// CHECK MEDICINE TIME
// =====================================================

void checkMedicineTime(DateTime now)
{
  if (now.hour() == MEDICINE_HOUR &&
      now.minute() == MEDICINE_MINUTE)
  {
    bool alreadyReminded =
      (lastReminderDay == now.day() &&
       lastReminderMonth == now.month() &&
       lastReminderYear == now.year());


    if (!alreadyReminded)
    {
      lastReminderDay = now.day();
      lastReminderMonth = now.month();
      lastReminderYear = now.year();

      startReminder();
    }
  }
}


// =====================================================
// START REMINDER
// =====================================================

void startReminder()
{
  currentState = REMINDER;


  // ===================================================
  // RED LED
  // ===================================================

  redLedState = false;

  lastRedBlinkTime = millis();

  digitalWrite(RED_LED, LOW);


  // ===================================================
  // GREEN LED OFF
  // ===================================================

  digitalWrite(GREEN_LED, LOW);


  // ===================================================
  // BUZZER ON
  // ===================================================

  tone(BUZZER_PIN, 2000);


  // ===================================================
  // RESET IR STATUS
  // ===================================================

  ir1Detected = false;
  ir2Detected = false;


  // ===================================================
  // LCD
  // ===================================================

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("MEDICINE TIME!");

  lcd.setCursor(0, 1);
  lcd.print("MOVE HAND NEAR");


  // ===================================================
  // SERIAL
  // ===================================================

  Serial.println("==============================");
  Serial.println("MEDICINE TIME!");
  Serial.println("BUZZER ON");
  Serial.println("RED LED BLINKING");
  Serial.println("WAITING FOR HAND");
  Serial.println("==============================");
}


// =====================================================
// REMINDER STATE
// =====================================================

void handleReminder()
{
  // ===================================================
  // RED LED BLINKING
  // ===================================================

  if (millis() - lastRedBlinkTime >= RED_BLINK_TIME)
  {
    lastRedBlinkTime = millis();

    redLedState = !redLedState;

    digitalWrite(RED_LED, redLedState);
  }


  // ===================================================
  // BUZZER CONTINUES
  // ===================================================

  tone(BUZZER_PIN, 2000);


  // ===================================================
  // LCD
  // ===================================================

  lcd.setCursor(0, 0);
  lcd.print("MEDICINE TIME! ");

  lcd.setCursor(0, 1);
  lcd.print("MOVE HAND NEAR ");


  // ===================================================
  // ULTRASONIC
  // ===================================================

  long distance = getDistance();


  if (distance > 0 && distance <= HAND_DISTANCE)
  {
    Serial.print("HAND DETECTED: ");
    Serial.print(distance);
    Serial.println(" cm");

    openMedicineBox();
  }
}


// =====================================================
// ULTRASONIC SENSOR
// =====================================================

long getDistance()
{
  digitalWrite(TRIG_PIN, LOW);

  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);

  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);


  long duration =
    pulseIn(ECHO_PIN, HIGH, 30000);


  if (duration == 0)
  {
    return -1;
  }


  long distance =
    duration * 0.0343 / 2;


  return distance;
}


// =====================================================
// OPEN MEDICINE BOX
// =====================================================

void openMedicineBox()
{
  Serial.println("==============================");
  Serial.println("HAND DETECTED!");
  Serial.println("OPENING BOX TO 90 DEGREES");
  Serial.println("==============================");


  // ===================================================
  // MOVE BOTH SERVOS TO 90°
  // ===================================================

  medicineServo1.write(SERVO1_OPEN);
  medicineServo2.write(SERVO2_OPEN);


  // ===================================================
  // START 10 SECOND TIMER
  // ===================================================

  servoOpenStartTime = millis();


  // ===================================================
  // BUZZER CONTINUES
  // ===================================================

  tone(BUZZER_PIN, 2000);


  // ===================================================
  // RED LED CONTINUES BLINKING
  // ===================================================

  redLedState = true;

  digitalWrite(RED_LED, HIGH);

  lastRedBlinkTime = millis();


  // ===================================================
  // LCD
  // ===================================================

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("BOX OPEN 90 DEG");

  lcd.setCursor(0, 1);
  lcd.print("TAKE MEDICINE");


  // ===================================================
  // CHANGE STATE
  // ===================================================

  currentState = BOX_OPEN;


  // ===================================================
  // SERIAL
  // ===================================================

  Serial.println("SERVO 1 = 90 DEG");
  Serial.println("SERVO 2 = 90 DEG");
  Serial.println("BUZZER ON");
  Serial.println("RED LED BLINKING");
  Serial.println("WAITING FOR BUTTON");
}


// =====================================================
// BOX OPEN STATE
// =====================================================

void handleBoxOpen()
{
  // ===================================================
  // KEEP BOTH SERVOS AT 90°
  // ===================================================

  medicineServo1.write(SERVO1_OPEN);
  medicineServo2.write(SERVO2_OPEN);


  // ===================================================
  // RED LED CONTINUES BLINKING
  // ===================================================

  if (millis() - lastRedBlinkTime >= RED_BLINK_TIME)
  {
    lastRedBlinkTime = millis();

    redLedState = !redLedState;

    digitalWrite(RED_LED, redLedState);
  }


  // ===================================================
  // BUZZER CONTINUES
  // ===================================================

  tone(BUZZER_PIN, 2000);


  // ===================================================
  // READ IR SENSORS
  // ===================================================

  int ir1 = digitalRead(IR1_PIN);
  int ir2 = digitalRead(IR2_PIN);


  // ===================================================
  // IR SENSOR 1
  // ===================================================

  if (ir1 == LOW)
  {
    if (!ir1Detected)
    {
      ir1Detected = true;

      Serial.println("IR SENSOR 1: HAND DETECTED");

      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print("HAND DETECTED");

      lcd.setCursor(0, 1);
      lcd.print("SENSOR 1");
    }
  }


  // ===================================================
  // IR SENSOR 2
  // ===================================================

  if (ir2 == LOW)
  {
    if (!ir2Detected)
    {
      ir2Detected = true;

      Serial.println("IR SENSOR 2: HAND DETECTED");

      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print("HAND DETECTED");

      lcd.setCursor(0, 1);
      lcd.print("SENSOR 2");
    }
  }


  // ===================================================
  // BOTH IR SENSORS
  // ===================================================

  if (ir1 == LOW && ir2 == LOW)
  {
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("HAND DETECTED");

    lcd.setCursor(0, 1);
    lcd.print("BOTH SENSORS");
  }


  // ===================================================
  // AFTER 10 SECONDS
  // ===================================================
  //
  // IMPORTANT:
  // Servos remain at 90 degrees.
  //
  // Buzzer continues.
  //
  // Red LED continues blinking.
  //
  // The system waits for the push button.
  // ===================================================

  unsigned long elapsedTime =
    millis() - servoOpenStartTime;


  if (elapsedTime >= SERVO_OPEN_TIME)
  {
    // Keep servos at 90 degrees

    medicineServo1.write(SERVO1_OPEN);
    medicineServo2.write(SERVO2_OPEN);


    // If no hand is detected,
    // show button instruction.

    if (ir1 != LOW && ir2 != LOW)
    {
      lcd.setCursor(0, 0);
      lcd.print("TAKE MEDICINE   ");

      lcd.setCursor(0, 1);
      lcd.print("PRESS BUTTON    ");
    }
  }


  // ===================================================
  // PUSH BUTTON
  // ===================================================

  if (digitalRead(BUTTON_PIN) == LOW)
  {
    Serial.println("==============================");
    Serial.println("BUTTON PRESSED!");
    Serial.println("MEDICINE CONFIRMED");
    Serial.println("==============================");


    // =================================================
    // DEBOUNCE
    // =================================================

    delay(50);


    if (digitalRead(BUTTON_PIN) == LOW)
    {
      // =================================================
      // BUZZER OFF
      // =================================================

      noTone(BUZZER_PIN);


      // =================================================
      // RED LED OFF
      // =================================================

      digitalWrite(RED_LED, LOW);

      redLedState = false;


      // =================================================
      // GREEN LED ON
      // =================================================

      digitalWrite(GREEN_LED, HIGH);


      // =================================================
      // CLOSE BOTH SERVOS
      // =================================================

      medicineServo1.write(SERVO1_CLOSED);
      medicineServo2.write(SERVO2_CLOSED);


      // =================================================
      // LCD
      // =================================================

      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print("MEDICINE TAKEN");

      lcd.setCursor(0, 1);
      lcd.print("DOSE CONFIRMED");


      // =================================================
      // SERIAL
      // =================================================

      Serial.println("BUZZER OFF");
      Serial.println("RED LED OFF");
      Serial.println("GREEN LED ON");
      Serial.println("SERVO 1 = 0 DEG");
      Serial.println("SERVO 2 = 0 DEG");


      // =================================================
      // CHANGE STATE
      // =================================================

      currentState = MEDICINE_TAKEN;


      // =================================================
      // WAIT FOR BUTTON RELEASE
      // =================================================

      while (digitalRead(BUTTON_PIN) == LOW)
      {
        delay(10);
      }
    }
  }
}


// =====================================================
// MEDICINE TAKEN STATE
// =====================================================

void handleMedicineTaken()
{
  // ===================================================
  // GREEN LED ON
  // ===================================================

  digitalWrite(GREEN_LED, HIGH);


  // ===================================================
  // RED LED OFF
  // ===================================================

  digitalWrite(RED_LED, LOW);


  // ===================================================
  // BUZZER OFF
  // ===================================================

  noTone(BUZZER_PIN);


  // ===================================================
  // SERVOS CLOSED
  // ===================================================

  medicineServo1.write(SERVO1_CLOSED);
  medicineServo2.write(SERVO2_CLOSED);


  // ===================================================
  // LCD
  // ===================================================

  lcd.setCursor(0, 0);
  lcd.print("MEDICINE TAKEN");

  lcd.setCursor(0, 1);
  lcd.print("DOSE CONFIRMED");


  // ===================================================
  // GREEN LED FOR 3 SECONDS
  // ===================================================

  delay(GREEN_LED_TIME);


  // ===================================================
  // GREEN LED OFF
  // ===================================================

  digitalWrite(GREEN_LED, LOW);


  // ===================================================
  // RETURN TO NORMAL
  // ===================================================

  currentState = NORMAL;

  lcd.clear();


  Serial.println("==============================");
  Serial.println("DOSE COMPLETED");
  Serial.println("SYSTEM READY");
  Serial.println("WAITING FOR NEXT DOSE");
  Serial.println("==============================");
}


// =====================================================
// NORMAL SCREEN
// =====================================================

void showNormalScreen(DateTime now)
{
  lcd.setCursor(0, 0);

  lcd.print("Time: ");


  // ===================================================
  // HOUR
  // ===================================================

  if (now.hour() < 10)
  {
    lcd.print("0");
  }

  lcd.print(now.hour());

  lcd.print(":");


  // ===================================================
  // MINUTE
  // ===================================================

  if (now.minute() < 10)
  {
    lcd.print("0");
  }

  lcd.print(now.minute());

  lcd.print(":");


  // ===================================================
  // SECOND
  // ===================================================

  if (now.second() < 10)
  {
    lcd.print("0");
  }

  lcd.print(now.second());


  // ===================================================
  // SECOND LINE
  // ===================================================

  lcd.setCursor(0, 1);

  lcd.print("System Ready   ");
}