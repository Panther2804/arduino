
#include <Wire.h>
#include <WireRtcLib.h>

WireRtcLib rtc;

// include the SevenSegmentTM1637 library
#include <SevenSegmentTM1637.h>

const byte button1 = 10;
const byte button2 = 11;
const byte button3 = 12;

const byte PIN_CLK = 4;   // define CLK pin (any digital pin)
const byte PIN_DIO = 5;   // define DIO pin (any digital pin)
SevenSegmentTM1637    display(PIN_CLK, PIN_DIO);

const int led = 9;

int number = 10;
byte lastpress = 0;
byte setmode = 1;
byte lastpress2;
short hours = 0;
short minutes = 0;
short whours = 0;
short wminutes = 0;
byte brightness = 0;
long wcounter = 0;
int hm = 0;

byte bs1 = digitalRead(button1);
byte bs2 = digitalRead(button2);
byte bs3 = digitalRead(button3);

// run setup code
void setup() {

  Wire.begin();
  rtc.begin();

  Serial.begin(9600);         // initializes the Serial connection @ 9600 baud
  display.begin();            // initializes the display
  display.setBacklight(100);  // set the brightness to 100 %
  display.print("RDY");      // display INIT on the display

  pinMode(button1, INPUT);
  pinMode(button2, INPUT);
  pinMode(button3, INPUT);
  pinMode(led, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  WireRtcLib::tm* t = rtc.getTime();
  hours = t->hour;
  minutes = t->min;
  Serial.println(hours);
  Serial.println(minutes);

  bs1 = digitalRead(button1);
  bs2 = digitalRead(button2);
  bs3 = digitalRead(button3);

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

    Serial.print(" hm: ");
    Serial.print(hm);

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

    hm = whours * 100 + wminutes;
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
    analogWrite(led, brightness);

  }

  if (bs1 || bs3) {
    wcounter = 0;
  }

  if (setmode == 1) {
    hm = hours * 100 + minutes;
    if (hm < 1000) {
      String s = String('0') + String(hm);
      display.print(s);
    } else {
      display.print(hm);
    }
  }
  delay(20);
}

