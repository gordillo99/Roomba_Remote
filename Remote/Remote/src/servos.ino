#include "Arduino.h"

int main() {
    
  //Set PB7 (pin 13) to output
  //(If we wanted to write to the pin use 'PORTB |= (1<<PB7)' to turn it on, and 'PORTB &= ~(1<<PB7)' to turn it off.
  // For the purposes of this example the pin will be controlled directly by timer 1.
  DDRB = (1<<PB7);
  
  //Clear timer config.
  TCCR3A = 0;
  TCCR3B = 0;  
  //Set to CTC (mode 4)
  TCCR3B |= (1<<WGM32);
  
  //Set prescaler to 256
  TCCR3B |= (1<<CS32);
  
  //Set TOP value (0.05 seconds)
  OCR3A = 3125;
  
  //Enable interupt A for timer 3.
  TIMSK3 |= (1<<OCIE3A);
  
  //Set timer to 0 (optional here).
  TCNT3 = 0;
  
  //=======================
  
  //PWM  
  //Clear timer config
  TCCR1A = 0;
  TCCR1B = 0;
  TIMSK1 &= ~(1<<OCIE1C);
  //Set to Fast PWM (mode 15)
  TCCR1A |= (1<<WGM10) | (1<<WGM11);
  TCCR1B |= (1<<WGM12) | (1<<WGM13);
    
  //Enable output C.
  TCCR1A |= (1<<COM1C1);
  //No prescaler
  TCCR1B |= (1<<CS12);
  
  OCR1A = 1250;  //20 ms period
  OCR1C = 70;  //Target

  //Enable interupts
  sei();

  servo_init();
  while(OCR1C < 129)
    turn_right();
  while(OCR1C > 68)
    turn_left();
  while(true);
}

static int direction_flag;

void servo_init(){
  if(OCR1C < 97)
    turn_right();
  else if(OCR1C > 97)
    turn_left();
}

void turn_right(void){
  direction_flag = 1;
}

void turn_left(void){
  direction_flag = -1;
}

ISR(TIMER3_COMPA_vect)
{
  //Slowly incrase the duty cycle of the LED.
  // Could change OCR1A to increase/decrease the frequency also.
  if(OCR1C >= 130 || OCR1C <= 65) {
    return;
  }
  OCR1C += direction_flag;
}
