#include "uart.h"
#include "os.h"
#include <avr/io.h>

/**
 * UART0 Handler
 */
void uart_init(){
    uint8_t sreg = SREG;
    cli();

    uart_rx = 0;

    // Make sure I/O clock to USART1 is enabled
    PRR1 &= ~(1 << PRUSART2);

    // 51 = 19.2k, 103 = 9600
    UBRR2 = 51;

    // Clear USART Transmit complete flag, normal USART transmission speed
    UCSR2A = (1 << TXC2) | (0 << U2X2);

    // Enable receiver, transmitter, and rx complete interrupt.
    UCSR2B = (1<<TXEN2) | (1<<RXEN2);
    // 8-bit data
    UCSR2C = ((1<<UCSZ21)|(1<<UCSZ20));
    // disable 2x speed
    UCSR2A &= ~(1<<U2X2);

    SREG = sreg;
}

void uart_putchar (char c)
{
    cli();
    while ( !( UCSR2A & (1<<UDRE2)) ); // Wait for empty transmit buffer
    UDR2 = c;  // Putting data into the buffer, forces transmission
    sei();
}

char uart_getchar()
{
    while(!(UCSR2A & (1<<RXC2)));
    return UDR2;
}

void uart_putstr(char *s)
{
    while(*s) uart_putchar(*s++);
}


/**
 * UART1 Handler
 */
void uart1_init(){
    uint8_t sreg = SREG;
    cli();

    uart1_rx = 0;

    // Make sure I/O clock to USART1 is enabled
    PRR1 &= ~(1 << PRUSART1);

    // Set baud rate to 19.2k at fOSC = 16 MHz
    UBRR1 = 103;

    // Clear USART Transmit complete flag, normal USART transmission speed
    UCSR1A = (1 << TXC1) | (0 << U2X1);

    // Enable receiver, transmitter, and rx complete interrupt.
    UCSR1B = (1<<TXEN1) | (1<<RXEN1) /*| (1<<RXCIE1)*/;
    // 8-bit data
    UCSR1C = ((1<<UCSZ11)|(1<<UCSZ10));
    // disable 2x speed
    UCSR1A &= ~(1<<U2X1);

    SREG = sreg;
}

void uart1_putchar (char c)
{
    cli();
    while ( !( UCSR1A & (1<<UDRE1)) ); // Wait for empty transmit buffer
    UDR1 = c;  // Putting data into the buffer, forces transmission
    sei();
}

char uart1_getchar()
{
    while(!(UCSR1A & (1<<RXC1)));
    return UDR1;
}

void uart1_putstr(char *s)
{
	while(*s) uart1_putchar(*s++);
}
