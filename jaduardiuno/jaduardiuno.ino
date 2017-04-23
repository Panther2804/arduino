/**
   Projekt: Tageslichtwecker

   Paul Ole und Thomas Pasch
*/
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

/*
   power LED port
*/
const uint8_t LED = 9;
// Must be between 0 and 255, however, our experiments show that there is no change above ~20.
const int MAX_LED_VALUE = 30;

WireRtcLib rtc;
SevenSegmentTM1637 display(PIN_CLK, PIN_DIO);

enum { MODE_CLOCK = 1, MODE_SET_ALARM_HOURS, MODE_SET_ALARM_MINUTES, MODE_OVERFLOW };

// mode, i.e. state machine state
byte mode = MODE_CLOCK;
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
// w: 0 normal, 1: alarm triggered, >1 ticks after alarm
unsigned long wcounter = 0;

/*
   Button states (0 or 1)
*/
OneButton ob1(BUTTON1, false);
OneButton ob2(BUTTON2, false);
OneButton ob3(BUTTON3, false);
int bs1 = 0;
int bs3 = 0;

// run setup code
void setup() {
  Wire.begin();
  rtc.begin();

  Serial.begin(9600);         // initializes the Serial connection @ 9600 baud
  display.begin();            // initializes the display
  display.setBacklight(100);  // set the brightness to 100 %
  display.print("RDY");      // display INIT on the display

  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  pinMode(BUTTON3, INPUT);
  pinMode(LED, OUTPUT);

  // read RTC alarm (this is in eeprom hence persistent)
  WireRtcLib::tm* t = rtc.getAlarm();
  whours = t->hour;
  wminutes = t->min;

  ob1.attachClick(bs1Click);
  ob2.attachClick(bs2Click);
  ob3.attachClick(bs3Click);

  ob1.attachDuringLongPress(bs1DuringLong);
  ob3.attachDuringLongPress(bs3DuringLong);
}

void loop() {
  // read RTC
  WireRtcLib::tm* t = rtc.getTime();
  hours = t->hour;
  minutes = t->min;
  seconds = t->sec;
  ticks = millis();

  // tick buttons
  ob1.tick();
  ob2.tick();
  ob3.tick();

  if (mode >= MODE_OVERFLOW) {
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

    mode = MODE_CLOCK;
    Serial.println("mode overflow, back to 1");
  }

  /*
  if (bs2) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
   */

  if (mode > MODE_CLOCK && mode < MODE_OVERFLOW) {
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
      Serial.print(mode);

      Serial.print(" alarm: ");
      Serial.print(whours);
      Serial.print(":");
      Serial.println(wminutes);

      if (mode == MODE_SET_ALARM_HOURS) {
        if (bs1) {
          whours += bs1;
          // delay(100);
        }
        if (bs3) {
          whours -= bs3;
          // delay(100);
        }
      }

      if (mode == MODE_SET_ALARM_MINUTES) {
        if (bs1) {
          wminutes += bs1;
        }
        if (bs3) {
          wminutes -= bs3;
        }
      }

      if (whours > 23) {
        whours = 0;
      }
      /*
      if (whours < 0) {
        whours = 23;
      }
       */
      if (wminutes > 59) {
        wminutes = 0;
      }
      /*
      if (wminutes < 0) {
        wminutes = 59;
      }
       */
    }
    hmDisplay(whours, wminutes, true);

    // reset buttons
    bs1 = bs3 = 0;
  }

  // check for alarm
  if (wcounter == 0 && whours == hours && wminutes == minutes) {
    wcounter = ticks;
  }

  // brighten LED
  if (wcounter) {
    // Add one to brightness every second
    int brightness = (ticks - wcounter) / 10000;
    if (brightness > MAX_LED_VALUE) {
      // max brightness is 255
      brightness = MAX_LED_VALUE;
    }
    Serial.print("LED power: ");
    Serial.println(brightness);
    analogWrite(LED, brightness);
  }

  // turn off alarm (if 'ringing')
  if (bs1 || bs3) {
    // end reached
    wcounter = 0;
    Serial.print("LED power: 0 (off)");
    analogWrite(LED, 0);

    // reset buttons
    bs1 = bs3 = 0;
  }

  // display time (if in normal mode)
  if (mode == MODE_CLOCK) {
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
  if (blink && (ticks/500) % 2) {
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

void bs1Click() {
  bs1 = 1;
  Serial.println("Clicked b1");
}

void bs2Click() {
  // change mode if BUTTON2 is (newly) pressed
  ++mode;
  Serial.print("Clicked b2 mode=");
  Serial.println(mode);
}

void bs3Click() {
  bs3 = 1;
  Serial.println("Clicked b3");
}

void bs1DuringLong() {
  delay(300);
  ++bs1;
}

void bs3DuringLong() {
  delay(300);
  ++bs3;
}

