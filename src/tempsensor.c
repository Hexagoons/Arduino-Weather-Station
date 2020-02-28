#include "tempsensor.h"
#include "pa_io.h"

//Read the raw sensor data of the temperature sensor
int readTempSensor(){
    return analogRead(TEMP_SENSOR_PIN_A);
}

//Read the temperature in celcius
float getDegreesInCelsius(){
    float adcRes = readTempSensor();
    adcRes *= 5;
    adcRes /= 1024.0;
    return (adcRes - 0.5) * 100;
}

//Read the temperature in fahrenheit
float getDegreesInFahrenheit(){
    float tempInC = getDegreesInCelsius();
    return (tempInC * 9.0 / 5.0) + 32.0;
}