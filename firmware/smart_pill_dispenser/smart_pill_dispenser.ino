#include <Wire.h>
#include <RTClib.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

// Smart Pill Dispenser — educational prototype firmware.
// Target: Arduino Uno-compatible board.

namespace Pins {
constexpr uint8_t SERVO = 9;
constexpr uint8_t BUZZER = 11;
constexpr uint8_t GREEN_LED = 13;
constexpr uint8_t RED_LED = 12;
constexpr uint8_t LID_SWITCH = 2;
constexpr uint8_t RESET_BUTTON = 3;
constexpr uint8_t SELECT_BUTTON = 4;
constexpr uint8_t INCREMENT_BUTTON = 5;
}  // namespace Pins

constexpr uint8_t LCD_ADDRESS = 0x27;
constexpr uint8_t LCD_COLUMNS = 16;
constexpr uint8_t LCD_ROWS = 2;
constexpr uint8_t LID_CLOSED_LEVEL = LOW;
constexpr uint8_t SERVO_LOCKED_ANGLE = 0;
constexpr uint8_t SERVO_UNLOCKED_ANGLE = 90;
constexpr uint8_t ALARM_COUNT = 3;
constexpr unsigned long BUTTON_DEBOUNCE_MS = 35;
constexpr unsigned long SETTINGS_TIMEOUT_MS = 12000;
constexpr unsigned long REMINDER_BEEP_PERIOD_MS = 800;

struct AlarmTime {
  uint8_t hour;
  uint8_t minute;
};

struct Button {
  uint8_t pin;
  bool stablePressed = false;
  bool lastReading = false;
  unsigned long changedAt = 0;

  explicit Button(uint8_t buttonPin) : pin(buttonPin) {}

  void begin() const { pinMode(pin, INPUT_PULLUP); }

  bool pressed(unsigned long nowMs) {
    const bool reading = digitalRead(pin) == LOW;
    if (reading != lastReading) {
      lastReading = reading;
      changedAt = nowMs;
    }

    if ((nowMs - changedAt) >= BUTTON_DEBOUNCE_MS && reading != stablePressed) {
      stablePressed = reading;
      return stablePressed;
    }
    return false;
  }
};

enum class Mode : uint8_t {
  READY,
  MEDICINE_DUE,
  TAMPER_ALERT,
  SETTINGS
};

RTC_DS3231 rtc;
Servo lidServo;
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

AlarmTime alarms[ALARM_COUNT] = {{8, 0}, {15, 0}, {21, 0}};
uint32_t lastTriggeredMinute[ALARM_COUNT] = {0, 0, 0};

Button resetButton{Pins::RESET_BUTTON};
Button selectButton{Pins::SELECT_BUTTON};
Button incrementButton{Pins::INCREMENT_BUTTON};

Mode mode = Mode::READY;
uint8_t settingsField = 0;  // alarm 0 hour, alarm 0 minute, ...
unsigned long lastSettingsActionMs = 0;
unsigned long lastReminderBeepMs = 0;
bool lidOpenedForDose = false;

bool lidIsClosed() {
  return digitalRead(Pins::LID_SWITCH) == LID_CLOSED_LEVEL;
}

void lockLid() {
  lidServo.write(SERVO_LOCKED_ANGLE);
}

void unlockLid() {
  lidServo.write(SERVO_UNLOCKED_ANGLE);
}

void stopOutputs() {
  noTone(Pins::BUZZER);
  digitalWrite(Pins::GREEN_LED, LOW);
  digitalWrite(Pins::RED_LED, LOW);
}

void printTwoDigits(uint8_t value) {
  if (value < 10) lcd.print('0');
  lcd.print(value);
}

void printPaddedLine(uint8_t row, const char *text) {
  lcd.setCursor(0, row);
  uint8_t written = 0;
  while (*text != '\0' && written < LCD_COLUMNS) {
    lcd.print(*text++);
    ++written;
  }
  while (written++ < LCD_COLUMNS) lcd.print(' ');
}

void beginMedicineReminder(uint8_t alarmIndex, uint32_t minuteStamp) {
  lastTriggeredMinute[alarmIndex] = minuteStamp;
  mode = Mode::MEDICINE_DUE;
  lidOpenedForDose = false;
  lastReminderBeepMs = 0;
  unlockLid();
  Serial.print(F("Reminder triggered: alarm "));
  Serial.println(alarmIndex + 1);
}

void checkScheduledAlarms(const DateTime &now) {
  const uint32_t minuteStamp = now.unixtime() / 60UL;
  for (uint8_t i = 0; i < ALARM_COUNT; ++i) {
    if (now.hour() == alarms[i].hour && now.minute() == alarms[i].minute &&
        lastTriggeredMinute[i] != minuteStamp) {
      beginMedicineReminder(i, minuteStamp);
      return;
    }
  }
}

uint8_t findNextAlarm(const DateTime &now) {
  const uint16_t currentMinute = now.hour() * 60U + now.minute();
  uint8_t bestIndex = 0;
  uint16_t bestWait = 24U * 60U;

  for (uint8_t i = 0; i < ALARM_COUNT; ++i) {
    const uint16_t alarmMinute = alarms[i].hour * 60U + alarms[i].minute;
    const uint16_t wait = (alarmMinute + 24U * 60U - currentMinute) % (24U * 60U);
    if (wait < bestWait) {
      bestWait = wait;
      bestIndex = i;
    }
  }
  return bestIndex;
}

