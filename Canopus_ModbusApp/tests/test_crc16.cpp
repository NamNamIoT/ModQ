#include "mocks/Arduino.h"
#include "util/crc16.h"
#include <cassert>
#include <iostream>

void test_crc16_basic() {
    std::cout << "Running test_crc16_basic..." << std::endl;

    // Test case 1: Standard Modbus request frame: 01 03 00 00 00 02
    // Expected CRC: 0xC40B -> Low byte = 0x0B, High byte = 0xC4
    uint8_t frame1[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x02};
    uint16_t crc1 = 0xFFFF;
    for (size_t i = 0; i < sizeof(frame1); i++) {
        crc1 = crc16_update(crc1, frame1[i]);
    }
    
    assert(lowByte(crc1) == 0xC4);
    assert(highByte(crc1) == 0x0B);

    // Test case 2: Single byte update tests
    uint16_t crc2 = 0xFFFF;
    crc2 = crc16_update(crc2, 0x02); // Just any data
    assert(crc2 != 0xFFFF); // CRC should change

    std::cout << "test_crc16_basic PASSED!" << std::endl;
}
