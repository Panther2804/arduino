/**
 Projekt: Tageslichtwecker

 Paul Ole und Thomas Pasch
 */
#include "jaduardiuno.h"
#include <EEPROM.h>
#include <Wire.h>
#include <WireRtcLib.h>
#include <SevenSegmentTM1637.h>
#include <OneButton.h>

/*
 Push buttons
 */
const int BUTTON1 = 10;
const int BUTTON2 = 11;
const int BUTTON3 = 12;

/*
 4 digit 7 segment display TM1637 ports
 */
const uint8_t PIN_CLK = 4;   // define CLK pin (any digital pin)
const uint8_t PIN_DIO = 5;   // define DIO pin (any digital pin)
// brightness: only 0, 10, 20, 30, 40, 50, 60, 70, 80
const uint8_t SEVEN_SEGMENT_BRIGHTNESS_DAY = 50;
const uint8_t SEVEN_SEGMENT_BRIGHTNESS_NIGHT = 10;

/*
 power LED port
 */
const uint8_t LED = 9;
// Must be between 0 and 255, however, our experiments show that there is no change above ~20.
const int MAX_LED_VALUE = 30;

const uint8_t LED_LEFT = 2;
const uint8_t LED_RIGHT = 3;
// on on LED_LEFT and LED_RIGHT is analog (0-255)
const int LED_LR_ON_VALUE = 10;

WireRtcLib rtc;
SevenSegmentTM1637 display(PIN_CLK, PIN_DIO);

enum struct Mode {
  MODE_CLOCK = 1,
  MODE_SET_ALARM_HOURS,
  MODE_SET_ALARM_MINUTES,
  MODE_SET_CLOCK_HOURS,
  MODE_SET_CLOCK_MINUTES,
  MODE_OVERFLOW
};

// pre-increment, see http://en.cppreference.com/w/cpp/language/operator_incdec
Mode& operator++(Mode& m) {
  int next = (int) m;
  m = (Mode) (next + 1);
  return m;
}

// mode, i.e. state machine state
Mode mode = Mode::MODE_CLOCK;
// RTC hours of day
uint8_t hours;
// RTC minutes of hour
uint8_t minutes;
// RTC seconds of minute
uint8_t seconds;
// RTC ticks
unsigned long ticks;

// wake/alarm hours of day
uint8_t whours = 0;
// wake/alarm minutes of hour
uint8_t wminutes = 0;
// w: 0 normal, >0: ticks for start of alarm
unsigned long wcounter = 0L;

// w: 0 MODE_CLOCK, >0 ticks when non-MODE_CLOCK (mode != MODE_CLOCK) has been entered
unsigned long nonClockMode = 0L;
const unsigned long NON_CLOCK_MODE_STAY_TIME = 60L * 1000L;

// old brightness (for better logging)
int oldBrightness = 0;

/*
 Button states (0 or 1)
 */
OneButton ob1(BUTTON1, false);
OneButton ob2(BUTTON2, false);
OneButton ob3(BUTTON3, false);
int bs1 = 0;
int bs3 = 0;

/**
 * Weather the alarm is ON or OFF
 */
bool isAlarmEnabled = true;
// EEPROM addr for storing alarm (value)
const int EEPROM_ALARM_ADR = 0;

/**
 * LED could be switched on manually!
 */
int manuallyLedValue = 0;

