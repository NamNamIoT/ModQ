/**
 * CRC16 utility for Modbus RTU
 * Compatible with ESP32 (no avr/pgmspace.h required)
 */
#ifndef CRC16_H
#define CRC16_H

#include <stdint.h>

static inline uint16_t crc16_update(uint16_t crc, uint8_t a)
{
    crc ^= (uint16_t)a;
    for (int i = 0; i < 8; i++) {
        if (crc & 1)
            crc = (crc >> 1) ^ 0xA001;
        else
            crc >>= 1;
    }
    return crc;
}

#endif // CRC16_H
