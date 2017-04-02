#include <avr/io.h>
#include <util/delay.h>
#include "os.h"
#include "roomba.h"
#include "utils.h"
#include "uart.h"

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

// constants for type of control
typedef enum roomba_states {
	MANUAL,
	AUTONOMOUS
} STATUS;

// GLOBAL STATUS VARIABLES
volatile BOOL firing_laser = FALSE; // global variable for current state of laser
volatile int servo_1_dir = 0; // -1 (backward), 0 (static), 1 (forward)
volatile int servo_2_dir = 0; // -1 (backward), 0 (static), 1 (forward)
volatile int roomba1_dir = 0;// -1 (backward), 0 (static), 1 (forward)
volatile int roomba2_dir = 0;// -1 (backward), 0 (static), 1 (forward)
volatile STATUS roomba_status = MANUAL;

volatile int bump_state = 0;
volatile int wall_state = 0;

//Servo myServo, myServo2; //The two servo motors. myServo attached to digital pin 9 and myServo2 attached to digital pin 8


void poll_incoming_commands() {
	// poll for incoming commands
	int command, previous_command;
	
	while(1){
		command = (int) uart1_getchar();
		//uart_putchar(uart1_getchar());
		
		if(command != previous_command) {
			previous_command = command;
			
			switch(command) {
			
				case 0: // servo1left = 0
					servo_1_dir = backward;
					break;
				case 1: // servo1right = 1
					servo_1_dir = forward;
					break;
				case 2: //// servo1stopped = 2
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
				
				case 64: // 64 == firelaser
				case 65: // 65 == offlaser
					fire_laser(command);
					break;
				
				default:
					break;
			}
		}
		
		Task_Next();
	}
}

void fire_laser(int command) {
	if (command == offlaser) {
		PORTA &= ~(1<<PA3); // PIN 25 
	} else if(command == firelaser) {
		PORTA |= (1<<PA3); // PIN 25 FOR LASER
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

void turn_servo_2_right(void){
	if(OCR4C >= 130)
		return;
	OCR4C += 1;
}

void turn_servo_2_left(void){
	if(OCR4C <= 65)
		return;
	OCR4C -= 1;
}

void move_servo()
{
	while(1){
		if(servo_1_dir == forward)
			turn_servo_right();
		else if(servo_1_dir == backward)
			turn_servo_left();
		else if(servo_2_dir == forward)
			turn_servo_2_left();
		else if(servo_2_dir == backward)
			turn_servo_2_right();
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
		if (wall_state) roomba_status = AUTONOMOUS;
		else roomba_status = MANUAL; 

		if (roomba_status == MANUAL) {
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
		} else { // AUTONOMOUS BEHAVIOR
			move_roomba_backward_fast();
		}
	
		Task_Next();
	}
}

void get_sensor_data() {
	for(;;) {
		Roomba_QueryList(7, 13);

		bump_state = uart_getchar();
		wall_state = uart_getchar();

		Task_Next();
	}
}

int servo_timer_init() {
	
	//Set PE5 (pin 3) to output
	//(If we wanted to write to the pin use 'PORTB |= (1<<PB7)' to turn it on, and 'PORTB &= ~(1<<PB7)' to turn it off.
	// For the purposes of this example the pin will be controlled directly by timer 1.
	DDRE = (1<<PE5);

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

int servo_timer_init2() {
	
	//Set PH5 (pin 8) to output
	//(If we wanted to write to the pin use 'PORTB |= (1<<PB7)' to turn it on, and 'PORTB &= ~(1<<PB7)' to turn it off.
	// For the purposes of this example the pin will be controlled directly by timer 1.
	DDRH = (1<<PH5);

	//PWM
	//Clear timer config
	TCCR4A = 0;
	TCCR4B = 0;
	TIMSK4 &= ~(1<<OCIE4C);
	//Set to Fast PWM (mode 15)
	TCCR4A |= (1<<WGM40) | (1<<WGM41);
	TCCR4B |= (1<<WGM42) | (1<<WGM43);
	
	//Enable output C.
	TCCR4A |= (1<<COM4C1);
	//prescaler
	TCCR4B |= (1<<CS42);
	
	OCR4A = 1250;  //20 ms period
	OCR4C = 95;
}

void servo_init(){
	servo_timer_init();
	servo_timer_init2();
	Task_Terminate();
}

void serial_init()
{
	// initialize serial communication at 19 200 bits per second:
	uart_init();
	uart1_init();
	Roomba_Init();
	Task_Terminate();
}

void pin_init()
{
	// initialize laser related pins
	mode_PORTA_OUTPUT(laser_activation_pin);
	init_ADC();
	Task_Terminate();
}

void a_main()
{
	Task_Create_System(servo_init, 0);
	Task_Create_System(serial_init, 0);
	Task_Create_System(pin_init, 0);
	Task_Create_RR(move_servo, 0);
	DDRB |= (1<<PB3);
	DDRB |= (1<<PB2);
	PORTB &= ~(1<<PB2);
	//PORTB |= (1<<PB3);
	//PORTB |= (1<<PB3);
	Task_Create_Period(poll_incoming_commands, 0, 4, 2, 2);
	Task_Create_Period(move_roomba, 0, 15, 1, 10);
	Task_Create_Period(get_sensor_data, 0, 20, 19, 4);

	Task_Terminate();
}
