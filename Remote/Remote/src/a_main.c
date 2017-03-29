#include <avr/io.h>
#include <util/delay.h>
#include "os.h"
//#include "Roomba_Driver.h"

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

//Roomba r(2, 30);

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
//const int photoresitor_pin = A15;

// GLOBAL STATUS VARIABLES
BOOL firing_laser = FALSE; // global variable for current state of laser
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

static unsigned int servo_target;

int servo_timer_init() {
	
	//Set PE5 (pin 3) to output
	//(If we wanted to write to the pin use 'PORTB |= (1<<PB7)' to turn it on, and 'PORTB &= ~(1<<PB7)' to turn it off.
	// For the purposes of this example the pin will be controlled directly by timer 1.
	DDRE = (1<<PB5);
	
	//PWM
	//Clear timer config
	TCCR3A = 0;
	TCCR3B = 0;
	TIMSK3 &= ~(1<<OCIE3C);
	//Set to Fast PWM (mode 15)
	TCCR3A |= (1<<WGM30) | (1<<WGM31);
	TCCR3B |= (1<<WGM32) | (1<<WGM33);
	
	//Enable output C.
	TCCR3A |= (1<<COM3C1);
	//prescaler
	TCCR3B |= (1<<CS32);
	
	OCR3A = 1250;  //20 ms period
	
}

void servo_init(){
	servo_timer_init();
	servo_target = 95;
}

void turn_servo_right(void){
	if(servo_target >= 130)
		return;
	OCR3C += 1;
}

void turn_servo_left(void){
	if(servo_target <= 65)
		return;
	OCR3C -= 1;
}

void move_servo()
{
	//Slowly incrase the duty cycle of the LED.
	// Could change OCR1A to increase/decrease the frequency also.

	while(1){
		if(OCR3C != servo_target){
			if(OCR3C < servo_target)
				turn_servo_right();
			else if(OCR3C > servo_target)
				turn_servo_left();
		}
	}
}

/*
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
*/
void a_main()
{
	Task_Create_System(servo_init, 0);
	Task_Create_Period(move_servo, 0, 2, 1, 0);
}
