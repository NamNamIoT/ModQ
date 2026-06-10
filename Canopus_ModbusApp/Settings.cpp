#include "Settings.h"
#include <Preferences.h>

AppConfig config;
DeviceStatus deviceStates[MAX_DEVICES];

namespace Settings {
    const char* PREFS_NAMESPACE = "canopus";
    const char* PREFS_KEY = "cfg";

    void reset() {
        memset(&config, 0, sizeof(AppConfig));
        
        // WiFi Defaults (empty)
        config.wifiCount = 0;

        // MQTT Defaults
        strcpy(config.mqtt.server, "broker.hivemq.com");
        config.mqtt.port = 1883;
        strcpy(config.mqtt.user, "");
        strcpy(config.mqtt.password, "");
        sprintf(config.mqtt.clientId, "canopus_%06X", (uint32_t)(ESP.getEfuseMac() & 0xFFFFFF));
        strcpy(config.mqtt.dataTopic, "canopus/device/data");
        strcpy(config.mqtt.cmdTopic, "canopus/device/cmd");

        // Device Defaults (1 Demo Device)
        config.devices[0].active = true;
        strcpy(config.devices[0].name, "Demo Temp Sensor");
        config.devices[0].slaveId = 1;
        config.devices[0].baudRate = 9600;
        config.devices[0].readInterval = 5000; // 5 seconds
        config.devices[0].regCount = 2;
        
        // Reg 1: Temp
        config.devices[0].registers[0].address = 0;
        config.devices[0].registers[0].funcCode = 3; // Holding
        strcpy(config.devices[0].registers[0].name, "Temperature");
        strcpy(config.devices[0].registers[0].unit, "°C");
        config.devices[0].registers[0].dataType = TYPE_INT16;
        config.devices[0].registers[0].multiplier = 0.1f;

        // Reg 2: Humidity
        config.devices[0].registers[1].address = 1;
        config.devices[0].registers[1].funcCode = 3; // Holding
        strcpy(config.devices[0].registers[1].name, "Humidity");
        strcpy(config.devices[0].registers[1].unit, "%");
        config.devices[0].registers[1].dataType = TYPE_UINT16;
        config.devices[0].registers[1].multiplier = 0.1f;

        // Reset all states
        memset(deviceStates, 0, sizeof(deviceStates));
    }

    void init() {
        reset();
    }

    String getAsJson() {
#if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument doc;
#else
        DynamicJsonDocument doc(16384);
#endif
        
        // Serialize WiFi
        JsonArray wifisArr = doc.createNestedArray("wifis");
        for (int i = 0; i < config.wifiCount; i++) {
            JsonObject wifi = wifisArr.createNestedObject();
            wifi["ssid"] = config.wifis[i].ssid;
            wifi["pass"] = config.wifis[i].password;
        }

        // Serialize MQTT
        JsonObject mqtt = doc.createNestedObject("mqtt");
        mqtt["server"] = config.mqtt.server;
        mqtt["port"] = config.mqtt.port;
        mqtt["user"] = config.mqtt.user;
        mqtt["pass"] = config.mqtt.password;
        mqtt["cid"] = config.mqtt.clientId;
        mqtt["dtop"] = config.mqtt.dataTopic;
        mqtt["ctop"] = config.mqtt.cmdTopic;

        // Serialize Devices
        JsonArray devicesArr = doc.createNestedArray("devices");
        for (int i = 0; i < MAX_DEVICES; i++) {
            if (!config.devices[i].active) continue;
            JsonObject dev = devicesArr.createNestedObject();
            dev["index"] = i;
            dev["name"] = config.devices[i].name;
            dev["sid"] = config.devices[i].slaveId;
            dev["baud"] = config.devices[i].baudRate;
            dev["int"] = config.devices[i].readInterval;
            
            JsonArray regsArr = dev.createNestedArray("regs");
            for (int j = 0; j < config.devices[i].regCount; j++) {
                JsonObject reg = regsArr.createNestedObject();
                reg["addr"] = config.devices[i].registers[j].address;
                reg["fn"] = config.devices[i].registers[j].funcCode;
                reg["name"] = config.devices[i].registers[j].name;
                reg["unit"] = config.devices[i].registers[j].unit;
                reg["type"] = config.devices[i].registers[j].dataType;
                reg["mult"] = config.devices[i].registers[j].multiplier;
            }
        }

        String jsonStr;
        serializeJson(doc, jsonStr);
        return jsonStr;
    }

