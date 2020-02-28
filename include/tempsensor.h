#ifndef TEMPSENSOR_H
#define TEMPSENSOR_H

#define TEMP_SENSOR_PIN_A 0     //The sensor pin

int readTempSensor();           //Read the raw sensor data from the temperature sensor
float getDegreesInCelsius();    //Read the degrees in celsius
float getDegreesInFahrenheit(); //Read the degrees in fahrenheit

#endif