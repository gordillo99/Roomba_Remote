#include <avr/io.h>
#include <util/delay.h>
#include "os.h"
#include "Roomba_Driver.h"

/*
#include "./tests/TEST_too_many_tasks.h"
#include "./tests/TEST_priority.h"
#include "./tests/TEST_system_task_scheduling.h"
#include "./tests/TEST_rr_task_scheduling.h"
#include "./tests/TEST_periodic_task_scheduling.h"
#include "./tests/TEST_Task_Next.h"
#include "./tests/TEST_periodic_task_overlap.h"
#include "./tests/TEST_periodic_task_timing.h"
#include "./tests/TEST_chan_send_recieve.h"
*/

Roomba r(2, 30);

//Values for digital operations
const unsigned int LOW = 0;
const unsigned int HIGH = 1;

// CONSTANT BYTE TO NUMBER CONVENTIONS
//servo1
const unsigned int servo1left = 0;
const unsigned int servo1right = 1;
const unsigned int servo1stopped = 2;

//servo2
const unsigned int servo2left = 16;
const unsigned int servo2right = 17;
const unsigned int servo2stopped = 18;

//Roomba
const unsigned int roomba2for = 32;
const unsigned int roomba2back = 33;
const unsigned int roomba2stop = 34;
const unsigned int roomba2forfast = 35;
const unsigned int roomba2backfast = 36;

const unsigned int roomba1for = 48;
const unsigned int roomba1back = 49;
const unsigned int roomba1stop = 50;
const unsigned int roomba1forfast = 51;
const unsigned int roomba1backfast = 52;

//laser
const unsigned int firelaser = 64;
const unsigned int offlaser = 65;

//status
const unsigned int left_on = 80;
const unsigned int right_on = 81;
const unsigned int forward_on = 82;
const unsigned int backward_on = 83;
const unsigned int stopped_on = 84;

const unsigned int left_off = 112;
const unsigned int right_off = 113;
const unsigned int forward_off = 114;
const unsigned int backward_off = 115;
const unsigned int stopped_off = 116;

// GENERAL DIRECTION CONSTANTS
const int backward = -1;
const int forward = 1;
const int stopped = 0;
const int right = 2;
const int left = 3;
const int backward_fast = 4;
const int forward_fast = 5;
const int right_fast = 6;
const int left_fast = 7;

// CONSTANT PIN ASSIGNATIONS
const int laser_activation_pin = 3; // the number of the laster activation pin
const int photoresitor_pin = A15;

// GLOBAL STATUS VARIABLES
bool firing_laser = false; // global variable for current state of laser
volatile int servo_1_dir = 0; // -1 (backward), 0 (static), 1 (forward)
volatile int servo_2_dir = 0; // -1 (backward), 0 (static), 1 (forward)
volatile int roomba1_dir = 0;// -1 (backward), 0 (static), 1 (forward)
volatile int roomba2_dir = 0;// -1 (backward), 0 (static), 1 (forward)

//Servo myServo, myServo2; //The two servo motors. myServo attached to digital pin 9 and myServo2 attached to digital pin 8

/**** PORTA Digital Pins ******/
void mode_PORTA_INPUT(unsigned int pin)
{
	DDRA &= ~_BV(pin);
}

void mode_PORTA_OUTPUT(unsigned int pin)
{
	DDRA |= _BV(pin);
	write_PORTA_LOW(pin);
}

void write_PORTA_HIGH(unsigned int pin)
{
	PORTA |= _BV(pin);
}

void write_PORTA_LOW(unsigned int pin)
{
	PORTA &= ~_BV(pin);
}

int read_PORTA(unsigned int pin)
{
	int val = PINA & _BV(pin);
	return val;
}

void fire_laser(int command) {
	if (command == offlaser) {
		write_PORTA_LOW(laser_activation_pin); //pin MUST be on port A !!!!!!!!
		} else if(command == firelaser) {
		write_PORTA_HIGH(laser_activation_pin); //pin MUST be on port A
	}
}

int servo_init() {
	
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
	OCR3A = 3438;
	
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

void move_servo_left(Servo servo) {
	int pos = servo.read();

	if (pos > 5) {
		servo.write(pos - 1);
	}
}

void move_servo_right(Servo servo) {
	int pos = servo.read();

	if (pos < 170) {
		servo.write(pos + 1);
	}
}


void a_main()
{

}
