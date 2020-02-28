#include "pa_io.h"

//Initialize the ADC
void adc_init()
{
    ADMUX = (1<<REFS0);
    ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}

//Analog read from the ADC
unsigned short analogRead(unsigned char channel)
{
    channel &= 0b00000111;  
    ADMUX = (ADMUX & 0xF8)|channel;    
 
    ADCSRA |= (1<<ADSC);
    while(ADCSRA & (1<<ADSC));
    return (ADC);
}

// write value to pin for portb
void writePin(unsigned char pin, unsigned char val)
{
    if (val == LOW) {
        PORTB &= ~(_BV(pin));
    } else {
        PORTB |= _BV(pin);
    }
}

// toggle value to pin for portb
void togglePin(unsigned char pin)
{
    PORTB ^= (_BV(pin));
}

// read value from pin for portb
int readPin(unsigned char pin)
{
    if (PINB & _BV(pin)) {
        return HIGH;
    } else {
        return LOW;
    }
}