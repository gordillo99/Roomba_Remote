#include <avr/io.h>
#include <util/delay.h>
#include "os.h"
#include "roomba.h"
#include "utils.h"
#include "uart.h"

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
const int laser_activation_pin = 25; // the number of the laster activation pin
const int photoresitor_pin = 7;

// GLOBAL STATUS VARIABLES
BOOL firing_laser = FALSE; // global variable for current state of laser
volatile int servo_1_dir = 0; // -1 (backward), 0 (static), 1 (forward)
volatile int servo_2_dir = 0; // -1 (backward), 0 (static), 1 (forward)
volatile int roomba1_dir = 0;// -1 (backward), 0 (static), 1 (forward)
volatile int roomba2_dir = 0;// -1 (backward), 0 (static), 1 (forward)

//Servo myServo, myServo2; //The two servo motors. myServo attached to digital pin 9 and myServo2 attached to digital pin 8

void poll_incoming_commands() {
	// poll for incoming commands
	while(1){
		uart_putchar('c');
		if(uart1_rx == 26116) {
			int command = (int)uart1_getchar();
			//Serial.println(command);
			switch(command) {
				case 64: // 64 == firelaser
				case 65: // 65 == offlaser
					fire_laser(command);
					break;
			
				case 0: // servo1left = 0
					servo_1_dir = backward;
					break;
				case 1: // servo1left = 1
					servo_1_dir = forward;
					break;
				case 2: //// servo1left = 2
					servo_1_dir = stopped;
					break;

				case 16: // servo2left = 16
					servo_2_dir = backward;
					break;
				case 17: //// servo2left = 17
					servo_2_dir = forward;
					break;
				case 18: // servo2left = 18
					servo_2_dir = stopped;
					break;

				case 32: // roomba1for == 32
					roomba1_dir = right;
					break;
				case 33: // roomba1back == 33
					roomba1_dir = left;
					break;
				case 34: // roomba1stop == 34
					roomba1_dir = stopped;
					break;
				case 35: // roomba1forfast == 35
					roomba1_dir = right_fast;
					break;
				case 36: // roomba1backfast == 36
					roomba1_dir = left_fast;
					break;

				case 48: // roomba2for == 48
					roomba2_dir = backward;
					break;
				case 49: // roomba2back == 49
					roomba2_dir = forward;
					break;
				case 50: // roomba2stop == 50
					roomba2_dir = stopped;
					break;
				case 51: // roomba2forfast == 51
					roomba2_dir = forward_fast;
					break;
				case 52: // roomba2backfast == 52
					roomba2_dir = backward_fast;
					break;
				default:
					break;
			}
			uart_putchar(roomba1_dir);
			uart_putchar(roomba2_dir);
		}
		if('c' == uart1_getchar()){
			uart1_putchar('d');
		}
		Task_Next();
	}
}

void fire_laser(int command) {
	if (command == offlaser) {
			write_PORTA_LOW(laser_activation_pin); //pin MUST be on port A !!!!!!!!
		} else if(command == firelaser) {
			write_PORTA_HIGH(laser_activation_pin); //pin MUST be on port A
	}
}

void turn_servo_right(void){
	if(OCR3C >= 130)
		return;
	OCR3C += 1;
}

void turn_servo_left(void){
	if(OCR3C <= 65)
		return;
	OCR3C -= 1;
}

void move_servo()
{
	while(1){
			if(servo_1_dir == forward)
				turn_servo_right();
			else if(servo_1_dir == backward)
				turn_servo_left();
			Task_Next();
	}
}

void turn_roomba_right() {
	Roomba_Drive(100, -1);
}

void turn_roomba_left() {
	Roomba_Drive(100, 1);
}

void move_roomba_forward() {
	Roomba_Drive(-100, 32768);
}

void move_roomba_backward() {
	Roomba_Drive(100, 32768);
}

void stop_roomba() {
	Roomba_Drive(0, 0);
}

void move_roomba_forward_left() {
	Roomba_Drive(100, -100);
}

void move_roomba_forward_right() {
	Roomba_Drive(100, 100);
}

void move_roomba_backward_left() {
	Roomba_Drive(-100, 100);
}

void move_roomba_backward_right() {
	Roomba_Drive(-100, -100);
}

void turn_roomba_right_fast() {
	Roomba_Drive(200, -1);
}

