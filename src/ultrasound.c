#include "ultrasound.h"
#include "pa_io.h"
#include <avr/interrupt.h>
#include "util/delay.h"

static volatile unsigned long pulse = 0;            //The pulse duration on the echo pin
static volatile int i = 0;                          //The state of the pulse on the echo pin
static volatile unsigned long timerOverflow = 0;    //The amount of overflows during the echo pulse

//Setup the ultrasonor
void setup_ultrasound()
{
    //Set pin 4 to output and set pin 2 (INT0) to input
    DDRD |=  (1 << PORTD4);
    DDRD &=  ~(1 << PORTD2);
    PORTD |= (1 << PORTD2);

    //Set the interupt mask to enable the INT0 interupt
    EIMSK |= (1 << INT0);
    EICRA |= (1 << ISC00);

    //Set the TCCR2A register for timer 2 for default behaviour
    TCCR2A = 0;
}

//Triger the ultrasonor by sending a 12 microsecond pulse to the sensor
void trigger_ultrasonor()
{
    PORTD |= (1<<PIND4);
    _delay_us(12);
    PORTD &= ~(1<<PIND4);
}

//Calculate the distance based on pulse duration
unsigned long get_distance()
{
    return pulse / 58 / 16;
}

//The interupt service routine for INT0
ISR(INT0_vect)
{
    if (i == 1) //Check if state is high
    {
        //Reset the timer and set the pulse
        TCCR2B = 0;
        pulse = TCNT2 + (timerOverflow * 256);
        TCNT2 = 0;
        TIFR2 = 1 << TOV2;
        TIMSK2 = 0;
        i = 0;
    }
    if (i == 0) //Check if state is low
    {
        //Enable the timer
        TIFR2 = 1 << TOV2;
        timerOverflow = 0;
        TIMSK2 = 1 << TOIE2;   		
        TCCR2B |= (1 << CS10);
      
        i = 1;
    }
}

//The interupt service routinge for the overflow vector of timer2
ISR(TIMER2_OVF_vect)
{
    //Increment the amount of overflows
	timerOverflow++;	
}