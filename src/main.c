#define TEMPSENSOR 0 //Tempsensor mode, 0 means light 1 means temp
#define DEBUG 0 //Debug mode

#include "AVR_TTC_scheduler.h"
#include "pa_io.h"

#if TEMPSENSOR
#include "tempsensor.h" 
#else
#include "lightsensor.h" 
#endif

#include "ultrasound.h" 
#include "serial.h"
#include "util/delay.h"
#include <stdio.h>
#include "avr/interrupt.h"
#include "avr/eeprom.h"

//LED port macros
#define YELLOW_LED PB5
#define GREEN_LED PB4
#define RED_LED PB3

typedef enum{
    NONE,
    ROLLED_UP,
    ROLLED_DOWN,
    TRANSITIONING
} State; //State enum with all possible program states

const static unsigned char serial[] = {0xAC, 0x00, 0x00, 0x00}; // Last byte specifies type 0 = light, 1 = temp

static volatile float distance = 0;             //The distance in Centimeter
static volatile float maxDistance = 0;          //The maximum distance before stopping a transition
static volatile float minDistance = 0;          //The minimum distance before stopping a transition

#if TEMPSENSOR
static volatile float temperature = 0;          //The temperature in Celsius
static volatile float maxTemperature = 0;       //The maximum temperature before state change
static volatile float minTemperature = 0;       //The minimum temperature before state change
#else
static volatile float lightIntensity = 0;       //The lightintensity with a resolution of 0-1024
static volatile float maxLightIntensity = 0;    //The maximum light intensity before state change
static volatile float minLightIntensity = 0;    //The minimum light intensity before state change
#endif

static const  int triggerMaxAddress = 0;        //The trigger max eeprom address
static const  int triggerMinAddress = 4;        //The trigger min eeprom address
static const  int distanceMaxAddress = 8;       //The distance max eeprom address
static const  int distanceMinAddress = 12;      //The distance min eeprom address

State currentState = NONE;                      //The program state
char direction = 0;                             //The transition direction

//Initialize all components of the program
void initialize(){
    //Init the scheduler
    SCH_Init_T1();

    //Init the UART serial connection
    serial_init();

    //Init the ADC
    adc_init();

    //Init the ultrasound 
    setup_ultrasound();

#if DEBUG // Testing values
    //Set default values for debug purposes
    #if TEMPSENSOR
        maxTemperature = 30;
        minTemperature = 28;
    #else
        maxLightIntensity = 600;
        minLightIntensity = 400;
    #endif

    maxDistance = 30;
    minDistance = 10;
#else
    #if TEMPSENSOR
        //Read the temperature constraints from eeprom
        maxTemperature = eeprom_read_float((float*) triggerMaxAddress);
        minTemperature = eeprom_read_float((float*) triggerMinAddress);
    #else
        //Read the light intensity constraints from eeprom
        maxLightIntensity = eeprom_read_float((float*) triggerMaxAddress);
        minLightIntensity = eeprom_read_float((float*) triggerMinAddress);
    #endif

    //Read the distance constraints from eeprom
    maxDistance = eeprom_read_float((float*) distanceMaxAddress);
    minDistance = eeprom_read_float((float*) distanceMinAddress);
#endif

    //Set the Pins for the LED to output (portb)
    DDRB |= (1 << PORTB5);
    DDRB |= (1 << PORTB4);
    DDRB |= (1 << PORTB3);
}

