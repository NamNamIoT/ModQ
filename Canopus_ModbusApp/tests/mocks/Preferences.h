#ifndef MOCK_PREFERENCES_H
#define MOCK_PREFERENCES_H

#include "Arduino.h"
#include <map>
#include <string>

// Global NVS storage simulator for testing
extern std::map<std::string, std::string> mock_nvs_storage;

class Preferences {
private:
    std::string _namespace;
    bool _readOnly;
public:
    Preferences() : _readOnly(false) {}
    
    bool begin(const char* name, bool readOnly = false) {
        _namespace = name;
        _readOnly = readOnly;
        return true;
    }
    
    void end() {}
    
    size_t putString(const char* key, const String& value) {
        if (_readOnly) return 0;
        std::string full_key = _namespace + "/" + key;
        mock_nvs_storage[full_key] = std::string(value.c_str());
        return value.length();
    }
    
    String getString(const char* key, const String& defaultValue = "") {
        std::string full_key = _namespace + "/" + key;
        if (mock_nvs_storage.find(full_key) != mock_nvs_storage.end()) {
            return String(mock_nvs_storage[full_key]);
        }
        return defaultValue;
    }
};

#endif // MOCK_PREFERENCES_H
