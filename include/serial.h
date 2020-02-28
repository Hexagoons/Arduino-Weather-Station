#ifndef SERIAL_H
#define SERIAL_H

#define UBBRVAL 51 // Value for baudrate, 51 = 19200 baudrate

// Mask values
#define CMD_FUNCTION_MASK 0x80
#define CMD_VALUE_MASK 0x60
#define CMD_ID_MASK 0x18

// Command flags
#define CMD_STOP 0xFF
#define CMD_READ 0x00
#define CMD_WRITE 0x80

// Value flags
#define CMD_MODE_VALUE 0x00
#define CMD_MODE_MIN 0x20
#define CMD_MODE_MAX 0x40

// Id flags
#define CMD_ID_STATUS 0x00
#define CMD_ID_DISTANCE 0x08
#define CMD_ID_TRIGGER_SENSOR 0x10
#define CMD_ID_UUID 0x18

// Error flags
#define ERR_MASK 0x07
#define ERR_VALID 0x00
#define ERR_INVALID 0x01
#define ERR_DATA_LOSS 0x03
#define ERR_UNEXPECTED_BYTE_COUNT 0x05
#define ERR_INVALID_COMMAND 0x07

void serial_init(); // Initialize serial communication

void transmit(unsigned char data); // Transmit a byte
void transmit_string(unsigned char* str); // Transmit a string
void transmit_byte_stream(unsigned char* buffer, int size); // Transmit a byte stream

void receive_command(unsigned char* buffer); // Receive a command

void set_content_bytes(unsigned char* src, unsigned char* dest); // Write values from source buffer to byte 1 .. 4 of destination buffer
void get_content_bytes(unsigned char* src, unsigned char* dest); // Writes bytes 1..4 from source buffer to destination buffer
void set_error_flag(unsigned char* buffer, unsigned char error); // Sets the error flags of the first byte, sets the second byte to stop, and fill the rest with 0x00

float bytes_to_float(unsigned char* bytes); // Converts a byte array to an IEEE floating point value
void float_to_bytes(float value, unsigned char* buffer); // Converts an IEEE floating point value to byte array

void debug_transmit(int value); // Send a debug value via serial communication

#endif