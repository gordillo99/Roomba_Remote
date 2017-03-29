#define TRUE 1
#define FALSE 0

/*** Joystick Directions ***/
#define NORTH 'n'
#define SOUTH 's'
#define WEST 'w'
#define EAST 'e'
#define NONE '0'

void mode_PORTA_INPUT(unsigned int pin);
void mode_PORTA_OUTPUT(unsigned int pin);
void write_PORTA_HIGH(unsigned int pin);
void write_PORTA_LOW(unsigned int pin);
int read_PORTA(unsigned int pin);

void init_ADC();
uint16_t read_ADC(uint8_t ch);

long map(long x, long in_min, long in_max, long out_min, long out_max);