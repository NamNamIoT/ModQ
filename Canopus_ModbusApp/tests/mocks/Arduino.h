#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H

#include <stdint.h>
#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cstdarg>

// Define Arduino PROGMEM macros
#define PROGMEM
#define PSTR(s) (s)
#define R(s) s

// Arduino serial configurations
#define SERIAL_8N1 0x06

// Mock Arduino String class
class String : public std::string {
public:
    String() : std::string() {}
    String(const char* s) : std::string(s ? s : "") {}
    String(const std::string& s) : std::string(s) {}
    
    void trim() {
        size_t first = find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            clear();
            return;
        }
        size_t last = find_last_not_of(" \t\r\n");
        *this = substr(first, (last - first + 1));
    }
    
    bool equalsIgnoreCase(const String& s) const {
        if (size() != s.size()) return false;
        for (size_t i = 0; i < size(); i++) {
            if (tolower((*this)[i]) != tolower(s[i])) return false;
        }
        return true;
    }
    
    bool startsWith(const String& prefix) const {
        return compare(0, prefix.size(), prefix) == 0;
    }
    
    int indexOf(char c) const {
        size_t pos = find(c);
        return (pos == std::string::npos) ? -1 : (int)pos;
    }
    
    String substring(size_t from, size_t to = std::string::npos) const {
        if (to == std::string::npos) return substr(from);
        if (to <= from) return "";
        return substr(from, to - from);
    }
    
    int toInt() const {
        try {
            return std::stoi(*this);
        } catch (...) {
            return 0;
        }
    }
    
    float toFloat() const {
        try {
            return std::stof(*this);
        } catch (...) {
            return 0.0f;
        }
    }
    
    size_t length() const {
        return size();
    }
};

// Conversions
inline uint8_t lowByte(uint16_t w) { return w & 0xFF; }
inline uint8_t highByte(uint16_t w) { return (w >> 8) & 0xFF; }
inline uint16_t word(uint8_t h, uint8_t l) { return ((uint16_t)h << 8) | l; }

// Mock timing
extern uint32_t mock_millis;
inline uint32_t millis() { return mock_millis; }
inline void delay(uint32_t ms) { mock_millis += ms; }
inline void yield() {}

// Mock HardwareSerial
class HardwareSerial {
public:
    std::vector<uint8_t> rx_buffer;
    std::vector<uint8_t> tx_buffer;
    bool end_called = false;
    uint32_t baud = 0;

    void begin(uint32_t baudrate, uint32_t config = SERIAL_8N1, int rx = -1, int tx = -1) {
        baud = baudrate;
        end_called = false;
    }

    void end() {
        end_called = true;
    }
    
    int read() {
        if (tx_buffer.empty()) return -1;
        if (rx_buffer.empty()) return -1;
        uint8_t val = rx_buffer.front();
        rx_buffer.erase(rx_buffer.begin());
        return val;
    }
    
    size_t write(uint8_t val) {
        tx_buffer.push_back(val);
        return 1;
    }
    
    int available() {
        if (tx_buffer.empty()) return 0;
        return rx_buffer.size();
    }
    
    void flush() {}
    
    // Test helper APIs
    void clear() {
        rx_buffer.clear();
        tx_buffer.clear();
    }
};

extern HardwareSerial Serial2;

// Mock ESP system
class MockESP {
public:
    uint64_t getEfuseMac() { return 0x1234567890ABCDEFULL; }
    uint32_t getFreeHeap() { return 128000; }
};
extern MockESP ESP;

// Mock Serial console output
class MockSerial {
public:
    template<typename T>
    void print(T val) { std::cout << val; }
    template<typename T>
    void println(T val) { std::cout << val << std::endl; }
    void println() { std::cout << std::endl; }
    
    void printf(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
};
extern MockSerial Serial;

#endif // MOCK_ARDUINO_H
