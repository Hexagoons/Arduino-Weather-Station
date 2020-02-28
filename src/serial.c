#include "serial.h"
#include "pa_io.h"
#include <string.h>
#include "util/delay.h"

// Initialize serial communication
void serial_init()
{
    // Initialize UART
    UBRR0H = 0;
    UBRR0L = UBBRVAL;
    UCSR0A = 0;
    UCSR0B = _BV(TXEN0) | _BV(RXEN0);
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
}

// Transmit a byte
void transmit(unsigned char data)
{
    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = data;
}

// Transmit a string
void transmit_string(unsigned char *str)
{
    for (int i = 0; i < strlen(str); i++) {
        transmit(str[i]);
    }
}

// Transmit a stream of bytes
void transmit_byte_stream(unsigned char *buffer, int size)
{
    for (int i = 0; i < size; i++) {
        transmit(buffer[i]);
    }
}

// Receive a command
void receive_command(unsigned char *buffer)
{
    // Initialize values
    int i = 0;
    int expected_cont_bytes = 0;
    unsigned char packet = 0;

    // Read the first byte from the UART
    if (UCSR0A & (1 << RXC0)) {
        // Sets the packet value to received byte
        packet = UDR0;
    }
    else {
        // Sets the error flags of the first byte
        buffer[0] |= ERR_INVALID;
        return;
    }

    // Check if the first byte is a write command
    if (packet & CMD_WRITE) {
        // Set the expected amount of content bytes to 4
        expected_cont_bytes = 4; // 32 bit value
    }

    buffer[i] = packet;
    i++;

    int time_out = 0;
    // Read until expected amount of bytes is reached or stop byte is received
    while (1) {
        if (UCSR0A & (1 << RXC0)) {
            packet = UDR0;
            if (i > expected_cont_bytes) {
                if (packet == CMD_STOP) {
                    buffer[i] = packet;
                    break;
                }
                else {
                    buffer[0] |= ERR_UNEXPECTED_BYTE_COUNT;
                    break;
                }
            }
            buffer[i] = packet;
            i++;
        }
        else {
            time_out++;
            if (time_out == 10000) {
                buffer[0] |= ERR_UNEXPECTED_BYTE_COUNT;
                break;
            }
        }
    }
}

// Copies all the values of the source buffer to byte 1 .. 4 of destination buffer
void set_content_bytes(unsigned char *src, unsigned char *dest)
{
    dest[1] = src[0];
    dest[2] = src[1];
    dest[3] = src[2];
    dest[4] = src[3];
    dest[5] = 0xff;
}

// Extract byte 1 .. 4 from the source buffer and write them all to the destination buffer
void get_content_bytes(unsigned char *src, unsigned char *dest)
{
    dest[0] = src[1];
    dest[1] = src[2];
    dest[2] = src[3];
    dest[3] = src[4];
}

// Sets the error flags of the first byte and sets the second byte to stop. 0x00 all other bytes
void set_error_flag(unsigned char *buffer, unsigned char error)
{
    buffer[0] |= error;
    buffer[1] = 0xff;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    buffer[4] = 0x00;
    buffer[5] = 0x00;
}

// Converts a byte array to an IEEE floating point value
float bytes_to_float(unsigned char *bytes)
{
    union {
        float float_value;
        unsigned char byte_values[4];
    } float_byte_u;

    float_byte_u.byte_values[0] = bytes[0];
    float_byte_u.byte_values[1] = bytes[1];
    float_byte_u.byte_values[2] = bytes[2];
    float_byte_u.byte_values[3] = bytes[3];

    return float_byte_u.float_value;
}

// Converts an IEEE floating point value to a byte array
void float_to_bytes(float value, unsigned char *buffer)
{
    union {
        float float_value;
        unsigned char byte_values[4];
    } float_byte_u;

    float_byte_u.float_value = value;

    memcpy(buffer, float_byte_u.byte_values, sizeof(float_byte_u.byte_values));
}

// Transmits a debug value over the serial connection
void debug_transmit(int value)
{
    unsigned char buffer[10];
    sprintf(buffer, "%d", value);
    transmit_string(buffer);
}