void displayReady(const DateTime &now) {
  lcd.setCursor(0, 0);
  lcd.print(F("Time "));
  printTwoDigits(now.hour());
  lcd.print(':');
  printTwoDigits(now.minute());
  lcd.print(':');
  printTwoDigits(now.second());
  lcd.print(F("  "));

  const uint8_t next = findNextAlarm(now);
  lcd.setCursor(0, 1);
  lcd.print(F("Next A"));
  lcd.print(next + 1);
  lcd.print(' ');
  printTwoDigits(alarms[next].hour);
  lcd.print(':');
  printTwoDigits(alarms[next].minute);
  lcd.print(F("   "));
}

void displaySettings() {
  const uint8_t alarmIndex = settingsField / 2;
  const bool editingMinute = (settingsField % 2) == 1;

  lcd.setCursor(0, 0);
  lcd.print(F("Set alarm "));
  lcd.print(alarmIndex + 1);
  lcd.print(F("     "));

  lcd.setCursor(0, 1);
  lcd.print(editingMinute ? ' ' : '>');
  printTwoDigits(alarms[alarmIndex].hour);
  lcd.print(':');
  lcd.print(editingMinute ? '>' : ' ');
  printTwoDigits(alarms[alarmIndex].minute);
  lcd.print(F(" Sel=next"));
}

void handleMedicineReminder(unsigned long nowMs) {
  digitalWrite(Pins::RED_LED, LOW);
  digitalWrite(Pins::GREEN_LED, HIGH);

  if (!lidIsClosed()) {
    lidOpenedForDose = true;
    noTone(Pins::BUZZER);
    printPaddedLine(0, "DOSE ACCESSED");
    printPaddedLine(1, "Close the lid");
    return;
  }

  if (lidOpenedForDose) {
    mode = Mode::READY;
    lockLid();
    stopOutputs();
    lcd.clear();
    Serial.println(F("Reminder completed; lid relocked."));
    return;
  }

  printPaddedLine(0, "MEDICINE DUE");
  printPaddedLine(1, "Open lid / Reset");
  if (lastReminderBeepMs == 0 || nowMs - lastReminderBeepMs >= REMINDER_BEEP_PERIOD_MS) {
    tone(Pins::BUZZER, 650, 220);
    lastReminderBeepMs = nowMs;
  }
}

void handleTamperAlert() {
  digitalWrite(Pins::GREEN_LED, LOW);
  digitalWrite(Pins::RED_LED, HIGH);
  tone(Pins::BUZZER, 1200);
  printPaddedLine(0, "!!! WARNING !!!");
  printPaddedLine(1, "FORCED ENTRY");
}

void enterSettings(unsigned long nowMs) {
  mode = Mode::SETTINGS;
  settingsField = 0;
  lastSettingsActionMs = nowMs;
  stopOutputs();
  lcd.clear();
}

void handleButtons(unsigned long nowMs) {
  const bool resetPressed = resetButton.pressed(nowMs);
  const bool selectPressed = selectButton.pressed(nowMs);
  const bool incrementPressed = incrementButton.pressed(nowMs);

  if (resetPressed) {
    if (lidIsClosed()) {
      mode = Mode::READY;
      lidOpenedForDose = false;
      lockLid();
      stopOutputs();
      lcd.clear();
      Serial.println(F("System reset; lid locked."));
    }
    return;
  }

  if (mode == Mode::READY && selectPressed) {
    enterSettings(nowMs);
    return;
  }

  if (mode != Mode::SETTINGS) return;

  if (selectPressed) {
    settingsField = (settingsField + 1) % (ALARM_COUNT * 2);
    lastSettingsActionMs = nowMs;
    lcd.clear();
  }

  if (incrementPressed) {
    const uint8_t alarmIndex = settingsField / 2;
    if ((settingsField % 2) == 0) {
      alarms[alarmIndex].hour = (alarms[alarmIndex].hour + 1) % 24;
    } else {
      alarms[alarmIndex].minute = (alarms[alarmIndex].minute + 1) % 60;
    }
    lastSettingsActionMs = nowMs;
  }
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  pinMode(Pins::GREEN_LED, OUTPUT);
  pinMode(Pins::RED_LED, OUTPUT);
  pinMode(Pins::BUZZER, OUTPUT);
  pinMode(Pins::LID_SWITCH, INPUT_PULLUP);
  resetButton.begin();
  selectButton.begin();
  incrementButton.begin();

  lcd.init();
  lcd.backlight();
  printPaddedLine(0, "SMART PILL BOX");
  printPaddedLine(1, "Starting...");

  lidServo.attach(Pins::SERVO);
  lockLid();
  stopOutputs();

  if (!rtc.begin()) {
    printPaddedLine(0, "RTC ERROR");
    printPaddedLine(1, "Check wiring");
    Serial.println(F("RTC not detected."));
    while (true) delay(100);
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    Serial.println(F("RTC lost power; set to sketch compile time."));
  }

  delay(900);
  lcd.clear();
}

void loop() {
  const unsigned long nowMs = millis();
  const DateTime now = rtc.now();

  handleButtons(nowMs);

  // A closed lid and Reset are required to clear a tamper condition.
  if (mode != Mode::MEDICINE_DUE && !lidIsClosed()) {
    mode = Mode::TAMPER_ALERT;
  }

  if (mode == Mode::READY || mode == Mode::SETTINGS) {
    checkScheduledAlarms(now);
  }

  if (mode == Mode::SETTINGS && nowMs - lastSettingsActionMs >= SETTINGS_TIMEOUT_MS) {
    mode = Mode::READY;
    lcd.clear();
  }

  switch (mode) {
    case Mode::READY:
      stopOutputs();
      displayReady(now);
      break;
    case Mode::MEDICINE_DUE:
      handleMedicineReminder(nowMs);
      break;
    case Mode::TAMPER_ALERT:
      handleTamperAlert();
      break;
    case Mode::SETTINGS:
      displaySettings();
      break;
  }

  delay(5);
}
