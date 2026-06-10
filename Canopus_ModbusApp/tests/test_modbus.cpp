#include "mocks/Arduino.h"
#include "CanopusModbus.h"
#include "util/crc16.h"
#include <cassert>
#include <iostream>

// Helper to append correct CRC to a vector and push it to rx_buffer
void push_mock_response(HardwareSerial& hs, const std::vector<uint8_t>& data) {
    uint16_t crc = 0xFFFF;
    for (uint8_t b : data) {
        hs.rx_buffer.push_back(b);
        crc = crc16_update(crc, b);
    }
    hs.rx_buffer.push_back(lowByte(crc));
    hs.rx_buffer.push_back(highByte(crc));
}

void test_modbus_read_holding_success() {
    std::cout << "Running test_modbus_read_holding_success..." << std::endl;
    
    Serial2.clear();
    mock_millis = 1000;

    ModbusMaster node;
    node.begin(1, Serial2);

    // Mock response for reading 2 registers: 0x01, 0x03, 0x04 (byte count), 0x12, 0x34 (Reg0), 0x56, 0x78 (Reg1)
    std::vector<uint8_t> response = {0x01, 0x03, 0x04, 0x12, 0x34, 0x56, 0x78};
    push_mock_response(Serial2, response);

    uint8_t result = node.readHoldingRegisters(0x0002, 2);

    // Verify result is success
    assert(result == ModbusMaster::ku8MBSuccess);

    // Verify TX request packet contains correct bytes:
    // Slave ID (0x01) | Func (0x03) | Addr High (0x00) | Addr Low (0x02) | Qty High (0x00) | Qty Low (0x02) | CRC Low | CRC High
    assert(Serial2.tx_buffer.size() == 8);
    assert(Serial2.tx_buffer[0] == 0x01);
    assert(Serial2.tx_buffer[1] == 0x03);
    assert(Serial2.tx_buffer[2] == 0x00);
    assert(Serial2.tx_buffer[3] == 0x02);
    assert(Serial2.tx_buffer[4] == 0x00);
    assert(Serial2.tx_buffer[5] == 0x02);

    // Verify response buffer
    assert(node.getResponseBuffer(0) == 0x1234);
    assert(node.getResponseBuffer(1) == 0x5678);

    std::cout << "test_modbus_read_holding_success PASSED!" << std::endl;
}

void test_modbus_exception_response() {
    std::cout << "Running test_modbus_exception_response..." << std::endl;

    Serial2.clear();
    mock_millis = 1000;

    ModbusMaster node;
    node.begin(1, Serial2);

    // Mock response: Slave ID (0x01) | Exception code 0x83 (0x80 | 0x03) | Exception Value 0x02 (Illegal Data Address)
    std::vector<uint8_t> response = {0x01, 0x83, 0x02};
    push_mock_response(Serial2, response);

    uint8_t result = node.readHoldingRegisters(0x0002, 2);

    // Verify exception code is correctly returned (0x02)
    assert(result == ModbusMaster::ku8MBIllegalDataAddress);

    std::cout << "test_modbus_exception_response PASSED!" << std::endl;
}

void test_modbus_invalid_crc() {
    std::cout << "Running test_modbus_invalid_crc..." << std::endl;

    Serial2.clear();
    mock_millis = 1000;

    ModbusMaster node;
    node.begin(1, Serial2);

    // Mock response: 0x01, 0x03, 0x02, 0xAA, 0xBB + Bad CRC bytes (0x00, 0x00)
    Serial2.rx_buffer = {0x01, 0x03, 0x02, 0xAA, 0xBB, 0x00, 0x00};

    uint8_t result = node.readHoldingRegisters(0x0000, 1);

    // Verify CRC mismatch error code returned
    assert(result == ModbusMaster::ku8MBInvalidCRC);

    std::cout << "test_modbus_invalid_crc PASSED!" << std::endl;
}

void test_modbus_timeout() {
    std::cout << "Running test_modbus_timeout..." << std::endl;

    Serial2.clear();
    mock_millis = 1000;

    ModbusMaster node;
    node.begin(1, Serial2);

    // Provide only 3 bytes of response (incomplete packet)
    Serial2.rx_buffer = {0x01, 0x03, 0x04};

    // Run in a thread or let mock millis advance during read loop
    // In our mocked Serial2, available() returns size, so it will keep reading 3 bytes,
    // and then call delay(1) which increments mock_millis. Once timeout (200ms) is hit, it aborts.
    uint8_t result = node.readHoldingRegisters(0x0000, 2);

    // Verify it timed out
    assert(result == ModbusMaster::ku8MBResponseTimedOut);

    std::cout << "test_modbus_timeout PASSED!" << std::endl;
}

void test_modbus_size_overflow_protection() {
    std::cout << "Running test_modbus_size_overflow_protection..." << std::endl;

    Serial2.clear();
    mock_millis = 1000;

    ModbusMaster node;
    node.begin(1, Serial2);

    // Mock response with a massive byte count value of 250 bytes
    // Expected to fail validation (5 + 250 > 256)
    std::vector<uint8_t> response = {0x01, 0x03, 250, 0x00, 0x00};
    push_mock_response(Serial2, response);

    uint8_t result = node.readHoldingRegisters(0x0000, 2);

    // Verify it failed with illegal data value (ku8MBIllegalDataValue = 0x03)
    assert(result == ModbusMaster::ku8MBIllegalDataValue);

    std::cout << "test_modbus_size_overflow_protection PASSED!" << std::endl;
}