void execute(unsigned char* buffer) 
{
    // Reads the first packet from the buffer parameter and makes an empty content buffer
    unsigned char raw_command = buffer[0];
    unsigned char content_buffer[4];

    // Gets all the individual values from the raw_command variable
    unsigned char function = (raw_command & CMD_FUNCTION_MASK);
    unsigned char value = (raw_command & CMD_VALUE_MASK);
    unsigned char id = (raw_command & CMD_ID_MASK);
    
    // Check if function is write
    if (function == CMD_WRITE) { 
        // Check if value is min
        if (value == CMD_MODE_MIN) { 
            // Check if id is distance
            if (id == CMD_ID_DISTANCE) { 
                // Get min distance value from buffer and write to EEPROM
                get_content_bytes(buffer, content_buffer);
                float val = bytes_to_float(content_buffer);
                minDistance = val;
                eeprom_update_float((float*)distanceMinAddress, val);
            }
            else if (id == CMD_ID_TRIGGER_SENSOR) { 
                // Get min trigger value from buffer and write to EEPROM
                get_content_bytes(buffer, content_buffer);
                float val = bytes_to_float(content_buffer);
                #if TEMPSENSOR
                minTemperature = val;
                #else
                minLightIntensity = val;
                #endif
                eeprom_update_float((float*)triggerMinAddress, val);
            }
            else {
                // Not a valid command, set error flags
                set_error_flag(buffer, ERR_INVALID);
            }
        }
        // Check if value is max
        else if (value == CMD_MODE_MAX) { 
            // Check if id is distance
            if (id == CMD_ID_DISTANCE) { 
                // Get max distance value from buffer and write to EEPROM
                get_content_bytes(buffer, content_buffer);
                float val = bytes_to_float(content_buffer);
                maxDistance = val;
                eeprom_update_float((float*)distanceMaxAddress, val);
            }
            else if (id == CMD_ID_TRIGGER_SENSOR) { 
                // Get max sensor value from buffer and write to EEPROM
                get_content_bytes(buffer, content_buffer);
                float val = bytes_to_float(content_buffer);
                #if TEMPSENSOR
                maxTemperature = val;
                #else
                maxLightIntensity = val;
                #endif
                eeprom_update_float((float*)triggerMaxAddress, val);
            }
            else {
                // Not a valid command, set error flags
                set_error_flag(buffer, ERR_INVALID);
            }
        }
        // Send response
        if ((buffer[0] & ERR_MASK) == ERR_VALID) {
            buffer[1] = 0xff;
            transmit_byte_stream(buffer, 2);
        }
    }
    // Check if function is read
    else if (function == CMD_READ) { 
        // Check if value is current value
        if (value == CMD_MODE_VALUE) { 
            // Check if id is status
            if (id == CMD_ID_STATUS) {
                // Get current status and write to content buffer
                float_to_bytes(currentState, content_buffer);
                set_content_bytes(content_buffer, buffer);
            }
            //Checks if id is distance
            else if (id == CMD_ID_DISTANCE) {
                // Gets the current distance and write it to content buffer
                float_to_bytes(distance, content_buffer);
                set_content_bytes(content_buffer, buffer);
            }
            // Check if id is sensor
            else if (id == CMD_ID_TRIGGER_SENSOR) {
                // Get current sensor value and write it to content buffer
                #if TEMPSENSOR
                float_to_bytes(temperature, content_buffer);
                #else
                float_to_bytes(lightIntensity, content_buffer);
                #endif
                set_content_bytes(content_buffer, buffer);
            }
            // Check if id is UUID/TYPE
            else if (id == CMD_ID_UUID) {
                // Get the UUID and write it to content buffer
                set_content_bytes(serial, buffer);
            }
            else {
                // Not a valid command, set error flags
                set_error_flag(buffer, ERR_INVALID);
            }
        }
        // Check if value is min
        else if (value == CMD_MODE_MIN) { 
            // Check if id is distance
            if (id == CMD_ID_DISTANCE) { 
                // Get min distance value and write to content buffer
                float_to_bytes(minDistance, content_buffer);
                set_content_bytes(content_buffer, buffer);
            }
            // Check if id is sensor
            else if (id == CMD_ID_TRIGGER_SENSOR) { 
                // Get min sensor value and write content buffer
                #if TEMPSENSOR
                float_to_bytes(minTemperature, content_buffer);
                #else
                float_to_bytes(minLightIntensity, content_buffer);
                #endif
                set_content_bytes(content_buffer, buffer);
            }
            else {
                // Not a valid command, set error flags
                set_error_flag(buffer, ERR_INVALID);
            }
        }
        // Check if value is max
        else if (value == CMD_MODE_MAX) { 
            // Check if id is distance
            if (id == CMD_ID_DISTANCE) {
                // Get max distance and write to content buffer
                float_to_bytes(maxDistance, content_buffer);
                set_content_bytes(content_buffer, buffer);
            }
            // Check if id is sensor
            else if (id == CMD_ID_TRIGGER_SENSOR) {
                // Get max sensor value and write content buffer
                #if TEMPSENSOR
                float_to_bytes(maxTemperature, content_buffer);
                #else
                float_to_bytes(maxLightIntensity, content_buffer);
                #endif
                set_content_bytes(content_buffer, buffer);
            }
            else {
                // Not a valid command, set error flags
                set_error_flag(buffer, ERR_INVALID);
            }
        }
        // Send reply
        if((buffer[0] & ERR_MASK) == ERR_VALID) {
            transmit_byte_stream(buffer, 6);
        }
    }
    else {
        // Not a valid command, set error flags
        set_error_flag(buffer, ERR_INVALID);
    }
}

