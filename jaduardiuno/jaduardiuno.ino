/**
 * Projekt: Tageslichtwecker
 * 
 * Paul Ole und Thomas Pasch
 */
#include <Wire.h>
#include <WireRtcLib.h>
#include <SevenSegmentTM1637.h>

/*
 * Push buttons
 */
const byte BUTTON1 = 10;
const byte BUTTON2 = 11;
const byte BUTTON3 = 12;

/*
 * 4 digit 7 segment display TM1637 ports
 */
const byte PIN_CLK = 4;   // define CLK pin (any digital pin)
const byte PIN_DIO = 5;   // define DIO pin (any digital pin)

/*
 * power LED port
 */
const int LED = 9;

WireRtcLib rtc;
SevenSegmentTM1637 display(PIN_CLK, PIN_DIO);

// last state of BUTTON2
byte lastpress = 0;
// mode, i.e. state machine state
byte setmode = 1;
// RTC hours of day
short hours = 0;
// RTC minutes of hour
short minutes = 0;

// wake/alarm hours of day
short whours = 0;
// wake/alarm minutes of hour
short wminutes = 0;
// brightness (between 0 and 255)
uint8_t brightness = 0;
// w: 0 normal, 1: alarm triggered, >1 ticks after alarm
long wcounter = 0;

/*
 * Button states
 */
byte bs1 = digitalRead(BUTTON1);
byte bs2 = digitalRead(BUTTON2);
byte bs3 = digitalRead(BUTTON3);

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
}

void loop() {
  WireRtcLib::tm* t = rtc.getTime();
  hours = t->hour;
  minutes = t->min;
  Serial.println(hours);
  Serial.println(minutes);

  bs1 = digitalRead(BUTTON1);
  bs2 = digitalRead(BUTTON2);
  bs3 = digitalRead(BUTTON3);

  if ((bs2 == 1) && (lastpress == 0)) {
    setmode++;
  }

  if (setmode == 4) {
    setmode = 1;
  }

  if (bs1 || (bs2 && !lastpress) || bs3) {

    Serial.print("buttonstate: ");
    Serial.print(bs2);

    Serial.print(" lastpress: ");
    Serial.print(lastpress);

    Serial.print(" setastate: ");
    Serial.print(setmode);

    Serial.print(" whours: ");
    Serial.print(hours);

    Serial.print(" wminutes: ");
    Serial.println(minutes);

    //if (setmode == 1)
    //{
    // display.print(hm);
    //}

    if (setmode == 2) {
      if (bs1 == 1) {
        whours--;
        delay(100);
      }
      if (bs3 == 1) {
        whours++;
        delay(100);
      }
    }

    if (setmode == 3) {
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

    int hm = whours * 100 + wminutes;
    if (hm < 1000) {
      String s = String('0') + String(hm);
      display.print(s);
    } else {
      display.print(hm);
    }

    lastpress = bs2;
    delay(20);
  }

  if ((whours == hours) && (wminutes == minutes)) {
    wcounter = 1;
  }

  if (wcounter >= 1) {
    wcounter++;
    brightness = wcounter / 100;
    analogWrite(LED, brightness);

  }

  if (bs1 || bs3) {
    wcounter = 0;
  }

  if (setmode == 1) {
    int hm = hours * 100 + minutes;
    if (hm < 1000) {
      String s = String('0') + String(hm);
      display.print(s);
    } else {
      display.print(hm);
    }
  }
  delay(20);
}

