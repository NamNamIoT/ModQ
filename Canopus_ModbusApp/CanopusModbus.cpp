#include "CanopusModbus.h"

ModbusMaster::ModbusMaster(void)
{
    _serial = nullptr;
    _u8MBSlave = 0;
    clearResponseBuffer();
}

void ModbusMaster::begin(uint8_t slave, HardwareSerial &serial)
{
    _u8MBSlave = slave;
    _serial = &serial;
}

uint16_t ModbusMaster::getResponseBuffer(uint8_t u8Index)
{
    if (u8Index < ku8MaxBufferSize)
    {
        return _u16ResponseBuffer[u8Index];
    }
    return 0xFFFF;
}

void ModbusMaster::clearResponseBuffer()
{
    for (uint8_t i = 0; i < ku8MaxBufferSize; i++)
    {
        _u16ResponseBuffer[i] = 0;
    }
    _u8ResponseBufferLength = 0;
}

uint8_t ModbusMaster::readHoldingRegisters(uint16_t u16ReadAddress, uint16_t u16ReadQty)
{
    _u16ReadAddress = u16ReadAddress;
    _u16ReadQty = u16ReadQty;
    return ModbusMasterTransaction(ku8MBReadHoldingRegisters);
}

uint8_t ModbusMaster::readInputRegisters(uint16_t u16ReadAddress, uint16_t u16ReadQty)
{
    _u16ReadAddress = u16ReadAddress;
    _u16ReadQty = u16ReadQty;
    return ModbusMasterTransaction(ku8MBReadInputRegisters);
}

uint8_t ModbusMaster::writeSingleRegister(uint16_t u16WriteAddress, uint16_t u16WriteValue)
{
    _u16WriteAddress = u16WriteAddress;
    _u16WriteValue = u16WriteValue;
    return ModbusMasterTransaction(ku8MBWriteSingleRegister);
}

