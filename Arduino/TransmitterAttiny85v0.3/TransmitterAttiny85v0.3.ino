/**
 * EasyRaceLapTimer - Copyright 2015-2016 by airbirds.de, a project of polyvision UG (haftungsbeschränkt)
 *
 * Author: Alexander B. Bierbrauer
 *
 * This file is part of EasyRaceLapTimer.
 *
 * Vresion: 0.3.1
 *
 * EasyRaceLapTimer is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * EasyRaceLapTimer is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Foobar. If not, see http://www.gnu.org/licenses/.
 **/
// use 8MHZ internal clock of the Attiny
// Timer code from http://forum.arduino.cc/index.php?topic=139729.0
#include <EEPROM.h>

#define BIT_SET(a,b) ((a) |= (1<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1<<(b)))
#define BIT_CHECK(a,b) ((a) & (1<<(b)))

// CHANGE HERE THE ID OF TRANSPONDER
// possible values are 1 to 63
#define TRANSPONDER_ID 21

// DEFINITIONS FOR TRANSPONDERS WITH PUSH BUTTON CONFIGURATION
//#define CLEAR_EEPROM
#define ENABLE_BUTTON_CONFIGURATION
#define BUTTON_PIN PB2
#define STATUS_LED_PIN PB0
#define BUTTON_INC_ID_PUSH_TIME 750
#define LED_BLINK_CONFIRM_TIME 750
#define ENABLE_PROGRAMMING_MODE_TIME 8000 // 8 seconds

// DO NOT CHANGE ANYTHING BELOW
#define NUM_BITS  9
#define ZERO 250
#define ONE  650

unsigned int buffer[NUM_BITS];
unsigned int num_one_pulses = 0;
unsigned int transponder_id = TRANSPONDER_ID;
unsigned long buttonPushTime = 0;
int buttonState = 0;
unsigned long ledBlinkStartTime = 0;

void EEPROMWriteInt(int p_address, int p_value);
unsigned int EEPROMReadInt(int p_address);

void encodeIdToBuffer() {
  num_one_pulses = 0;
  buffer[0] = ZERO;
  buffer[1] = ZERO;
  buffer[2] = get_pulse_width_for_buffer(5);
  buffer[3] = get_pulse_width_for_buffer(4);
  buffer[4] = get_pulse_width_for_buffer(3);
  buffer[5] = get_pulse_width_for_buffer(2);
  buffer[6] = get_pulse_width_for_buffer(1);
  buffer[7] = get_pulse_width_for_buffer(0);
  buffer[8] = control_bit();
}

void idSetupMode(){
    digitalWrite(STATUS_LED_PIN, HIGH); // led on while pressing button
    while (digitalRead(BUTTON_PIN) == LOW){ // wait for button to be released
     delay(1); 
    }
    digitalWrite(STATUS_LED_PIN, LOW); // turn led off, now in setup mode
    delay(1000);
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(1000);
    digitalWrite(STATUS_LED_PIN, LOW);
    int msb = 0;
    int lsb = 0;
    buttonState = HIGH;
    int state = HIGH;
    for (int i = 0 ; i <= 2 ; i++){
      while (i == 0 || i == 2){ // MSB: i=0, LSB: i=2
        digitalWrite(STATUS_LED_PIN, HIGH);
          delay(50);
         digitalWrite(STATUS_LED_PIN, LOW);
          delay(50);
        state = digitalRead(BUTTON_PIN);
        if (state == LOW) { // is button pressed?
          if (state != buttonState){ // button state changed
            buttonState = state;
            buttonPushTime = millis();
          }
        } else { //button not pressed
          if (state != buttonState){ //button was pressed
          buttonState=state;
            if (buttonPushTime + BUTTON_INC_ID_PUSH_TIME <= millis()) { //if long press move to LSB
              i++;
            } else { //if short press count for ID bit
              (i==0) ? msb+=1:lsb+=1;
            }
          }     
        } //if (state == LOW)
      } // while (i == 0){
    } //for (int i = 0 ; i <= 2 ; i++){
    
    
    transponder_id = msb*10+lsb;
    if (transponder_id > 63 ) {
      transponder_id=63;
    }
    EEPROMWriteInt(0, transponder_id);
}

unsigned int get_pulse_width_for_buffer(int bit) {
  if (BIT_CHECK(transponder_id, bit)) {
    num_one_pulses += 1;
    return ONE;
  }

  return ZERO;
}

unsigned int control_bit() {
  if (num_one_pulses % 2 >= 1) {
    return ONE;
  } else {
    return ZERO;
  }
}

