#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------------- PIN DEFINITIONS ----------------
const int PIN_PIR = 2;
const int LED_GREEN = 3;
const int LED_RED = 4;
const int BUZZER = 10;
const int BTN_ARM = 11;
const int BTN_DISARM = 12;

// ---------------- SYSTEM STATE ----------------
bool alarmArmed = false;
bool alarmTriggered = false;
bool armingDelayActive = false;
unsigned long armingStartTime = 0;

// ---------------- DEBOUNCE CONTROL ----------------
unsigned long lastBtnArmTime = 0;
unsigned long lastBtnDisarmTime = 0;
const unsigned long debounceDelay = 200; // 200ms debounce

// ---------------- PIR STABILIZATION ----------------
unsigned long pirStabilizeStart = 0;
const unsigned long pirStabilizeDelay = 2000; // 2 seconds stabilization
bool pirStable = false;

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27,16,2);

// ---------------- BLINK/SIREN CONTROL ----------------
unsigned long lastSirenChange = 0;
int sirenStep = 0;

// ==================== FUNCTIONS ====================

// Display message on LCD
void displayLCD(const char* line1, const char* line2){
  lcd.clear();
  lcd.print(line1);
  lcd.setCursor(0,1);
  lcd.print(line2);
}

// Activate alarm system
void activateAlarm(){
  alarmArmed = true;
  alarmTriggered = false;
  sirenStep = 0;
  displayLCD("System","Armed");
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(BUZZER, LOW);
}

// Deactivate alarm system
void deactivateAlarm(){
  alarmArmed = false;
  alarmTriggered = false;
  armingDelayActive = false;
  pirStable = false;
  sirenStep = 0;
  displayLCD("System","Disarmed");
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);
  digitalWrite(BUZZER, LOW);
}

// Handle triggered alarm (siren pattern)
void handleTriggeredAlarm(){
  digitalWrite(LED_GREEN,LOW);
  unsigned long currentMillis = millis();
  if(currentMillis - lastSirenChange >= 100){ // check every 100ms
    lastSirenChange = currentMillis;
    switch(sirenStep){
      case 0:
        digitalWrite(LED_RED,HIGH);
        digitalWrite(BUZZER,HIGH);
        sirenStep++;
        break;
      case 1:
        digitalWrite(LED_RED,LOW);
        digitalWrite(BUZZER,LOW);
        sirenStep++;
        break;
      case 2:
        digitalWrite(LED_RED,HIGH);
        digitalWrite(BUZZER,HIGH);
        sirenStep++;
        break;
      case 3:
        digitalWrite(LED_RED,LOW);
        digitalWrite(BUZZER,LOW);
        sirenStep = 0; // repeat
        break;
    }
  }
}

// ==================== SETUP ====================
void setup(){
  lcd.begin();
  lcd.backlight();

  pinMode(PIN_PIR, INPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BTN_ARM, INPUT);
  pinMode(BTN_DISARM, INPUT);

  // Initial LED state
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);
  digitalWrite(BUZZER, LOW);

  displayLCD("Home Security","System Loading");
  delay(1500);
  displayLCD("System","Disarmed");

  // Start PIR stabilization timer
  pirStabilizeStart = millis();
}

// ==================== LOOP ====================
void loop(){
  unsigned long currentMillis = millis();

  // --- PIR stabilization (first 2 seconds) ---
  if(!pirStable && currentMillis - pirStabilizeStart >= 2000){
    pirStable = true;
  }

  // --- ARM BUTTON with debounce ---
  if(digitalRead(BTN_ARM) == HIGH && !alarmArmed && !armingDelayActive && currentMillis - lastBtnArmTime > debounceDelay){
    armingDelayActive = true;
    armingStartTime = currentMillis;
    displayLCD("Arming...","Wait 5 sec");
    lastBtnArmTime = currentMillis;
  }

  // --- Check if arming delay is over ---
  if(armingDelayActive && currentMillis - armingStartTime >= 5000){
    activateAlarm();
    armingDelayActive = false;
  }

  // --- DISARM BUTTON with debounce ---
  if(digitalRead(BTN_DISARM) == HIGH && currentMillis - lastBtnDisarmTime > debounceDelay){
    deactivateAlarm();
    lastBtnDisarmTime = currentMillis;
  }

  // --- CHECK PIR SENSOR ---
  if(alarmArmed && !alarmTriggered && pirStable){
    if(digitalRead(PIN_PIR) == HIGH){
      alarmTriggered = true;
      displayLCD("System","Triggered");
      lastSirenChange = currentMillis;
      sirenStep = 0;
    }
  }

  // --- HANDLE TRIGGERED ALARM ---
  if(alarmTriggered){
    handleTriggeredAlarm();
  }
}