uint8_t ModbusMaster::ModbusMasterTransaction(uint8_t u8MBFunction)
{
    if (!_serial) return ku8MBSlaveDeviceFailure;

    uint8_t u8ModbusADU[256];
    uint8_t u8ModbusADUSize = 0;
    uint16_t u16CRC;
    uint32_t u32StartTime;
    uint8_t u8BytesLeft = 8; // Default minimum frame size (Slave(1) + Func(1) + Addr(2) + Qty/Val(2) + CRC(2))
    uint8_t u8MBStatus = ku8MBSuccess;

    // Clear response buffer
    clearResponseBuffer();

    // Assemble Modbus Request ADU
    u8ModbusADU[u8ModbusADUSize++] = _u8MBSlave;
    u8ModbusADU[u8ModbusADUSize++] = u8MBFunction;

    switch (u8MBFunction)
    {
        case ku8MBReadHoldingRegisters:
        case ku8MBReadInputRegisters:
            u8ModbusADU[u8ModbusADUSize++] = highByte(_u16ReadAddress);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16ReadAddress);
            u8ModbusADU[u8ModbusADUSize++] = highByte(_u16ReadQty);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16ReadQty);
            break;

        case ku8MBWriteSingleRegister:
            u8ModbusADU[u8ModbusADUSize++] = highByte(_u16WriteAddress);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16WriteAddress);
            u8ModbusADU[u8ModbusADUSize++] = highByte(_u16WriteValue);
            u8ModbusADU[u8ModbusADUSize++] = lowByte(_u16WriteValue);
            break;
    }

    // Append CRC
    u16CRC = 0xFFFF;
    for (uint8_t i = 0; i < u8ModbusADUSize; i++)
    {
        u16CRC = crc16_update(u16CRC, u8ModbusADU[i]);
    }
    u8ModbusADU[u8ModbusADUSize++] = lowByte(u16CRC);
    u8ModbusADU[u8ModbusADUSize++] = highByte(u16CRC);

    // Flush serial receive buffer before transmitting
    while (_serial->read() != -1);

    // Transmit request
    for (uint8_t i = 0; i < u8ModbusADUSize; i++)
    {
        _serial->write(u8ModbusADU[i]);
    }
    _serial->flush();

    // Reset frame read state
    u8ModbusADUSize = 0;
    u32StartTime = millis();

    while (u8BytesLeft && !u8MBStatus)
    {
        if (_serial->available())
        {
            if (u8ModbusADUSize >= 256)
            {
                u8MBStatus = ku8MBSlaveDeviceFailure;
                break;
            }
            u8ModbusADU[u8ModbusADUSize++] = _serial->read();
            u8BytesLeft--;
        }
        else
        {
            delay(1); // Yield to FreeRTOS scheduler to prevent CPU pegging
        }

        // Evaluate slave ID, function code once first 5 bytes are read
        if (u8ModbusADUSize == 5)
        {
            // Verify correct Modbus slave ID
            if (u8ModbusADU[0] != _u8MBSlave)
            {
                u8MBStatus = ku8MBInvalidSlaveID;
                break;
            }

            // Verify correct Modbus function code (ignoring error bit 7)
            if ((u8ModbusADU[1] & 0x7F) != u8MBFunction)
            {
                u8MBStatus = ku8MBInvalidFunction;
                break;
            }

            // Check if exception code is returned (bit 7 of function code is set)
            if (u8ModbusADU[1] & 0x80)
            {
                u8MBStatus = u8ModbusADU[2]; // Modbus Exception Code (e.g. 0x01, 0x02)
                break;
            }

            // Verify that the requested byte count is safe and fits in u8ModbusADU
            if (u8ModbusADU[1] == ku8MBReadHoldingRegisters || u8ModbusADU[1] == ku8MBReadInputRegisters)
            {
                if (u8ModbusADU[2] < 2 || u8ModbusADU[2] > (ku8MaxBufferSize * 2) || (5 + u8ModbusADU[2]) > 256) {
                    u8MBStatus = ku8MBIllegalDataValue;
                    break;
                }
            }

            // Determine bytes remaining to read
            switch (u8ModbusADU[1])
            {
                case ku8MBReadHoldingRegisters:
                case ku8MBReadInputRegisters:
                    // Byte count is at index 2
                    u8BytesLeft = u8ModbusADU[2]; 
                    break;
                case ku8MBWriteSingleRegister:
                    // Response has 8 bytes total (we have read 5, so 3 left)
                    u8BytesLeft = 3;
                    break;
            }
        }

        if ((millis() - u32StartTime) > ku16MBResponseTimeout)
        {
            u8MBStatus = ku8MBResponseTimedOut;
        }
    }

    // Verify response integrity
    if (!u8MBStatus && u8ModbusADUSize >= 5)
    {
        // Calculate CRC
        u16CRC = 0xFFFF;
        for (uint8_t i = 0; i < (u8ModbusADUSize - 2); i++)
        {
            u16CRC = crc16_update(u16CRC, u8ModbusADU[i]);
        }

        // Verify CRC
        if ((lowByte(u16CRC) != u8ModbusADU[u8ModbusADUSize - 2]) ||
            (highByte(u16CRC) != u8ModbusADU[u8ModbusADUSize - 1]))
        {
            u8MBStatus = ku8MBInvalidCRC;
        }
    }

    // Load response bytes into getResponseBuffer
    if (!u8MBStatus)
    {
        switch (u8ModbusADU[1])
        {
            case ku8MBReadHoldingRegisters:
            case ku8MBReadInputRegisters:
                // Response bytes are ordered H, L, H, L ... starting at byte 3
                for (uint8_t i = 0; i < (u8ModbusADU[2] >> 1); i++)
                {
                    if (i < ku8MaxBufferSize)
                    {
                        _u16ResponseBuffer[i] = word(u8ModbusADU[2 * i + 3], u8ModbusADU[2 * i + 4]);
                        _u8ResponseBufferLength = i + 1;
                    }
                }
                break;
        }
    }

    // Small delay to prevent back-to-back Modbus query issues on the bus
    delay(10);
    return u8MBStatus;
}
