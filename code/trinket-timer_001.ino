#include <TinyWireM.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

// setup 7-segment i2c backpack
Adafruit_7segment matrix = Adafruit_7segment();

// define RTC variables
int seconds; //00-59;
int minutes; //00-59;
int hours;//1-12 - 00-23;
int day;//1-7
int date;//01-31
int month;//01-12
int year;//0-99;
boolean weekday;

// define relay veriables
boolean relay_state;
const int relay_pin = 1;

// define button variables
const int buttonPin = 4;
int buttonState = 0;

// define digit math variables
int da; int db; int dc; int dd;

void setup()
{
  pinMode (relay_pin, OUTPUT);
  pinMode (buttonPin, INPUT);

  TinyWireM.begin();
  matrix.begin(0x70);
  matrix.setBrightness(4);
  // clear /EOSC bit
  // Sometimes necessary to ensure that the clock
  // keeps running on just battery power. Once set,
  // it shouldn't need to be reset but it's a good
  // idea to make sure.
  TinyWireM.beginTransmission(0x68); // address DS3231
  TinyWireM.write(0x0E); // select register
  TinyWireM.write(0b00011100); // write register bitmap, bit 7 is /EOSC
  TinyWireM.endTransmission();
}

// routine to check for timer events
// rewite -- this is ugly
void check_schedule() {
  if ( day > 0 && day < 6 ) {
    weekday = true;
  } else {
    weekday = false; }

  if ( weekday == true ) {
  
  //turn on curlers in the morning
  if (hours == 5 && minutes == 15) { relay_on(); }
  if (hours == 7 && minutes == 00) { relay_off(); }
  
  //dim the led at night
  if (hours > 20 && hours < 24) { matrix.setBrightness(0); }
  else
  if (hours > 0 && hours < 10) { matrix.setBrightness(0); }
  else 
  { matrix.setBrightness(8); }
  
  }

}

void relay_on() {
  relay_state = true;
  digitalWrite(relay_pin,HIGH);
}

void relay_off() {
  relay_state = false;
  digitalWrite(relay_pin,LOW);
}

byte decToBcd(byte val) {
  return ( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val) {
  return ( (val/16*10) + (val%16) );
}

// get time from RTC
void get_time()
{
  TinyWireM.beginTransmission(104);
  TinyWireM.write(0);//set register to 0
  TinyWireM.endTransmission();
  TinyWireM.requestFrom(104, 3);//get 3 bytes (seconds,minutes,hours);
  seconds = bcdToDec(TinyWireM.read() & 0x7f);
  minutes = bcdToDec(TinyWireM.read());
  hours = bcdToDec(TinyWireM.read() & 0x3f);
}

// get date from RTC
void get_date()
{
  TinyWireM.beginTransmission(104);
  TinyWireM.write(3);//set register to 3 (day)
  TinyWireM.endTransmission();
  TinyWireM.requestFrom(104, 4); //get 5 bytes(day,date,month,year,control);
  day   = bcdToDec(TinyWireM.read());
  date  = bcdToDec(TinyWireM.read());
  month = bcdToDec(TinyWireM.read());
  year  = bcdToDec(TinyWireM.read());
}

// display time on 7-segment display
void matrix_time() {
 if (hours < 10) {
   da = 0; db = hours;
   if (hours == 0) {
     da = 1; db = 2;
   }
 } else if (hours < 20) {
     da = 1; db = hours - 10;
   } else {
     da = 2; db = hours - 20;
   }

  if (minutes < 10) {
    dc = 0; dd = minutes;
  } else if (minutes < 20) {
    dc = 1; dd = minutes - 10;
  } else if (minutes < 30) {
    dc = 2; dd = minutes - 20;
  } else if (minutes < 40) {
    dc = 3; dd = minutes - 30;
  } else if (minutes < 50) {
    dc = 4; dd = minutes - 40;
  } else if (minutes < 60) {
     dc = 5; dd = minutes - 50;
  } else {
     dc = 0; dd = 0;
  }
 // compose and write date to display
 matrix.writeDigitNum(0,da);
 matrix.writeDigitNum(1,db);
 matrix.writeDisplay();
 matrix.drawColon(true);
 matrix.writeDigitNum(3,dc);
 matrix.writeDigitNum(4,dd,relay_state);
}

void loop()
{ 
  // write request to receive data starting at register 0
  TinyWireM.beginTransmission(0x68); // 0x68 is DS3231 device address
  TinyWireM.write((byte)0); // start at register 0
  TinyWireM.endTransmission();
  TinyWireM.requestFrom(0x68, 3); // request three bytes (seconds, minutes, hours)
 
  // check date and time
  get_date();
  get_time();
  // display date and time
  matrix_time();
  // check for scheduled events
  check_schedule();

  // check for button presses
  // this loop should take ~1000ms
  // so we aren't updating the clock too often
  for(int i = 0; i < 1000; i++) {
    buttonState = digitalRead(buttonPin);
    
    if (buttonState == HIGH) { // button pressed
      if (relay_state == false) { // if relay was off, turn it on
        relay_on();
        delay(250);
      } else { // if relay was not off, turn it off
        relay_off();
        delay(250);
      } 
    }
    delay(1);
  }
}