// run setup code
void setup() {
  Wire.begin();
  rtc.begin();

  Serial.begin(9600);         // initializes the Serial connection @ 9600 baud
  display.begin();            // initializes the display
  display.setBacklight(SEVEN_SEGMENT_BRIGHTNESS_DAY);  // set the brightness
  display.print("RDY");      // display INIT on the display

  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  pinMode(BUTTON3, INPUT);
  pinMode(LED, OUTPUT);
  pinMode(LED_LEFT, OUTPUT);
  pinMode(LED_RIGHT, OUTPUT);

  digitalWrite(LED_LEFT, LOW);
  digitalWrite(LED_RIGHT, LOW);

  colon();

  // testing only
  // analogWrite(LED_LEFT, LED_LR_ON_VALUE);
  // analogWrite(LED_RIGHT, LED_LR_ON_VALUE);
  // display.setBacklight(SEVEN_SEGMENT_BRIGHTNESS_NIGHT);

  // read RTC alarm (this is in eeprom hence persistent)
  WireRtcLib::tm* t = rtc.getAlarm();
  whours = t->hour;
  wminutes = t->min;

  // read isAlarmEnabled from EEPROM
  isAlarmEnabled = (bool) EEPROM.read(EEPROM_ALARM_ADR);
  colon();

  ob1.attachClick(bs1Click);
  ob2.attachClick(bs2Click);
  ob3.attachClick(bs3Click);

  ob1.attachDuringLongPress(bs1DuringLong);
  ob2.attachDuringLongPress(bs2DuringLong);
  ob3.attachDuringLongPress(bs3DuringLong);

  ob1.attachDoubleClick(alarmOff);
  ob2.attachDoubleClick(alarmOff);
  ob3.attachDoubleClick(alarmOff);
}

void loop() {
  bool isModeSetAlarm = mode == Mode::MODE_SET_ALARM_HOURS || mode == Mode::MODE_SET_ALARM_MINUTES;
  bool isModeSetClock = mode == Mode::MODE_SET_CLOCK_HOURS || mode == Mode::MODE_SET_CLOCK_MINUTES;

  // read RTC
  WireRtcLib::tm* t = rtc.getTime();
  if (!isModeSetClock) {
    hours = t->hour;
    minutes = t->min;
    seconds = t->sec;
  }
  ticks = millis();

  // tick buttons
  ob1.tick();
  ob2.tick();
  ob3.tick();

  // day and night brightness
  if (seconds == 0) {
    if (hours >= 9 && hours <= 21) {
      display.setBacklight(SEVEN_SEGMENT_BRIGHTNESS_DAY);
    } else {
      display.setBacklight(SEVEN_SEGMENT_BRIGHTNESS_NIGHT);
    }
  }

  if (mode >= Mode::MODE_OVERFLOW) {
    mode = Mode::MODE_CLOCK;
    Serial.println("mode overflow, back to 1");
  }
  if (nonClockMode) {
    unsigned long interval = ticks - nonClockMode;
    if (interval >= NON_CLOCK_MODE_STAY_TIME) {
      // reset to clock mode
      mode = Mode::MODE_CLOCK;
      Serial.print("Timeout: reset to MODE_CLOCK ");
      Serial.println(interval / 1000);
    }
  }

  if (manuallyLedValue) {
    if (manuallyLedValue < 0) {
      manuallyLedValue = 0;
    } else if (manuallyLedValue > MAX_LED_VALUE) {
      manuallyLedValue = MAX_LED_VALUE;
    }
    analogWrite(LED, manuallyLedValue);
    if (oldBrightness != manuallyLedValue) {
      Serial.print("manually set LED power: ");
      Serial.println(manuallyLedValue);
      oldBrightness = manuallyLedValue;
    }
  }

  /*
   if (bs2) {
   digitalWrite(LED_BUILTIN, HIGH);
   } else {
   digitalWrite(LED_BUILTIN, LOW);
   }
   */

  if (mode > Mode::MODE_CLOCK && mode < Mode::MODE_OVERFLOW) {
    if (!nonClockMode) {
      nonClockMode = ticks;
    }
    if (bs1 || bs3) {
      Serial.print(hours);
      Serial.print(":");
      Serial.print(minutes);
      Serial.print(":");
      Serial.print(seconds);
      Serial.print(" ");
      Serial.print(ticks);

      Serial.print(" buttons: ");
      Serial.print(bs1);
      // Serial.print(bs2);
      Serial.print(bs3);

      Serial.print(" mode: ");
      Serial.print((int) mode);

      Serial.print(" alarm: ");
      Serial.print(whours);
      Serial.print(":");
      Serial.println(wminutes);

      if (isModeSetAlarm) {
        hmSet(&whours, &wminutes, Mode::MODE_SET_ALARM_HOURS, Mode::MODE_SET_ALARM_MINUTES);
      } else if (isModeSetClock) {
        hmSet(&hours, &minutes, Mode::MODE_SET_CLOCK_HOURS, Mode::MODE_SET_CLOCK_MINUTES);
      } else {
        Serial.print("Unknown mode:");
        Serial.println((int) mode);
      }
    }
    if (isModeSetAlarm) {
      hmDisplay(whours, wminutes, true);
    } else if (isModeSetClock) {
      hmDisplay(hours, minutes, true);
    }

    // reset buttons
    bs1 = bs3 = 0;
  } else {
    nonClockMode = 0;
  }

  // check for alarm
  // seconds: If we don't check, 'Turn off alarm' will retrigger...
  if (isAlarmEnabled && wcounter == 0 && whours == hours && wminutes == minutes && seconds < 4) {
    wcounter = ticks;
    Serial.println("Turn on alarm");
  }

  // brighten LED
  if (wcounter) {
    // Add one to brightness every second
    int brightness = (ticks - wcounter) / 10000;
    if (brightness >= 0) {
      if (brightness > MAX_LED_VALUE / 4) {
        analogWrite(LED_LEFT, LED_LR_ON_VALUE);
      }
      if (brightness > MAX_LED_VALUE / 2) {
        // max brightness is 255
        brightness = MAX_LED_VALUE;
        analogWrite(LED_RIGHT, LED_LR_ON_VALUE);
      }
      if (oldBrightness != brightness) {
        Serial.print("LED power: ");
        Serial.println(brightness);
      }
      analogWrite(LED, brightness);
      oldBrightness = brightness;
    }

    // sleep alarm (if 'ringing')
    if (bs1 || bs3) {
      sleepOnAlarm();
      // reset buttons
      bs1 = bs3 = 0;

      Serial.println("Sleep on alarm");
    }
  }

  // display time (if in normal mode)
  if (mode == Mode::MODE_CLOCK) {
    hmDisplay(hours, minutes, false);
  }
  delay(20);
}