void test_modbus_read_input_registers_success() {
    std::cout << "Running test_modbus_read_input_registers_success..." << std::endl;
    
    Serial2.clear();
    mock_millis = 1000;

    ModbusMaster node;
    node.begin(1, Serial2);

    // Mock response for reading 1 input register (Function 4): 
    // Slave ID (0x01) | Func (0x04) | Byte Count (0x02) | Reg0 High (0xAB) | Reg0 Low (0xCD)
    std::vector<uint8_t> response = {0x01, 0x04, 0x02, 0xAB, 0xCD};
    push_mock_response(Serial2, response);

    uint8_t result = node.readInputRegisters(0x0010, 1);

    // Verify result is success
    assert(result == ModbusMaster::ku8MBSuccess);

    // Verify TX request packet contains correct bytes:
    // Slave ID (0x01) | Func (0x04) | Addr High (0x00) | Addr Low (0x10) | Qty High (0x00) | Qty Low (0x01) | CRC Low | CRC High
    assert(Serial2.tx_buffer.size() == 8);
    assert(Serial2.tx_buffer[0] == 0x01);
    assert(Serial2.tx_buffer[1] == 0x04);
    assert(Serial2.tx_buffer[2] == 0x00);
    assert(Serial2.tx_buffer[3] == 0x10);
    assert(Serial2.tx_buffer[4] == 0x00);
    assert(Serial2.tx_buffer[5] == 0x01);

    // Verify response buffer
    assert(node.getResponseBuffer(0) == 0xABCD);

    std::cout << "test_modbus_read_input_registers_success PASSED!" << std::endl;
}

void test_modbus_write_single_register_success() {
    std::cout << "Running test_modbus_write_single_register_success..." << std::endl;

    Serial2.clear();
    mock_millis = 1000;

    ModbusMaster node;
    node.begin(2, Serial2);

    // Mock response for write single register (Function 6 echoes the request):
    // Slave ID (0x02) | Func (0x06) | Addr High (0x00) | Addr Low (0x05) | Val High (0x12) | Val Low (0x34)
    std::vector<uint8_t> response = {0x02, 0x06, 0x00, 0x05, 0x12, 0x34};
    push_mock_response(Serial2, response);

    uint8_t result = node.writeSingleRegister(0x0005, 0x1234);

    // Verify result is success
    assert(result == ModbusMaster::ku8MBSuccess);

    // Verify TX request
    assert(Serial2.tx_buffer.size() == 8);
    assert(Serial2.tx_buffer[0] == 0x02);
    assert(Serial2.tx_buffer[1] == 0x06);
    assert(Serial2.tx_buffer[2] == 0x00);
    assert(Serial2.tx_buffer[3] == 0x05);
    assert(Serial2.tx_buffer[4] == 0x12);
    assert(Serial2.tx_buffer[5] == 0x34);

    std::cout << "test_modbus_write_single_register_success PASSED!" << std::endl;
}

void test_modbus_invalid_slave_id() {
    std::cout << "Running test_modbus_invalid_slave_id..." << std::endl;

    Serial2.clear();
    mock_millis = 1000;

    ModbusMaster node;
    node.begin(1, Serial2); // Node expects Slave ID = 1

    // Mock response starts with Slave ID = 2
    std::vector<uint8_t> response = {0x02, 0x03, 0x02, 0x12, 0x34};
    push_mock_response(Serial2, response);

    uint8_t result = node.readHoldingRegisters(0x0002, 1);

    // Verify error code returned
    assert(result == ModbusMaster::ku8MBInvalidSlaveID);

    std::cout << "test_modbus_invalid_slave_id PASSED!" << std::endl;
}

void test_modbus_invalid_function_code() {
    std::cout << "Running test_modbus_invalid_function_code..." << std::endl;

    Serial2.clear();
    mock_millis = 1000;

    ModbusMaster node;
    node.begin(1, Serial2);

    // Mock response contains Function Code = 0x04 (Input registers) instead of 0x03 (Holding registers)
    std::vector<uint8_t> response = {0x01, 0x04, 0x02, 0x12, 0x34};
    push_mock_response(Serial2, response);

    uint8_t result = node.readHoldingRegisters(0x0002, 1);

    // Verify error code returned
    assert(result == ModbusMaster::ku8MBInvalidFunction);

    std::cout << "test_modbus_invalid_function_code PASSED!" << std::endl;
}

void test_modbus_null_serial() {
    std::cout << "Running test_modbus_null_serial..." << std::endl;

    ModbusMaster node;
    // Don't call begin() so _serial remains nullptr

    uint8_t result = node.readHoldingRegisters(0x0002, 1);

    // Verify device failure returned due to null serial
    assert(result == ModbusMaster::ku8MBSlaveDeviceFailure);

    std::cout << "test_modbus_null_serial PASSED!" << std::endl;
}

void test_modbus_get_response_buffer_oob() {
    std::cout << "Running test_modbus_get_response_buffer_oob..." << std::endl;

    ModbusMaster node;
    node.clearResponseBuffer();

    // Verify that index 0 returns 0 (cleared value)
    assert(node.getResponseBuffer(0) == 0);

    // Verify that out of bound index (e.g., 64 or larger) returns 0xFFFF
    assert(node.getResponseBuffer(64) == 0xFFFF);
    assert(node.getResponseBuffer(100) == 0xFFFF);

    std::cout << "test_modbus_get_response_buffer_oob PASSED!" << std::endl;
}
