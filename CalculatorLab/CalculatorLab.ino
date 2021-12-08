/***
 *** Jack Kieny & Matt Hotovy
 *** Dr. Christophe Bohn
 *** CSCE 231
 *** 8 December 2021
 *** CalculatorLab
***/

#include "cowpi.h"
#include "math.h"

/*** INITIALIZE ***/
const uint8_t keys[4][4] = {
  {0x1, 0x2, 0x3, 0xA},
  {0x4, 0x5, 0x6, 0xB},
  {0x7, 0x8, 0x9, 0xC},
  {0xF, 0x0, 0xE, 0xD}
};
const uint8_t seven_segments[16] = {
  0x7e, 0x30, 0x6d, 0x79, 0x33, 0x5b,
  0x5f, 0x70, 0x7f, 0x73, 0x77, 0x1f, 0x0d,
  0x3d, 0x4f, 0x47
};

uint8_t operand1[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
uint8_t operand2[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
uint8_t arithmeticOperator = 0x0;
uint8_t empty_disp[8] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}; //'xxxxxxxx'
uint8_t error_array[8] = {0x05, 0x1D, 0x05, 0x05, 0x4F, 0x0, 0x0, 0x0}; // 'rorrExxx'
volatile long now = 0;
volatile long last_button_press = 0 ;
volatile long last_keypad_press = 0;

/*** DECLARE FUNCTIONS***/
void setup_hardware();
void setup_timer();
void display_data();
void display_array();
void clear_display();
void display_error();
void setup_hardware_interrupts();
void check_buttons();
void check_keypad();
void add_value_to_array(uint8_t value, uint8_t array[]);
void clear_dispay_array(uint8_t array[]);
void negate_operand(uint8_t array[]);
void perform_operation(uint8_t op1[], uint8_t op2[], uint8_t arithOp);
void convert_res_to_array(int result, bool is_negative);


/*** SETUP ***/
void setup(){
  Serial.begin(9600);
  setup_hardware();
  setup_timer();
  setup_hardware_interrupts();
  display_data(1, 0x7E);
}

/*** FUNCTIONS ***/
void setup_hardware() {

  //Setup switches
  pinMode(A4, INPUT); //Left switch
  pinMode(A5, INPUT); //Rigth switch

  // Setup buttons
  pinMode(8, INPUT_PULLUP); //Left Button
  pinMode(9, INPUT_PULLUP); //Right Button
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

void setup_timer(){
  cli();  // Pause interrupts during setup
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  // Set compare match register, must be < 65535
  OCR1A = 15625; // 16000000 / 1024 = 15625

  // Enable CTC mode
  TCCR1B |= (1 << WGM12);

  // Set prescalar to 1024
  TCCR1B |= (1 << CS10);
  TCCR1B |= (1 << CS12);

  // Enable reset timer on interrupt
  TIMSK1 |= (1 << OCIE1A);

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

void clear_dispay_array(uint8_t array[]){
  for(int i=0; i<8; i++){
    array[i] = 0x0;
  }
  return;
}

void setup_hardware_interrupts(){
  attachInterrupt(digitalPinToInterrupt(2), check_buttons, CHANGE);  //Checks buttons
  attachInterrupt(digitalPinToInterrupt(3), check_keypad, CHANGE);  //Checks keypad
  return;
}

void check_buttons(){
  now = millis();
  if (now-last_button_press > 250){
    last_button_press = now;

    if(digitalRead(8)){ //Right button pressed
      if(arithmeticOperator==0x0){
        clear_dispay_array(operand1);
        clear_display();
        display_data(1, 0x7E);  // Display "0"
      }else{
        clear_dispay_array(operand2);
        arithmeticOperator = 0x0;
        clear_display();
        display_array(operand1);  //Display "Operand 1"
      }


    }
    if(digitalRead(9)){ //Left button pressed
      if(arithmeticOperator==0x0){
        negate_operand(operand1);
        display_array(operand1);
      }else{
        negate_operand(operand2);
        display_array(operand2);
      }
    }
  }
  return;
}

void check_keypad(){
  now = millis();
  if(now-last_keypad_press > 250){
    last_keypad_press = now;
    uint8_t key_pressed = 0xFF;

    for(int i=4; i<8; i++){

      for(int j=4; j<8; j++){ // Set all rows to 1
        digitalWrite(j, 1);
      }
      digitalWrite(i, 0); //Set current row to 0

      int column = -1;
      if(!digitalRead(A0)) {
        column = 0;
      } else if(!digitalRead(A1)) {
        column = 1;
      } else if(!digitalRead(A2)) {
        column = 2;
      } else if(!digitalRead(A4)) {
        column = 3;
      }

      if(column != -1) {
        key_pressed = keys[i-4][column];

      }
    }

    for(int j=4; j<8; j++){
      digitalWrite(j, 0);
    }

    if(key_pressed>=0 && key_pressed<=9){
      if (arithmeticOperator==0x0 && operand1[7] == 0x0){
        //add to op 1
        add_value_to_array(seven_segments[key_pressed], operand1);
        display_array(operand1);
      }
      else if(operand2[7] == 0x0){
        //add to op 2
        add_value_to_array(seven_segments[key_pressed], operand2);
      }
    }
    else if(key_pressed>=0xA && key_pressed<=0xD){  // +, -, *, /
      arithmeticOperator = key_pressed;
    }
    else if (key_pressed==0xE){ // =
        perform_operation(operand1, operand2, arithmeticOperator);
    }
  }

  return;
}

void add_value_to_array(uint8_t value, uint8_t array[]){

  for(int i=6; i>=0; i--){
    array[i+1] = array[i];
  }
  array[0] = value;
  return;
}

void convert_res_to_array(int result, bool is_negative){

  //do modulo code here

  if(is_negative){
    clear_dispay_array(operand2);
    arithmeticOperator = 0x0;
    negate_operand(operand1);
  }
  display_array()
}

void perform_operation(uint8_t op1[], uint8_t op2[], uint8_t arithOp){
  if(arithOp==0x0){
    return;
  }
  bool is_negative = false;
  int valA = 0, valB=0, result=0;
  //Convert first array to int
  for (int i=0; i<8; i++){
      if(op1[i]==0x30){valA += 1*pow(10,i);} // If 1
      else if (op1[i]==0x6D){valA += 2*pow(10,i);}  // If 2
      else if (op1[i]==0x79){valA += 3*pow(10,i);}  // If 3
      else if (op1[i]==0x33){valA += 4*pow(10,i);}  // If 4
      else if (op1[i]==0x5B){valA += 5*pow(10,i);}  // If 5
      else if (op1[i]==0x5F){valA += 6*pow(10,i);}  // If 6
      else if (op1[i]==0x70){valA += 7*pow(10,i);}  // If 7
      else if (op1[i]==0x7F){valA += 8*pow(10,i);}  // If 8
      else if (op1[i]==0x73){valA += 9*pow(10,i);}  // If 9
      else if (op1[7]==0x01){valA * -1;}  // Negate
  }
  for (int i=0; i<8; i++){
    if(op2[i]==0x30){valB += 1*pow(10,i);} // If 1
    else if (op1[i]==0x6D){valB += 2*pow(10,i);}  // If 2
    else if (op1[i]==0x79){valB += 3*pow(10,i);}  // If 3
    else if (op1[i]==0x33){valB += 4*pow(10,i);}  // If 4
    else if (op1[i]==0x5B){valB += 5*pow(10,i);}  // If 5
    else if (op1[i]==0x5F){valB += 6*pow(10,i);}  // If 6
    else if (op1[i]==0x70){valB += 7*pow(10,i);}  // If 7
    else if (op1[i]==0x7F){valB += 8*pow(10,i);}  // If 8
    else if (op1[i]==0x73){valB += 9*pow(10,i);}  // If 9
    else if (op1[7]==0x01){valB * -1;}  // Negate
  }

  if (arithOp==0xA){
    result = valA + valB;
    if(result != abs(result)){is_negative=true;}
    convert_res_to_array(result, is_negative);
  }else if(arithOp==0xB){
    result = valA - valB;
    if(result != abs(result)){is_negative=true;}
    convert_res_to_array(result, is_negative);
  }else if(arithOp==0xC){
    result = valA * valB;
    if(result != abs(result)){is_negative=true;}
    convert_res_to_array(result, is_negative);
  }else if(arithOp==0xD){
    result = valA / valB;
    if(result != abs(result)){is_negative=true;}
    convert_res_to_array(result, is_negative);
  }
  return;
}

int count = 0;

ISR(TIMER1_COMPA_vect){
  // count++;
  // Serial.print("IRS count: ");
  // Serial.print(count);
  // Serial.print("\n");

 }


/*** MAIN LOOP***/
void loop(){


}
