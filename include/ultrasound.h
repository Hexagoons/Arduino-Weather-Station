#ifndef ULTRASOUND_H
#define ULTRASOUND_H

void setup_ultrasound();        //Set up the ultrasound sensor
void trigger_ultrasonor();      //Trigger the ultrasound sensor
unsigned long get_distance();   //Returns the distance base on the pulse duration

#endif