    bool saveFromJson(const String& jsonStr) {
#if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument doc;
#else
        DynamicJsonDocument doc(16384);
#endif
        DeserializationError error = deserializeJson(doc, jsonStr);
        if (error) {
            Serial.printf("Settings: Deserialization failed: %s\n", error.c_str());
            return false;
        }

        // Load WiFi
        if (doc.containsKey("wifis")) {
            JsonArray wifisArr = doc["wifis"].as<JsonArray>();
            config.wifiCount = 0;
            for (JsonObject wifi : wifisArr) {
                if (config.wifiCount >= MAX_WIFI_NETS) break;
                int idx = config.wifiCount++;
                strncpy(config.wifis[idx].ssid, wifi["ssid"] | "", 32);
                config.wifis[idx].ssid[32] = '\0';
                strncpy(config.wifis[idx].password, wifi["pass"] | "", 64);
                config.wifis[idx].password[64] = '\0';
            }
        }

        // Load MQTT
        if (doc.containsKey("mqtt")) {
            JsonObject mqtt = doc["mqtt"].as<JsonObject>();
            strncpy(config.mqtt.server, mqtt["server"] | "", 64);
            config.mqtt.server[64] = '\0';
            config.mqtt.port = mqtt["port"] | 1883;
            strncpy(config.mqtt.user, mqtt["user"] | "", 32);
            config.mqtt.user[32] = '\0';
            strncpy(config.mqtt.password, mqtt["pass"] | "", 64);
            config.mqtt.password[64] = '\0';
            strncpy(config.mqtt.clientId, mqtt["cid"] | "canopus_node", 32);
            config.mqtt.clientId[32] = '\0';
            strncpy(config.mqtt.dataTopic, mqtt["dtop"] | "canopus/device/data", 127);
            config.mqtt.dataTopic[127] = '\0';
            strncpy(config.mqtt.cmdTopic, mqtt["ctop"] | "canopus/device/cmd", 127);
            config.mqtt.cmdTopic[127] = '\0';
        }

        // Load Devices
        if (doc.containsKey("devices")) {
            // First mark all inactive
            for (int i = 0; i < MAX_DEVICES; i++) {
                config.devices[i].active = false;
            }

            JsonArray devicesArr = doc["devices"].as<JsonArray>();
            for (JsonObject dev : devicesArr) {
                int idx = dev["index"] | -1;
                if (idx < 0 || idx >= MAX_DEVICES) continue;

                config.devices[idx].active = true;
                strncpy(config.devices[idx].name, dev["name"] | "", 31);
                config.devices[idx].name[31] = '\0';
                config.devices[idx].slaveId = dev["sid"] | 1;
                config.devices[idx].baudRate = dev["baud"] | 9600;
                config.devices[idx].readInterval = dev["int"] | 5000;
                config.devices[idx].regCount = 0;

                if (dev.containsKey("regs")) {
                    JsonArray regsArr = dev["regs"].as<JsonArray>();
                    for (JsonObject reg : regsArr) {
                        if (config.devices[idx].regCount >= MAX_REGS) break;
                        int regIdx = config.devices[idx].regCount++;
                        RegConfig& rc = config.devices[idx].registers[regIdx];

                        rc.address = reg["addr"] | 0;
                        rc.funcCode = reg["fn"] | 3;
                        strncpy(rc.name, reg["name"] | "", 31);
                        rc.name[31] = '\0';
                        strncpy(rc.unit, reg["unit"] | "", 15);
                        rc.unit[15] = '\0';
                        rc.dataType = reg["type"] | 0;
                        rc.multiplier = reg["mult"] | 1.0f;
                    }
                }
            }
        }

        save();
        return true;
    }

    void load() {
        Preferences prefs;
        prefs.begin(PREFS_NAMESPACE, true);
        String jsonStr = prefs.getString(PREFS_KEY, "");
        prefs.end();

        if (jsonStr.length() > 0) {
            Serial.println("Settings: Loading config from NVS...");
            if (!saveFromJson(jsonStr)) {
                Serial.println("Settings: Failed to load config, resetting...");
                reset();
                save();
            }
        } else {
            Serial.println("Settings: No config in NVS, saving defaults...");
            reset();
            save();
        }
    }

    void save() {
        String jsonStr = getAsJson();
        Preferences prefs;
        prefs.begin(PREFS_NAMESPACE, false);
        prefs.putString(PREFS_KEY, jsonStr);
        prefs.end();
        Serial.println("Settings: Configuration saved to NVS.");
    }
}
