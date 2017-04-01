/*
 * roomba.c
 *
 *  Created on: 4-Feb-2009
 *      Author: nrqm
 */

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "roomba.h"
#include "roomba_sci.h"

void Roomba_Init()
{
	cli();

	ROOMBA_DD_DDR |= 1<<ROOMBA_DD_PIN;
    ROOMBA_DD_PORT |= 1<<ROOMBA_DD_PIN;

	_delay_ms(2000);

	// Send DD start sequence
	ROOMBA_DD_PORT &= ~(1<<ROOMBA_DD_PIN);
	_delay_ms(300);
	ROOMBA_DD_PORT |= 1<<ROOMBA_DD_PIN;
	_delay_ms(300);
	ROOMBA_DD_PORT &= ~(1<<ROOMBA_DD_PIN);
	_delay_ms(300);
	ROOMBA_DD_PORT |= 1<<ROOMBA_DD_PIN;
	_delay_ms(300);
	ROOMBA_DD_PORT &= ~(1<<ROOMBA_DD_PIN);
	_delay_ms(300);
	ROOMBA_DD_PORT |= 1<<ROOMBA_DD_PIN;
    sei();

    // Try to start the SCI
	uart_putchar(START);
	_delay_ms(200);

    uart_putchar(SAFE);
    _delay_ms(20);
}

void Roomba_Finish() {
	uart_putchar(STOP);
}

void Roomba_UpdateSensorPacket(ROOMBA_SENSOR_GROUP group, roomba_sensor_data_t* sensor_packet)
{
	uart_putchar(SENSORS);
	uart_putchar(group);
	switch(group)
	{
	case EXTERNAL:
		// environment sensors
		while (uart_bytes_recv() != 10);
		sensor_packet->bumps_wheeldrops = uart_getchar(0);
		sensor_packet->wall = uart_getchar(1);
		sensor_packet->cliff_left = uart_getchar(2);
		sensor_packet->cliff_front_left = uart_getchar(3);
		sensor_packet->cliff_front_right = uart_getchar(4);
		sensor_packet->cliff_right = uart_getchar(5);
		sensor_packet->virtual_wall = uart_getchar(6);
		sensor_packet->motor_overcurrents = uart_getchar(7);
		sensor_packet->dirt_left = uart_getchar(8);
		sensor_packet->dirt_right = uart_getchar(9);
		break;
	case CHASSIS:
		// chassis sensors
		while (uart_bytes_received() != 6);
		sensor_packet->remote_opcode = uart_getchar(0);
		sensor_packet->buttons = uart_getchar(1);
		sensor_packet->distance.bytes.high_byte = uart_getchar(2);
		sensor_packet->distance.bytes.low_byte = uart_getchar(3);
		sensor_packet->angle.bytes.high_byte = uart_getchar(4);
		sensor_packet->angle.bytes.low_byte = uart_getchar(5);
		break;
	case INTERNAL:
		// internal sensors
		while (uart_bytes_received() != 10);
		sensor_packet->charging_state = uart_getchar(0);
		sensor_packet->voltage.bytes.high_byte = uart_getchar(1);
		sensor_packet->voltage.bytes.low_byte = uart_getchar(2);
		sensor_packet->current.bytes.high_byte = uart_getchar(3);
		sensor_packet->current.bytes.low_byte = uart_getchar(4);
		sensor_packet->temperature = uart_getchar(5);
		sensor_packet->charge.bytes.high_byte = uart_getchar(6);
		sensor_packet->charge.bytes.low_byte = uart_getchar(7);
		sensor_packet->capacity.bytes.high_byte = uart_getchar(8);
		sensor_packet->capacity.bytes.low_byte = uart_getchar(9);
		break;
	}
}

void Roomba_Drive(int16_t velocity, int16_t radius )
{
	uart_putchar(DRIVE);
	uart_putchar(HIGH_BYTE(velocity));
	uart_putchar(LOW_BYTE(velocity));
	uart_putchar(HIGH_BYTE(radius));
	uart_putchar(LOW_BYTE(radius));
}

void Roomba_Direct_Drive(int16_t right_velocity, int16_t left_velocity)
{
	uart_putchar(DIRECT_DRIVE);
	uart_putchar(HIGH_BYTE(right_velocity));
	uart_putchar(LOW_BYTE(right_velocity));
	uart_putchar(HIGH_BYTE(left_velocity));
	uart_putchar(LOW_BYTE(left_velocity));
}

void Roomba_Stop()
{
	uart_putchar(STOP);
}

void Roomba_Clean()
{
	uart_putchar(CLEAN);
}