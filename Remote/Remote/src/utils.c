#include <avr/io.h>
#include "utils.h"

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

/*** Analog Pins ***/
void init_ADC()
{
    ADMUX=(1<<REFS0);                         // For Aref=AVcc;
    ADCSRA=(1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); //Rrescalar div factor =128
}

uint16_t read_ADC(uint8_t ch)
{

    // select the corresponding channel 0~7
    // ANDing with ’7′ will always keep the value
    // of ‘ch’ between 0 and 7
    ch &= 0b00000111;  // AND operation with 7
    ADMUX = (ADMUX & 0xF8)|ch; // clears the bottom 3 bits before ORing

    // start single convertion
    // write ’1′ to ADSC
    ADCSRA |= (1<<ADSC);

    // wait for conversion to complete
    // ADSC becomes ’0′ again
    // till then, run loop continuously
    while(ADCSRA & (1<<ADSC));

    return (ADC);
}


long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}