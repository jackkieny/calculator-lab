/***
 *** Jack Kieny & Matt Hotovy
 *** Dr. Christophe Bohn
 *** CSCE 231
 *** 8 December 2021
 *** CalculatorLab
***/

#include "cowpi.h"


/*** INITIALIZE ***/
const uint8_t keys[4][4] = {
  {0x1, 0x2, 0x3, 0xa},
  {0x4, 0x5, 0x6, 0xb},
  {0x7, 0x8, 0x9, 0xc},
  {0xe, 0x0, 0xf, 0xd}
};
const uint8_t seven_segments[16] = {
  0x7e, 0x30, 0x6d, 0x79, 0x33, 0x5b,
  0x5f, 0x70, 0x7f, 0x73, 0x77, 0x1f, 0x0d,
  0x3d, 0x4f, 0x47
};

uint8_t operand1[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
uint8_t operand2[8];
uint8_t empty_disp[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}; //'xxxxxxxx'
uint8_t error_array[8] = {0x05, 0x1D, 0x05, 0x05, 0x4F, 0x0, 0x0, 0x0}; // 'rorrExxx'


/*** DECLARE FUNCTIONS***/
void setup_hardware();
void setup_timer1();
void setup_timer2();
void display_data();
void display_array();
void clear_display();
void display_error();

/*** SETUP ***/
void setup(){
  Serial.begin(9600);
  setup_hardware();
  setup_timer1();
  setup_timer2();
}

/*** FUNCTIONS ***/
void setup_hardware() {

  //Setup switches
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);

  // Setup buttons
  pinMode(8, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  pinMode(2, INPUT);

  // Setup Keypad
  for (int pin = 4; pin < 8; pin++){
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  for (int pin = A0; pin < A4; pin++){
    pinMode(pin, INPUT_PULLUP);
  }
  pinMode(3, INPUT);

  // Setup Display
  // Copied from DisplayTest.ino
  pinMode(SS, OUTPUT);
  pinMode(MOSI, OUTPUT);
  pinMode(SCK, OUTPUT);
  clear_display();
  display_data(0xA, 8);     // intensity at 17/32
  display_data(0xB, 7);     // scan all eight digits
  display_data(0xC, 1);     // take display out of shutdown mode
  display_data(0xF, 0);     // take display out of test mode, just in case
  return;
}

void setup_timer1(){
  cli();  // Pause interrupts during setup
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  // Set compare match register to 30kHz, must be < 65535
  OCR1A = 67; // (16000000) / ((8*30000)-1) = 66.667

  // Enable CTC mode
  TCCR1A |= (1 << WGM12);

  // Set prescalar to 8
  TCCR1A |= (1 << CS11);

  // Enable reset timer on interrupt
  TIMSK1 |= (1 << OCIE1A);

  sei();  // Resume interrupts
  return;
}

void setup_timer2(){
  cli();  // Pause interrupts during setup

  TCCR2A = 0; // Set register to 0
  TCCR2B = 0; // Set register to 0
  TCNT2 = 0;  // Set intial count value to 0

  // Set compare match register to 5kHz seconds, must be < 255
  OCR2A = 100; // (16000000) / ((64 * 5000) - 1)) = 100

  // Enable CTC mode
  TCCR2A |= (1 << WGM21);

  // Set prescalar to 32
  TCCR2B |= (1 << CS21);
  TCCR2B |= (1 << CS20);

  // Enable reset time on interrupt
  TIMSK2 |= (1 << OCIE2A);

  sei();  // Resume interrupts
  return;
}

void display_data(uint8_t address, uint8_t value){
  digitalWrite(SS, LOW);
  shiftOut(MOSI, SCK, MSBFIRST, address);
  shiftOut(MOSI, SCK, MSBFIRST, value);
  digitalWrite(SS, HIGH);
  return;
}

void display_array(uint8_t array[]){
  for(int i=1; i<=8; i++){
    display_data(i, array[i-1]);
  }
  return;
}

void clear_display(){
  display_array(empty_disp);
  return;
}

void display_error(){
  display_array(error_array);
  return;
}

bool flag = false;
 // ISR(TIMER2_COMPA_vect){
 //    if(flag){
 //      display_array(error_array);
 //      flag = false;
 //    }else{
 //      clear_display();
 //      flag = true;
 //    }
 //  }
/*** MAIN LOOP***/
void loop(){
  Serial.print(digitalRead(A4));
  if (digitalRead(A5)){
    display_array(error_array);
  }else{
    clear_display();
  }
  delay(250);

}
