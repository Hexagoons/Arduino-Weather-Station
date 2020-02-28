#ifndef PA_IO_H
#define PA_IO_H

#include "avr/io.h"
#define HIGH 0x1                                        
#define LOW  0x0

void adc_init();                                        //Initialize the ADC
unsigned short analogRead(unsigned char channel);       //Analog read the analog pins connected to the adc
void writePin(unsigned char pin, unsigned char val);    //Write the a pin on portb
void togglePin(unsigned char pin);                      //Toggle the value of a pin on portb
int readPin(unsigned char pin);                         //Read from a pin on portb

#endif