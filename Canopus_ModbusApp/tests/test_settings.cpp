#include "mocks/Arduino.h"
#include "mocks/Preferences.h"
#include "Settings.h"
#include <cassert>
#include <iostream>

// Instantiate global NVS storage mock
std::map<std::string, std::string> mock_nvs_storage;

// Mocks for globals needed by compiled C++ implementation files
HardwareSerial Serial2;
MockESP ESP;
MockSerial Serial;
uint32_t mock_millis = 0;

void test_settings_defaults() {
    std::cout << "Running test_settings_defaults..." << std::endl;

    mock_nvs_storage.clear();
    Settings::init();

    // Verify default configs loaded
    assert(config.wifiCount == 0);
    assert(std::string(config.mqtt.server) == "broker.hivemq.com");
    assert(config.mqtt.port == 1883);
    assert(config.devices[0].active == true);
    assert(std::string(config.devices[0].name) == "Demo Temp Sensor");

    std::cout << "test_settings_defaults PASSED!" << std::endl;
}

void test_settings_json_serialization() {
    std::cout << "Running test_settings_json_serialization..." << std::endl;

    mock_nvs_storage.clear();
    Settings::init(); // Loads defaults

    String json = Settings::getAsJson();
    assert(json.length() > 0);
    
    // Check if JSON contains default MQTT server and device name
    assert(json.find("broker.hivemq.com") != std::string::npos);
    assert(json.find("Demo Temp Sensor") != std::string::npos);

    std::cout << "test_settings_json_serialization PASSED!" << std::endl;
}

void test_settings_save_load_nvs() {
    std::cout << "Running test_settings_save_load_nvs..." << std::endl;

    mock_nvs_storage.clear();
    Settings::init();
    
    // Set a custom config field and save to NVS
    strcpy(config.mqtt.server, "test.broker.org");
    config.mqtt.port = 8883;
    config.wifiCount = 1;
    strcpy(config.wifis[0].ssid, "TestSSID");
    strcpy(config.wifis[0].password, "TestPass");

    Settings::save(); // Saves to Preferences

    // Verify it is saved in our simulated Preferences storage map
    assert(mock_nvs_storage.size() > 0);

    // Reset configuration in memory
    Settings::reset();
    assert(std::string(config.mqtt.server) == "broker.hivemq.com"); // Back to default

    // Load from Preferences
    Settings::load();

    // Verify custom settings are loaded back
    assert(std::string(config.mqtt.server) == "test.broker.org");
    assert(config.mqtt.port == 8883);
    assert(config.wifiCount == 1);
    assert(std::string(config.wifis[0].ssid) == "TestSSID");

    std::cout << "test_settings_save_load_nvs PASSED!" << std::endl;
}

void test_settings_json_import() {
    std::cout << "Running test_settings_json_import..." << std::endl;

    mock_nvs_storage.clear();
    Settings::init();

    // A sample JSON config input to import
    String jsonStr = "{"
        "\"wifis\":[{\"ssid\":\"HomeWiFi\",\"pass\":\"secret123\"}],"
        "\"mqtt\":{\"server\":\"mqtt.myhouse.com\",\"port\":1884,\"user\":\"usr\",\"pass\":\"pwd\",\"cid\":\"id\",\"dtop\":\"pub\",\"ctop\":\"sub\"},"
        "\"devices\":[{\"index\":0,\"name\":\"TempSensor\",\"sid\":5,\"baud\":115200,\"int\":3000,\"regs\":[{\"addr\":10,\"fn\":4,\"name\":\"Temperature\",\"unit\":\"C\",\"type\":4,\"mult\":1.0}]}]"
    "}";

    bool success = Settings::saveFromJson(jsonStr);
    assert(success);

    // Verify imported configuration
    assert(config.wifiCount == 1);
    assert(std::string(config.wifis[0].ssid) == "HomeWiFi");
    assert(std::string(config.mqtt.server) == "mqtt.myhouse.com");
    assert(config.mqtt.port == 1884);
    
    assert(config.devices[0].active == true);
    assert(std::string(config.devices[0].name) == "TempSensor");
    assert(config.devices[0].slaveId == 5);
    assert(config.devices[0].baudRate == 115200);
    assert(config.devices[0].readInterval == 3000);
    assert(config.devices[0].regCount == 1);
    assert(config.devices[0].registers[0].address == 10);
    assert(config.devices[0].registers[0].funcCode == 4);
    assert(config.devices[0].registers[0].dataType == 4); // TYPE_FLOAT

    std::cout << "test_settings_json_import PASSED!" << std::endl;
}

