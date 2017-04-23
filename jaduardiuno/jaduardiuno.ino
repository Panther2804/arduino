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
// brightness (between 0 and 255)
uint8_t brightness = 0;
// w: 0 normal, 1: alarm triggered, >1 ticks after alarm
long wcounter = 0;

/*
   Button states (0 or 1)
*/
OneButton ob1(BUTTON1, false);
OneButton ob2(BUTTON2, false);
OneButton ob3(BUTTON3, false);
int bs1 = 0;
int bs2 = 0;
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

  // change mode if BUTTON2 is (newly) pressed 
  if (bs2) {
    mode++;
  }
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
  }

  if (bs1 || bs2 || bs3) {
    Serial.print(hours);
    Serial.print(":");
    Serial.print(minutes);
    Serial.print(":");
    Serial.print(seconds);
    Serial.print(" ");
    Serial.print(ticks);

    Serial.print(" buttons: ");
    Serial.print(bs1);
    Serial.print(bs2);
    Serial.print(bs3);

    Serial.print(" mode: ");
    Serial.print(mode);

    Serial.print(" alarm: ");
    Serial.print(whours);
    Serial.print(":");
    Serial.println(wminutes);

    if (mode == MODE_SET_ALARM_HOURS) {
      if (bs1 == 1) {
        whours--;
        // delay(100);
      }
      if (bs3 == 1) {
        whours++;
        // delay(100);
      }
    }

    if (mode == MODE_SET_ALARM_MINUTES) {
      if (bs1 == 1) {
        wminutes++;
      }
      if (bs3 == 1) {
        wminutes--;
      }
    }

    if (bs2 == 1) {
      digitalWrite(LED_BUILTIN, HIGH);
    }
    if (bs2 == 0) {
      digitalWrite(LED_BUILTIN, LOW);
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
    hmDisplay(whours, wminutes);

    // reset buttons
    bs1 = bs2 = bs3 = 0;
  }

  // check for alarm
  if ((whours == hours) && (wminutes == minutes)) {
    wcounter = 1;
  }

  // brighten LED
  if (wcounter >= 1) {
    wcounter++;
    brightness = wcounter / 100;
    analogWrite(LED, brightness);
  }

  // turn off alarm (if 'ringing')
  if (bs1 || bs2 || bs3) {
    wcounter = 0;

    // reset buttons
    bs1 = bs2 = bs3 = 0;
  }

  // display time (if in normal mode)
  if (mode == MODE_CLOCK) {
    hmDisplay(hours, minutes);
  }
  delay(20);
}

void hmDisplay(uint8_t hours, uint8_t minutes) {
  int hm = hours * 100 + minutes;
  if (hm < 1000) {
    String s = String('0') + String(hm);
    display.print(s);
  } else {
    display.print(hm);
  }
}

void bs1Click() {
  bs1 = 1;
}

void bs2Click() {
  bs2 = 1;
}

void bs3Click() {
  bs3 = 1;
}

