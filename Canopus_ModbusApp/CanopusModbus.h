#ifndef CANOPUS_MODBUS_H
#define CANOPUS_MODBUS_H

#include "Arduino.h"
#include "util/crc16.h"
#include "util/word.h"

class ModbusMaster
{
  public:
    ModbusMaster();
    
    void begin(uint8_t slave, HardwareSerial &serial);
    
    // Modbus exception codes
    static const uint8_t ku8MBIllegalFunction            = 0x01;
    static const uint8_t ku8MBIllegalDataAddress         = 0x02;
    static const uint8_t ku8MBIllegalDataValue           = 0x03;
    static const uint8_t ku8MBSlaveDeviceFailure         = 0x04;

    // Success/error codes
    static const uint8_t ku8MBSuccess                    = 0x00;
    static const uint8_t ku8MBInvalidSlaveID             = 0xE0;
    static const uint8_t ku8MBInvalidFunction            = 0xE1;
    static const uint8_t ku8MBResponseTimedOut           = 0xE2;
    static const uint8_t ku8MBInvalidCRC                 = 0xE3;
    
    uint16_t getResponseBuffer(uint8_t u8Index);
    void     clearResponseBuffer();
    
    uint8_t  readHoldingRegisters(uint16_t u16ReadAddress, uint16_t u16ReadQty);
    uint8_t  readInputRegisters(uint16_t u16ReadAddress, uint16_t u16ReadQty);
    uint8_t  writeSingleRegister(uint16_t u16WriteAddress, uint16_t u16WriteValue);

  private:
    HardwareSerial* _serial;
    uint8_t  _u8MBSlave;
    static const uint8_t ku8MaxBufferSize                = 64;
    uint16_t _u16ReadAddress;
    uint16_t _u16ReadQty;
    uint16_t _u16ResponseBuffer[ku8MaxBufferSize];
    uint16_t _u16WriteAddress;
    uint16_t _u16WriteValue;
    uint8_t  _u8ResponseBufferLength;
    
    // Modbus function codes
    static const uint8_t ku8MBReadHoldingRegisters       = 0x03;
    static const uint8_t ku8MBReadInputRegisters         = 0x04;
    static const uint8_t ku8MBWriteSingleRegister        = 0x06;
    
    static const uint16_t ku16MBResponseTimeout          = 200; // ms
    
    uint8_t ModbusMasterTransaction(uint8_t u8MBFunction);
};

#endif // CANOPUS_MODBUS_H
