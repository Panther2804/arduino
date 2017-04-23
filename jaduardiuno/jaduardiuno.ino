/**
   Projekt: Tageslichtwecker

   Paul Ole und Thomas Pasch
*/
#include <Wire.h>
#include <WireRtcLib.h>
#include <SevenSegmentTM1637.h>

/*
   Push buttons
*/
const byte BUTTON1 = 10;
const byte BUTTON2 = 11;
const byte BUTTON3 = 12;

/*
   4 digit 7 segment display TM1637 ports
*/
const byte PIN_CLK = 4;   // define CLK pin (any digital pin)
const byte PIN_DIO = 5;   // define DIO pin (any digital pin)

/*
   power LED port
*/
const int LED = 9;

WireRtcLib rtc;
SevenSegmentTM1637 display(PIN_CLK, PIN_DIO);

enum { MODE_CLOCK = 1, MODE_SET_ALARM_HOURS, MODE_SET_ALARM_MINUTES, MODE_OVERFLOW };

// last state of BUTTON2
byte lastpress = 0;
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
short whours = 0;
// wake/alarm minutes of hour
short wminutes = 0;
// brightness (between 0 and 255)
uint8_t brightness = 0;
// w: 0 normal, 1: alarm triggered, >1 ticks after alarm
long wcounter = 0;

/*
   Button states (0 or 1)
*/
byte bs1;
byte bs2;
byte bs3;

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
}

void loop() {
  // read RTC
  WireRtcLib::tm* t = rtc.getTime();
  hours = t->hour;
  minutes = t->min;
  seconds = t->sec;
  ticks = millis();

  // read buttons
  bs1 = digitalRead(BUTTON1);
  bs2 = digitalRead(BUTTON2);
  bs3 = digitalRead(BUTTON3);

  // change mode if BUTTON2 is (newly) pressed 
  if ((bs2 == 1) && (lastpress == 0)) {
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

  if (bs1 || (bs2 && !lastpress) || bs3) {
    Serial.print(hours);
    Serial.print(":");
    Serial.print(minutes);
    Serial.print(":");
    Serial.print(seconds);
    Serial.print(" ");
    Serial.print(ticks);

    Serial.print(" buttonstate: ");
    Serial.print(bs2);

    Serial.print(" lastpress: ");
    Serial.print(lastpress);

    Serial.print(" setastate: ");
    Serial.print(mode);

    Serial.print(" whours: ");
    Serial.print(hours);

    Serial.print(" wminutes: ");
    Serial.println(minutes);

    if (mode == MODE_SET_ALARM_HOURS) {
      if (bs1 == 1) {
        whours--;
        delay(100);
      }
      if (bs3 == 1) {
        whours++;
        delay(100);
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
    if (whours < 0) {
      whours = 23;
    }
    if (wminutes > 59) {
      wminutes = 0;
    }
    if (wminutes < 0) {
      wminutes = 59;
    }
    hmDisplay(whours, wminutes);

    lastpress = bs2;
    delay(20);
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

  // turn of alarm (if 'ringing')
  if (bs1 || bs3) {
    wcounter = 0;
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