// Checks if there are bytes send from the dashboard. If so then a reply is send.
void parse_command() {
    //Protocol buffer
    unsigned char buffer[6] = {};

    //Check for recieving commands
    receive_command(buffer);
    
    //Check if the command was valid so far
    if((buffer[0] & ERR_MASK) == ERR_VALID) {
        execute(buffer);
    }
}

//Update the current state
void update_state()
{
    //Counter for the Blinking LED in the Transitioning state
    static int counter = 0;
    
    //The min, max and currecnt values of the trigger sensor
    float triggerMin = 0;
    float triggerMax = 0;
    float currentVal = 0;

//Set the appropriate sensor data depending on the type of sensor
#if TEMPSENSOR
    triggerMax = maxTemperature;
    triggerMin = minTemperature;
    currentVal = temperature;
#else
    triggerMax = maxLightIntensity;
    triggerMin = minLightIntensity;
    currentVal = lightIntensity;
#endif

    //Check if the arduino has been installed
    if(triggerMin != 0 && triggerMax != 0 && maxDistance != 0 && minDistance != 0){
        //Check the state constraint
        if(currentVal >= triggerMax && currentState != ROLLED_DOWN){
            currentState = TRANSITIONING;
            direction = 1;
        }else if(currentVal <= triggerMin && currentState != ROLLED_UP){
            currentState = TRANSITIONING;
            direction = -1;
        }
    }else { //Set the none state if variables should be initialized
        currentState = NONE;
    }

    //Update the correct state
    switch (currentState)
    {
        case ROLLED_UP: // The closed/rolledup state
            //Set the LEDs correctly, Green 0 red 1 yellow 0
            writePin(RED_LED, HIGH);
            writePin(GREEN_LED, LOW);
            writePin(YELLOW_LED, LOW);
            break;

        case ROLLED_DOWN: //The open/rolleddown state
            //Set the LEDs correctly, Green 1 red 0 yellow 0
            writePin(GREEN_LED, HIGH);
            writePin(RED_LED, LOW);
            writePin(YELLOW_LED, LOW);
            break;

        case TRANSITIONING: //The transitioning state
            //Set the LEDs correctly, Green or red 1 when moving towards that state
            writePin(RED_LED, (direction < 0) ? HIGH : LOW);
            writePin(GREEN_LED, (direction > 0) ? HIGH : LOW);
            
            //Check blink counter
            if(counter == 50){ //Check for 500ms, scheduler runs every   10ms, 10*50 is 500
                togglePin(YELLOW_LED);
                counter = 0;
            }
            //Increment the blink counter
            counter++;

            //Check the distance from the ultrasonor and change state accordingly
            if(distance > maxDistance && direction > 0){
                currentState = ROLLED_DOWN;
            }else if(distance < minDistance && direction < 0){
                currentState = ROLLED_UP;
            }
            break;

        default:
            //The None state every LED is 1
            writePin(GREEN_LED, HIGH);
            writePin(RED_LED, HIGH);
            writePin(YELLOW_LED, HIGH);
            break;
    }
}

//Update the distance with latest know sensordata
void update_distance() {
    //Get the distance from the ultrasound sensor
    distance = get_distance();
}

//Run the ultrasound sensor process
void ultrasonor_task(){
    //Trigger the ultrasonor sensor
    trigger_ultrasonor();

    //Run oneshot task to update the distance
    SCH_Add_Task(update_distance,40,0);
}

//Update and collect the trigger sensordata
void triggersensor_task() {
#if TEMPSENSOR
    //Update the temperature in celsius
    temperature = getDegreesInCelsius();
#else
    //Update the lightintensity
    lightIntensity = readLightSensor();
#endif
}

int main(){
    //Initialize the program
    initialize();
    
    //Create all the tasks
    SCH_Add_Task(parse_command, 0, 1);
    SCH_Add_Task(update_state, 0, 1);
    SCH_Add_Task(ultrasonor_task, 0, 40);
    SCH_Add_Task(triggersensor_task, 0, 10);

    //Start the scheduler (enable global interupts)
    SCH_Start();

    //Main loop
    while(1) { 
        //Task dispatching
        SCH_Dispatch_Tasks();
    }
    return 0;
}