void test_settings_invalid_json() {
    std::cout << "Running test_settings_invalid_json..." << std::endl;

    mock_nvs_storage.clear();
    Settings::init();

    // Malformed JSON (missing closing braces and quotes)
    String malformedJson = "{\"wifis\":[{\"ssid\":\"TestWiFi\"";
    
    bool success = Settings::saveFromJson(malformedJson);
    
    // Deserialization should fail and return false
    assert(!success);

    std::cout << "test_settings_invalid_json PASSED!" << std::endl;
}

void test_settings_empty_json() {
    std::cout << "Running test_settings_empty_json..." << std::endl;

    mock_nvs_storage.clear();
    Settings::init();

    // Correct JSON format but totally empty configuration object
    String emptyJson = "{}";
    
    bool success = Settings::saveFromJson(emptyJson);
    
    // Save should succeed (no error parsing), but keep existing defaults
    assert(success);
    
    // Check that default values are still intact
    assert(config.wifiCount == 0);
    assert(std::string(config.mqtt.server) == "broker.hivemq.com");

    std::cout << "test_settings_empty_json PASSED!" << std::endl;
}

void test_settings_max_wifi_nets() {
    std::cout << "Running test_settings_max_wifi_nets..." << std::endl;

    mock_nvs_storage.clear();
    Settings::init();

    // Attempt to load 4 wifi networks (MAX_WIFI_NETS is 3)
    String jsonStr = "{"
        "\"wifis\":["
            "{\"ssid\":\"WiFi1\",\"pass\":\"pw1\"},"
            "{\"ssid\":\"WiFi2\",\"pass\":\"pw2\"},"
            "{\"ssid\":\"WiFi3\",\"pass\":\"pw3\"},"
            "{\"ssid\":\"WiFi4\",\"pass\":\"pw4\"}"
        "]"
    "}";

    bool success = Settings::saveFromJson(jsonStr);
    assert(success);

    // Verify it only saved 3 wifi networks (ignoring the 4th)
    assert(config.wifiCount == 3);
    assert(std::string(config.wifis[0].ssid) == "WiFi1");
    assert(std::string(config.wifis[1].ssid) == "WiFi2");
    assert(std::string(config.wifis[2].ssid) == "WiFi3");

    std::cout << "test_settings_max_wifi_nets PASSED!" << std::endl;
}

void test_settings_max_devices() {
    std::cout << "Running test_settings_max_devices..." << std::endl;

    mock_nvs_storage.clear();
    Settings::init();

    // Attempt to load devices with indices exceeding MAX_DEVICES (e.g. index 8, 9 since indices are 0-7)
    // Also including valid ones
    String jsonStr = "{"
        "\"devices\":["
            "{\"index\":0,\"name\":\"Dev0\",\"sid\":1},"
            "{\"index\":7,\"name\":\"Dev7\",\"sid\":8},"
            "{\"index\":8,\"name\":\"Dev8-OOB\",\"sid\":9},"
            "{\"index\":-1,\"name\":\"DevNeg-OOB\",\"sid\":10}"
        "]"
    "}";

    bool success = Settings::saveFromJson(jsonStr);
    assert(success);

    // Verify only the valid indexed devices (0 and 7) were configured
    assert(config.devices[0].active == true);
    assert(std::string(config.devices[0].name) == "Dev0");
    assert(config.devices[7].active == true);
    assert(std::string(config.devices[7].name) == "Dev7");

    // The other two should not have been saved or modified
    assert(config.devices[1].active == false); // not active
    // If the parser checks boundaries index < 0 or >= MAX_DEVICES, it should ignore them.
    // Let's make sure it is not crash and not active.

    std::cout << "test_settings_max_devices PASSED!" << std::endl;
}
