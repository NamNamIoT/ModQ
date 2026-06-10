#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define MAX_WIFI_NETS 3
#define MAX_DEVICES 8
#define MAX_REGS 16

struct WiFiNet {
    char ssid[33];
    char password[65];
};

struct MqttConfig {
    char server[65];
    uint16_t port;
    char user[33];
    char password[65];
    char clientId[33];
    char dataTopic[128]; // Topic to publish Modbus data
    char cmdTopic[128];  // Topic to subscribe for commands/writes
};

enum DataType {
    TYPE_UINT16 = 0,
    TYPE_INT16 = 1,
    TYPE_UINT32 = 2,
    TYPE_INT32 = 3,
    TYPE_FLOAT = 4
};

struct RegConfig {
    uint16_t address;
    uint8_t funcCode; // 3 = Holding, 4 = Input
    char name[32];
    char unit[16];
    uint8_t dataType; // enum DataType
    float multiplier;
};

struct DeviceConfig {
    bool active;
    char name[32];
    uint8_t slaveId;
    uint32_t baudRate;
    uint32_t readInterval; // milliseconds
    uint8_t regCount;
    RegConfig registers[MAX_REGS];
};

struct AppConfig {
    WiFiNet wifis[MAX_WIFI_NETS];
    uint8_t wifiCount;
    MqttConfig mqtt;
    DeviceConfig devices[MAX_DEVICES];
};

// Global configuration variable
extern AppConfig config;

// Status & Data tracking
struct RegValue {
    float floatVal;
    uint32_t rawVal;
    bool valid;
    uint32_t updateTime;
};

struct DeviceStatus {
    bool online;
    uint32_t lastAttemptTime;
    uint32_t lastSuccessTime;
    uint8_t lastError;
    RegValue regValues[MAX_REGS];
};

extern DeviceStatus deviceStates[MAX_DEVICES];

namespace Settings {
    void init();
    void load();
    void save();
    void reset();
    String getAsJson();
    bool saveFromJson(const String& jsonStr);
}

#endif // SETTINGS_H