void hmDisplay(uint8_t hours, uint8_t minutes, bool blink) {
  /*
   Serial.print("hmDisplay: ");
   Serial.print(hours);
   Serial.print(":");
   Serial.print(minutes);
   Serial.print(" ");
   Serial.print(blink);
   Serial.print(" ");
   Serial.print(ticks);
   Serial.print(" ");
   Serial.println((ticks/500) % 2);
   */
  if (blink && (ticks / 500) % 2) {
    // empty blink cycle
    display.print("    ");
  } else {
    int hm = hours * 100 + minutes;
    if (hm < 10) {
      String s = String("000") + String(hm);
      display.print(s);
    } else if (hm < 100) {
      String s = String("00") + String(hm);
      display.print(s);
    } else if (hm < 1000) {
      String s = String('0') + String(hm);
      display.print(s);
    } else {
      display.print(hm);
    }
  }
}

void hmSet(uint8_t* h, uint8_t* m, Mode SET_HOURS, Mode SET_MINUTES) {
  if (mode == SET_HOURS) {
    if (bs1) {
      *h += bs1;
      // delay(100);
    }
    if (bs3) {
      *h -= bs3;
      // delay(100);
    }
  }

  if (mode == SET_MINUTES) {
    if (bs1) {
      *m += bs1;
    }
    if (bs3) {
      *m -= bs3;
    }
  }

  if (*h > 23) {
    *h = 0;
  }
  /*
   if (whours < 0) {
   whours = 23;
   }
   */
  if (*m > 59) {
    *m = 0;
  }
  /*
   if (wminutes < 0) {
   wminutes = 59;
   }
   */
}

void ledOff() {
  // end reached
  wcounter = 0;
  Serial.println("LED power: 0 (off)");
  analogWrite(LED, 0);
  digitalWrite(LED_LEFT, LOW);
  digitalWrite(LED_RIGHT, LOW);
  oldBrightness = 0;
  Serial.println("LED off");
}

