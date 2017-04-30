#ifdef __IN_ECLIPSE__
//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2017-04-30 15:29:37

#include "Arduino.h"
#include "jaduardiuno.h"
#include <Wire.h>
#include <WireRtcLib.h>
#include <SevenSegmentTM1637.h>
#include <OneButton.h>
Mode& operator++(Mode& m) ;
void setup() ;
void loop() ;
void hmDisplay(uint8_t hours, uint8_t minutes, bool blink) ;
void hmSet(uint8_t* h, uint8_t* m, Mode SET_HOURS, Mode SET_MINUTES) ;
void ledOff() ;
void sleepOnAlarm() ;
void bs1Click() ;
void bs2Click() ;
void bs3Click() ;
void bs1DuringLong() ;
void bs2DuringLong() ;
void bs3DuringLong() ;
void alarmOff() ;
void colon() ;

#include "jaduardiuno.ino"


#endif