void setup()
{
  PORTB = 0;
#ifdef CLEAR_EEPROM
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
#endif
  
  // put your setup code here, to run once:
  pinMode(STATUS_LED_PIN, OUTPUT);
  // initialize the pushbutton pin as an input:
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  PORTB |= (1 << BUTTON_PIN);    // enable pull-up resistor

#ifdef ENABLE_BUTTON_CONFIGURATION
if (digitalRead(BUTTON_PIN) == LOW) { // if button pressed on start up, enter ID setup mode
  idSetupMode();
} else {
  transponder_id = EEPROMReadInt(0);
  if (transponder_id == 0 || transponder_id == 0xFF) { //EEPROM was erased or new chip (default from factory eeprom "should" be 0xFF)
    transponder_id = TRANSPONDER_ID; //copy from TRANSPONDER_ID default instead of using ID 1
    EEPROMWriteInt(0, transponder_id);
  }
}

  flashTransponderId();

#endif


  pinMode(PB1, OUTPUT);
  //DDRB =  0b00000010;    // set PB1 (= OCR1A) to be an output

  setFrequency(38000); // 38 kHz

  encodeIdToBuffer();
}

// Set the frequency that we will get on pin OCR1A but don't turn it on
void setFrequency(uint16_t freq)
{
  uint32_t requiredDivisor = (F_CPU / 2) / (uint32_t)freq;

  uint16_t prescalerVal = 1;
  uint8_t prescalerBits = 1;
  while ((requiredDivisor + prescalerVal / 2) / prescalerVal > 256)
  {
    ++prescalerBits;
    prescalerVal <<= 1;
  }

  uint8_t top = ((requiredDivisor + (prescalerVal / 2)) / prescalerVal) - 1;
  TCCR1 = (1 << CTC1) | prescalerBits;
  GTCCR = 0;
  OCR1C = top;
}

// Turn the frequency on
void ir_pulse_on()
{
  TCNT1 = 0;
  TCCR1 |= (1 << COM1A0);
}

// Turn the frequency off and turn off the IR LED.
// We let the counter continue running, we just turn off the OCR1A pin.
void ir_pulse_off()
{
  TCCR1 &= ~(1 << COM1A0);
}

void loop() {
  for (int i = 0; i < 3; i++) {
    for (int b = 0; b < NUM_BITS; b++) {
      switch (b) {
        case 0:
          ir_pulse_on();
          delayMicroseconds(buffer[b]);
          break;
        case 1:
          ir_pulse_off();
          delayMicroseconds(buffer[b]);
          break;
        case 2:
          ir_pulse_on();
          delayMicroseconds(buffer[b]);
          break;
        case 3:
          ir_pulse_off();
          delayMicroseconds(buffer[b]);
          break;
        case 4:
          ir_pulse_on();
          delayMicroseconds(buffer[b]);
          break;
        case 5:
          ir_pulse_off();
          delayMicroseconds(buffer[b]);
          break;
        case 6:
          ir_pulse_on();
          delayMicroseconds(buffer[b]);
          break;
        case 7:
          ir_pulse_off();
          delayMicroseconds(buffer[b]);
          break;
        case 8:
          ir_pulse_on();
          delayMicroseconds(buffer[b]);
          break;
      }
      ir_pulse_off();
    } // going through the buffer

    delay(20 + random(0, 5));
  } // 3 times

#ifdef ENABLE_BUTTON_CONFIGURATION
if (digitalRead(BUTTON_PIN) == LOW) { 
  flashTransponderId();
}
#endif
} // end of main loop

void EEPROMWriteInt(int p_address, int p_value)
{
  byte lowByte = ((p_value >> 0) & 0xFF);
  byte highByte = ((p_value >> 8) & 0xFF);

  EEPROM.write(p_address, lowByte);
  EEPROM.write(p_address + 1, highByte);
}

unsigned int EEPROMReadInt(int p_address)
{
  byte lowByte = EEPROM.read(p_address);
  byte highByte = EEPROM.read(p_address + 1);

  return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
}

void flashTransponderId() {
  //Display Transponder ID in two sets of LED Flashes
  for (int i = 0; i < transponder_id / 10; i++) { //Display MSB
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(300);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(300);
  }
  //Display a single LED Flash to seperate the bits (helps distinguish ID 1 and ID 10)
  delay(300);
  digitalWrite(STATUS_LED_PIN, HIGH);
  delay(50);
  digitalWrite(STATUS_LED_PIN, LOW);
  delay(600);

  for (int i = 0; i < transponder_id % 10; i++) { //Display LSB
    digitalWrite(STATUS_LED_PIN, HIGH);
    delay(300);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(300);
  }
}
