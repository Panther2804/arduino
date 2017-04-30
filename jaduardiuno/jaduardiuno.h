#ifndef Jaduardiuno_h
#define Jaduardiuno_h

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

enum struct Mode;

void hmSet(uint8_t* h, uint8_t* m, Mode SET_HOURS, Mode SET_MINUTES);
void hmDisplay(uint8_t hours, uint8_t minutes, bool blink);
void ledOff();
void sleepOnAlarm();
void bs1Click();
void bs2Click();
void bs3Click();
void bs1DuringLong();
void bs2DuringLong();
void bs3DuringLong();
void alarmOff();
void colon();

void setup();
void loop();

#endif