void turn_roomba_left_fast() {
	Roomba_Drive(200, 1);
}

void move_roomba_forward_fast() {
	Roomba_Drive(200, 32768);
}

void move_roomba_backward_fast() {
	Roomba_Drive(-200, 32768);
}

void move_roomba_forward_left_fast() {
	Roomba_Drive(200, 100);
}

void move_roomba_forward_right_fast() {
	Roomba_Drive(200, -100);
}

void move_roomba_backward_left_fast() {
	Roomba_Drive(-200, -100);
}

void move_roomba_backward_right_fast() {
	Roomba_Drive(-200, 100);
}

void move_roomba() {
	while(1){
		if (roomba1_dir == right_fast && roomba2_dir == forward_fast || roomba1_dir == right && roomba2_dir == forward_fast || roomba1_dir == right_fast && roomba2_dir == forward) {
			move_roomba_forward_right_fast();
			} else if (roomba1_dir == left_fast && roomba2_dir == forward_fast || roomba1_dir == left && roomba2_dir == forward_fast || roomba1_dir == left_fast && roomba2_dir == forward) {
			move_roomba_forward_left_fast();
			} else if (roomba1_dir == right_fast && roomba2_dir == backward_fast || roomba1_dir == right && roomba2_dir == backward_fast || roomba1_dir == right_fast && roomba2_dir == backward) {
			move_roomba_backward_right_fast();
			} else if (roomba1_dir == left_fast && roomba2_dir == backward_fast || roomba1_dir == left && roomba2_dir == backward_fast || roomba1_dir == left_fast && roomba2_dir == backward) {
			move_roomba_backward_left_fast();
			} else if (roomba2_dir == backward_fast) {
			move_roomba_backward_fast();
			} else if (roomba2_dir == forward_fast) {
			move_roomba_forward_fast();
			} else if (roomba1_dir == right_fast) {
			turn_roomba_right_fast();
			} else if (roomba1_dir == left_fast) {
			turn_roomba_left_fast();
			} else if (roomba2_dir == backward) {
			move_roomba_backward();
			} else if (roomba2_dir == forward) {
			move_roomba_forward();
			} else if (roomba1_dir == right) {
			turn_roomba_right();
			} else if (roomba1_dir == left) {
			turn_roomba_left();
		}

		if (roomba1_dir == stopped && roomba2_dir == stopped) {
			stop_roomba();
		}
		Task_Next();
	}
}

void send_status() {
	
	int photoresistor_val;
	BOOL light_on_photoresistor;
	while(1){
		photoresistor_val = read_ADC(photoresitor_pin);
		light_on_photoresistor = photoresistor_val > 400;
		if (light_on_photoresistor) { 
			if (roomba1_dir == left_fast) uart1_putchar(80);
			else if (roomba1_dir == right_fast) uart1_putchar(81);
			else if (roomba2_dir == forward_fast) uart1_putchar(82);
			else if (roomba2_dir == backward_fast) uart1_putchar(83);
			else if (roomba1_dir == stopped && roomba2_dir == stopped) uart1_putchar(84);
			} else {
			if (roomba1_dir == left_fast) uart1_putchar(112);
			else if (roomba1_dir == right_fast) uart1_putchar(113);
			else if (roomba2_dir == forward_fast) uart1_putchar(114);
			else if (roomba2_dir == backward_fast) uart1_putchar(115);
			else if (roomba1_dir == stopped && roomba2_dir == stopped) uart1_putchar(116);
		}
		Task_Next();
	}
}

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
	OCR3C = 95;
}

void servo_init(){
	servo_timer_init();
}

void serial_init()
{
	// initialize serial communication at 19 200 bits per second:
	uart_init();
	uart1_init();
}

void pin_init()
{
	// initialize laser related pins
	mode_PORTA_OUTPUT(laser_activation_pin);
	init_ADC();
}

void a_main()
{
	Task_Create_System(servo_init, 0);
	Task_Create_System(serial_init, 0);
	Task_Create_System(Roomba_Init, 0); //Roomba is on pin 13, and uart0
	Task_Create_System(pin_init, 0);
	Task_Create_RR(move_servo, 0);
	Task_Create_RR(poll_incoming_commands, 0);
	Task_Create_RR(move_roomba, 0);
	//Task_Create_RR(send_status, 0);
}