void sleepOnAlarm() {
  ledOff();
  // power led again in 5min
  wcounter = ticks + 5L * 60L * 1000L;
  Serial.println("Alarm is sleeping for 5 min");
}

void bs1Click() {
  if (mode != Mode::MODE_CLOCK) {
    bs1 = 1;
    Serial.println("Clicked b1");
  } else {
    delay(300);
    manuallyLedValue = 1;
    Serial.print("manually LED ");
    Serial.println(manuallyLedValue);
  }
}

void bs2Click() {
  // change mode if BUTTON2 is (newly) pressed
  if (wcounter) {
    sleepOnAlarm();
  } else if (mode == Mode::MODE_SET_ALARM_MINUTES) {
    // This means that the alarm has set before
    // Write it to eeprom
    WireRtcLib::tm* t = rtc.getAlarm();
    t->hour = whours;
    t->min = wminutes;
    //
    t->mday = 0;
    t->mon = 0;
    t->sec = 0;
    rtc.setAlarm(t);
    Serial.println("Alarm saved to eeprom");

    mode = Mode::MODE_OVERFLOW;
  } else if (mode == Mode::MODE_SET_CLOCK_MINUTES) {
    // This means that the clock has set before
    // Write it to eeprom
    WireRtcLib::tm* t = rtc.getTime();
    t->hour = hours;
    t->min = minutes;
    //
    t->mday = 0;
    t->mon = 0;
    t->sec = 0;
    rtc.setTime(t);
    Serial.println("Clock saved to eeprom");

    mode = Mode::MODE_OVERFLOW;
  } else {
    ++mode;
    // mode = (Mode) (((int) mode) + 1);
  }
  Serial.print("Clicked b2 mode=");
  Serial.println((int) mode);
}

void bs3Click() {
  if (mode != Mode::MODE_CLOCK) {
    bs3 = 1;
    Serial.println("Clicked b3");
  } else {
    delay(300);
    manuallyLedValue = 1;
    Serial.print("manually LED ");
    Serial.println(manuallyLedValue);
  }
}

void bs1DuringLong() {
  if (mode != Mode::MODE_CLOCK) {
    delay(300);
    ++bs1;
  } else {
    delay(300);
    ++manuallyLedValue;
    Serial.print("manually LED ");
    Serial.println(manuallyLedValue);
  }
}

void bs2DuringLong() {
  if (wcounter) {
    // turn off alarm (if 'ringing')
    ledOff();
    Serial.println("Turn off alarm (2)");
  } else if (mode == Mode::MODE_CLOCK) {
    mode = Mode::MODE_SET_CLOCK_HOURS;
  }
}

void bs3DuringLong() {
  if (mode != Mode::MODE_CLOCK) {
    delay(300);
    ++bs3;
  } else {
    delay(300);
    --manuallyLedValue;
    Serial.print("manually LED ");
    Serial.println(manuallyLedValue);
  }
}

void alarmOff() {
  if (wcounter) {
    // turn off alarm (if 'ringing')
    ledOff();
    Serial.println("alarmOff: Turn off alarm");
  } else {
    if (mode != Mode::MODE_CLOCK) {
      // reset mode
      mode = Mode::MODE_CLOCK;
      Serial.println("alarmOff: Mode is now MODE_CLOCK");
    } else if (manuallyLedValue) {
      manuallyLedValue = 0;
      oldBrightness = 0;
      analogWrite(LED, 0);
      Serial.println("alarmOff: switched off manually LED");
    } else {
      isAlarmEnabled = !isAlarmEnabled;
      EEPROM.write(EEPROM_ALARM_ADR, (byte) isAlarmEnabled);
      colon();
      String msg = String("alarmOff: Alarm is now ");
      msg += isAlarmEnabled ? String("on") : String("off");
      Serial.println(msg);
    }
  }
}

void colon() {
  display.setColonOn(isAlarmEnabled);
}

