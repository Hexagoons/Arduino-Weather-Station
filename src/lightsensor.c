#include "lightsensor.h"

//Read the raw sensor data of the light sensor
int readLightSensor(){
    return analogRead(LIGHT_SENSOR_PIN_A);
}