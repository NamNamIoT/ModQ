#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "Settings.h"
#include "CanopusModbus.h"
#include "WebUI.h"

// Hardware Pin Definitions
#define LED_SYS_PIN 25 // LED 1 (System / WiFi / HTTP activity)
#define LED_MB_PIN  27 // LED 2 (Modbus RTU activity)

// Globals
WiFiMulti wifiMulti;
WebServer server(80);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
ModbusMaster node;

uint32_t currentBaudRate = 0;
uint32_t lastPollTime[MAX_DEVICES] = {0};
uint32_t lastMqttPubTime = 0;
uint32_t lastMqttAttemptTime = 0;
bool wifiConnected = false;
bool apActive = false;

// System activity LED control variables
uint32_t ledSysOffTime = 0;
uint32_t ledMbOffTime = 0;

// Function declarations
void setupWiFi();
void setupWebServer();
void setupMqtt();
void handleMqttCallback(char* topic, byte* payload, unsigned int length);
void processModbusPolls();
void triggerSysLedFlash(uint32_t durationMs);
void triggerMbLedFlash(uint32_t durationMs);
void reconnectMqtt();
void publishDeviceData(int devIdx);
void checkSerialCommands();
void executeSerialCommand(String cmd);

// --- HTTP API Handlers ---
void handleRoot() {
    triggerSysLedFlash(100);
    server.send_P(200, "text/html", INDEX_HTML);
}

void handleGetConfig() {
    triggerSysLedFlash(100);
    server.send(200, "application/json", Settings::getAsJson());
}

void handlePostConfig() {
    triggerSysLedFlash(100);
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing body\"}");
        return;
    }
    
    String body = server.arg("plain");
    if (Settings::saveFromJson(body)) {
        server.send(200, "application/json", "{\"status\":\"success\"}");
        
        // Re-configure WiFi
        setupWiFi();
        // Re-configure MQTT
        setupMqtt();
    } else {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
    }
}

void handleGetData() {
    triggerSysLedFlash(50);
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    DynamicJsonDocument doc(8192);
#endif

    for (int i = 0; i < MAX_DEVICES; i++) {
        if (!config.devices[i].active) continue;
        
        JsonObject devObj = doc.createNestedObject(String(i));
        devObj["online"] = deviceStates[i].online;
        devObj["lastError"] = deviceStates[i].lastError;
        
        JsonObject regsObj = devObj.createNestedObject("regs");
        for (int j = 0; j < config.devices[i].regCount; j++) {
            if (deviceStates[i].regValues[j].valid) {
                regsObj[String(j)] = deviceStates[i].regValues[j].floatVal;
            }
        }
    }

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleGetStatus() {
    triggerSysLedFlash(50);
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    DynamicJsonDocument doc(1024);
#endif

    doc["wifiConnected"] = (WiFi.status() == WL_CONNECTED);
    doc["wifiSsid"] = WiFi.SSID();
    doc["apActive"] = apActive;
    doc["mqttConnected"] = mqttClient.connected();
    doc["uptime"] = millis() / 1000;
    doc["ipAddress"] = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
    doc["freeHeap"] = ESP.getFreeHeap();

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleWifiScan() {
    triggerSysLedFlash(200);
    int n = WiFi.scanNetworks();
#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    DynamicJsonDocument doc(4096);
#endif
    JsonArray arr = doc.to<JsonArray>();

    for (int i = 0; i < n; ++i) {
        JsonObject obj = arr.createNestedObject();
        obj["ssid"] = WiFi.SSID(i);
        obj["rssi"] = WiFi.RSSI(i);
        obj["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    }

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
    WiFi.scanDelete();
}

void handleRestart() {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
    delay(500);
    ESP.restart();
}

void handleReset() {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
    delay(500);
    Settings::reset();
    Settings::save();
    ESP.restart();
}

// --- Arduino Core functions ---
void setup() {
    // LED Indicators
    pinMode(LED_SYS_PIN, OUTPUT);
    pinMode(LED_MB_PIN, OUTPUT);
    digitalWrite(LED_SYS_PIN, LOW);
    digitalWrite(LED_MB_PIN, LOW);

    Serial.begin(115200);
    Serial.println("\n--- ESP32 ModQ Modbus Gateway starting ---");

    // Initialize Settings (from NVS)
    Settings::init();
    Settings::load();

    // Start Modbus Default Baudrate (will change dynamically if needed)
    currentBaudRate = 9600;
    Serial2.begin(currentBaudRate, SERIAL_8N1, 16, 17);
    Serial.println("Modbus Serial2 initialized (Baud: 9600, RX: 16, TX: 17)");

    // Network Setup
    setupWiFi();
    setupWebServer();

    // MQTT Setup
    setupMqtt();
    
    // Setup complete animation
    for(int i=0; i<3; i++) {
        digitalWrite(LED_SYS_PIN, HIGH);
        digitalWrite(LED_MB_PIN, HIGH);
        delay(100);
        digitalWrite(LED_SYS_PIN, LOW);
        digitalWrite(LED_MB_PIN, LOW);
        delay(100);
    }
}

void loop() {
    // Non-blocking WiFi status checking and Multi-WiFi processing
    wl_status_t wifiStatus = WiFi.status();
    static uint32_t lastWifiAttempt = 0;
    if (wifiStatus == WL_CONNECTED) {
        if (!wifiConnected) {
            Serial.print("WiFi Connected. IP: ");
            Serial.println(WiFi.localIP());
            wifiConnected = true;
            triggerSysLedFlash(500);
        }
    } else {
        if (wifiConnected) {
            Serial.println("WiFi Disconnected!");
            wifiConnected = false;
        }
        // Run WiFiMulti with rate limiting (every 5 seconds) to avoid CPU starvation
        if (config.wifiCount > 0) {
            uint32_t now = millis();
            if (now - lastWifiAttempt >= 5000) {
                lastWifiAttempt = now;
                wifiMulti.run();
            }
        }
    }

    // Web Server client handling
    server.handleClient();

    // MQTT Client Loop & Reconnection
    if (wifiConnected && strlen(config.mqtt.server) > 0) {
        if (!mqttClient.connected()) {
            uint32_t now = millis();
            if (now - lastMqttAttemptTime > 5000) {
                lastMqttAttemptTime = now;
                reconnectMqtt();
            }
        } else {
            mqttClient.loop();
        }
    } else if (mqttClient.connected()) {
        mqttClient.disconnect();
        Serial.println("MQTT Broker server address empty, disconnected MQTT Client.");
    }

    // Modbus RTU Polling Loop
    processModbusPolls();

    // Check for incoming configuration commands via Serial Port
    checkSerialCommands();

    // Handle LED auto-off timings (non-blocking)
    if (ledSysOffTime > 0 && millis() >= ledSysOffTime) {
        digitalWrite(LED_SYS_PIN, LOW);
        ledSysOffTime = 0;
    }
    if (ledMbOffTime > 0 && millis() >= ledMbOffTime) {
        digitalWrite(LED_MB_PIN, LOW);
        ledMbOffTime = 0;
    }
}

// --- WiFi Setup ---
void setupWiFi() {
    Serial.println("Configuring Network Interfaces...");
    
    // 1. Enable AP + STA mode so Gateway is always accessible
    WiFi.mode(WIFI_AP_STA);
    
    // Start local Access Point (ModQ_AP, IP 192.168.4.1)
    WiFi.softAP("ModQ_AP");
    apActive = true;
    Serial.println("Access Point 'ModQ_AP' started. IP: 192.168.4.1");

    // 2. Configure WiFi Multi-AP list
    // Clear previous configurations in WiFiMulti if possible (WiFiMulti doesn't support clear, 
    // but re-creating it/adding is fine)
    wifiMulti = WiFiMulti();
    
    Serial.printf("Registered %d WiFi networks.\n", config.wifiCount);
    for (int i = 0; i < config.wifiCount; i++) {
        wifiMulti.addAP(config.wifis[i].ssid, config.wifis[i].password);
        Serial.printf(" - Added network SSID: %s\n", config.wifis[i].ssid);
    }
}

// --- Web Server Setup ---
void setupWebServer() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/config", HTTP_GET, handleGetConfig);
    server.on("/api/config", HTTP_POST, handlePostConfig);
    server.on("/api/data", HTTP_GET, handleGetData);
    server.on("/api/status", HTTP_GET, handleGetStatus);
    server.on("/api/wifi-scan", HTTP_GET, handleWifiScan);
    server.on("/api/restart", HTTP_POST, handleRestart);
    server.on("/api/reset", HTTP_POST, handleReset);
    
    server.begin();
    Serial.println("HTTP Web Server started on port 80.");
}

// --- MQTT Setup ---
void setupMqtt() {
    if (strlen(config.mqtt.server) > 0) {
        mqttClient.setServer(config.mqtt.server, config.mqtt.port);
        mqttClient.setCallback(handleMqttCallback);
        Serial.printf("MQTT Server configured: %s:%d\n", config.mqtt.server, config.mqtt.port);
    } else {
        Serial.println("MQTT Broker server address empty, MQTT disabled.");
    }
}

void reconnectMqtt() {
    Serial.print("Connecting to MQTT broker...");
    String clientId = config.mqtt.clientId;
    bool connected = false;
    
    if (strlen(config.mqtt.user) > 0) {
        connected = mqttClient.connect(clientId.c_str(), config.mqtt.user, config.mqtt.password);
    } else {
        connected = mqttClient.connect(clientId.c_str());
    }

    if (connected) {
        Serial.println("connected!");
        triggerSysLedFlash(300);
        // Subscribe to Cmd Topic
        if (strlen(config.mqtt.cmdTopic) > 0) {
            mqttClient.subscribe(config.mqtt.cmdTopic);
            Serial.printf("Subscribed to MQTT cmd topic: %s\n", config.mqtt.cmdTopic);
        }
    } else {
        Serial.printf("failed, rc=%d. Will retry in 5s.\n", mqttClient.state());
    }
}

// --- MQTT CMD Payload Callback ---
// Format: {"sid": 1, "addr": 10, "val": 123}
void handleMqttCallback(char* topic, byte* payload, unsigned int length) {
    triggerSysLedFlash(150);
    Serial.printf("MQTT Message received on [%s]\n", topic);

#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    DynamicJsonDocument doc(1024);
#endif
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error) {
        Serial.printf("MQTT parsing failed: %s\n", error.c_str());
        return;
    }

    if (doc.containsKey("sid") && doc.containsKey("addr") && doc.containsKey("val")) {
        uint8_t sid = doc["sid"];
        uint16_t addr = doc["addr"];
        uint16_t val = doc["val"];

        Serial.printf("MQTT Write CMD: Slave %d, Reg %d -> Value %d\n", sid, addr, val);
        
        // Find if we have this device configured to determine its baudrate
        uint32_t devBaud = 9600;
        for (int i = 0; i < MAX_DEVICES; i++) {
            if (config.devices[i].active && config.devices[i].slaveId == sid) {
                devBaud = config.devices[i].baudRate;
                break;
            }
        }

        // Reinitialize baudrate if needed
        if (devBaud != currentBaudRate) {
            Serial2.end();
            Serial2.begin(devBaud, SERIAL_8N1, 16, 17);
            currentBaudRate = devBaud;
        }

        triggerMbLedFlash(100);
        node.begin(sid, Serial2);
        uint8_t result = node.writeSingleRegister(addr, val);
        
        // Publish response back to MQTT if possible
        if (mqttClient.connected()) {
            char responseTopic[256];
            sprintf(responseTopic, "%s/response", config.mqtt.dataTopic);
#if ARDUINOJSON_VERSION_MAJOR >= 7
            JsonDocument respDoc;
#else
            DynamicJsonDocument respDoc(512);
#endif
            respDoc["sid"] = sid;
            respDoc["addr"] = addr;
            respDoc["val"] = val;
            respDoc["success"] = (result == node.ku8MBSuccess);
            respDoc["code"] = result;
            
            String responseStr;
            serializeJson(respDoc, responseStr);
            mqttClient.publish(responseTopic, responseStr.c_str());
        }

        Serial.printf("Modbus Write Result: %s (Code: %d)\n", (result == node.ku8MBSuccess) ? "SUCCESS" : "FAILED", result);
    }
}

// --- Modbus RTU Polling Loop ---
void processModbusPolls() {
    uint32_t now = millis();

    for (int i = 0; i < MAX_DEVICES; i++) {
        if (!config.devices[i].active) continue;

        if (now - lastPollTime[i] >= config.devices[i].readInterval) {
            lastPollTime[i] = now;
            
            DeviceConfig& dev = config.devices[i];
            DeviceStatus& status = deviceStates[i];

            Serial.printf("Polling Modbus Device: %s (ID: %d)\n", dev.name, dev.slaveId);
            
            // Adjust baud rate if required
            if (dev.baudRate != currentBaudRate) {
                Serial2.end();
                Serial2.begin(dev.baudRate, SERIAL_8N1, 16, 17);
                currentBaudRate = dev.baudRate;
                Serial.printf("Switched Modbus Serial2 Baudrate to %d bps\n", dev.baudRate);
            }

            node.begin(dev.slaveId, Serial2);
            status.lastAttemptTime = now;
            
            bool deviceCommunicationOk = true;
            uint8_t devLastError = node.ku8MBSuccess;

            // Poll configured registers
            for (int j = 0; j < dev.regCount; j++) {
                RegConfig& reg = dev.registers[j];
                RegValue& regVal = status.regValues[j];

                // Flash LED2 indicating Modbus RTU operation
                triggerMbLedFlash(80);

                uint8_t result = node.ku8MBSlaveDeviceFailure;
                uint8_t qtyToRead = 1;

                // Adjust qty for 32-bit datatypes (UINT32, INT32, FLOAT)
                if (reg.dataType >= TYPE_UINT32) {
                    qtyToRead = 2;
                }

                // Query based on function code
                if (reg.funcCode == 4) {
                    result = node.readInputRegisters(reg.address, qtyToRead);
                } else {
                    result = node.readHoldingRegisters(reg.address, qtyToRead);
                }

                if (result == node.ku8MBSuccess) {
                    regVal.valid = true;
                    regVal.updateTime = now;

                    if (qtyToRead == 1) {
                        uint16_t w0 = node.getResponseBuffer(0);
                        regVal.rawVal = w0;
                        if (reg.dataType == TYPE_INT16) {
                            regVal.floatVal = ((int16_t)w0) * reg.multiplier;
                        } else {
                            regVal.floatVal = w0 * reg.multiplier;
                        }
                    } else {
                        uint16_t w0 = node.getResponseBuffer(0); // High word
                        uint16_t w1 = node.getResponseBuffer(1); // Low word
                        uint32_t combined = ((uint32_t)w0 << 16) | w1;
                        regVal.rawVal = combined;

                        if (reg.dataType == TYPE_UINT32) {
                            regVal.floatVal = combined * reg.multiplier;
                        } else if (reg.dataType == TYPE_INT32) {
                            regVal.floatVal = ((int32_t)combined) * reg.multiplier;
                        } else if (reg.dataType == TYPE_FLOAT) {
                            float f;
                            memcpy(&f, &combined, 4);
                            regVal.floatVal = f * reg.multiplier;
                        }
                    }
                    
                    Serial.printf(" - Reg %d (%s): %.2f %s\n", reg.address, reg.name, regVal.floatVal, reg.unit);
                } else {
                    regVal.valid = false;
                    deviceCommunicationOk = false;
                    devLastError = result;
                    Serial.printf(" - Reg %d (%s) Read Failed. Code: %d\n", reg.address, reg.name, result);
                    
                    if (result == node.ku8MBResponseTimedOut) {
                        Serial.println("   Timeout occurred. Skipping remaining registers for this device.");
                        for (int k = j + 1; k < dev.regCount; k++) {
                            status.regValues[k].valid = false;
                        }
                        break;
                    }
                }
            }

            status.online = deviceCommunicationOk;
            status.lastError = devLastError;
            
            if (deviceCommunicationOk) {
                status.lastSuccessTime = now;
            }

            // Publish this device's updated values to MQTT
            publishDeviceData(i);
        }
    }
}

void publishDeviceData(int devIdx) {
    if (!mqttClient.connected() || strlen(config.mqtt.dataTopic) == 0) return;

    DeviceConfig& dev = config.devices[devIdx];
    DeviceStatus& status = deviceStates[devIdx];

#if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
#else
    DynamicJsonDocument doc(2048);
#endif
    
    doc["name"] = dev.name;
    doc["sid"] = dev.slaveId;
    doc["online"] = status.online;
    doc["error"] = status.lastError;
    
    JsonObject regsObj = doc.createNestedObject("regs");
    for (int j = 0; j < dev.regCount; j++) {
        if (status.regValues[j].valid) {
            regsObj[dev.registers[j].name] = status.regValues[j].floatVal;
        }
    }

    String payload;
    serializeJson(doc, payload);

    // Topic format: canopus/device/data/<slave_id>
    char topic[256];
    sprintf(topic, "%s/%d", config.mqtt.dataTopic, dev.slaveId);

    mqttClient.publish(topic, payload.c_str());
    Serial.printf("MQTT Published device status to [%s]\n", topic);
}

// --- Status LEDs Flashing Control ---
void triggerSysLedFlash(uint32_t durationMs) {
    digitalWrite(LED_SYS_PIN, HIGH);
    ledSysOffTime = millis() + durationMs;
}

void triggerMbLedFlash(uint32_t durationMs) {
    digitalWrite(LED_MB_PIN, HIGH);
    ledMbOffTime = millis() + durationMs;
}

// --- Serial CLI Configuration Interface ---
void checkSerialCommands() {
    if (Serial.available()) {
        static String cmdBuffer = "";
        while (Serial.available()) {
            char c = Serial.read();
            if (c == '\n') {
                cmdBuffer.trim();
                if (cmdBuffer.length() > 0) {
                    executeSerialCommand(cmdBuffer);
                }
                cmdBuffer = "";
            } else if (c != '\r') {
                if (cmdBuffer.length() < 256) {
                    cmdBuffer += c;
                } else {
                    cmdBuffer = "";
                    Serial.println("ERROR: Serial CLI command buffer overflow (max 256 chars)");
                }
            }
        }
    }
}

void executeSerialCommand(String cmd) {
    Serial.print("\nReceived command: ");
    Serial.println(cmd);

    if (cmd.equalsIgnoreCase("help")) {
        Serial.println("--- Available Serial Commands ---");
        Serial.println("help                               - Show this menu");
        Serial.println("status                             - Show current system status");
        Serial.println("config                             - Print current JSON configuration");
        Serial.println("config set <json>                  - Set configuration from JSON string");
        Serial.println("wifi add <ssid> <pass>             - Add WiFi network (Max 3)");
        Serial.println("wifi clear                         - Clear all saved WiFi networks");
        Serial.println("mqtt set <server> <port> [u] [p]   - Configure MQTT Broker server, port, optional user & pass");
        Serial.println("restart                            - Restart the gateway");
        Serial.println("reset                              - Factory reset and restart");
        Serial.println("---------------------------------");
    } 
    else if (cmd.equalsIgnoreCase("status")) {
        Serial.println("--- System Status ---");
        Serial.printf("WiFi Connected: %s\n", (WiFi.status() == WL_CONNECTED) ? "YES" : "NO");
        Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("IP Address: %s\n", (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString().c_str() : "0.0.0.0");
        Serial.printf("AP Active: %s\n", apActive ? "YES" : "NO");
        Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
        Serial.printf("MQTT Connected: %s\n", mqttClient.connected() ? "YES" : "NO");
        Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
        Serial.printf("Free Memory: %d bytes\n", ESP.getFreeHeap());
    } 
    else if (cmd.equalsIgnoreCase("config")) {
        Serial.println("--- Current Configuration ---");
        Serial.println(Settings::getAsJson());
    } 
    else if (cmd.startsWith("config set ")) {
        String json = cmd.substring(11);
        json.trim();
        if (Settings::saveFromJson(json)) {
            Serial.println("SUCCESS: Configuration loaded and saved.");
            setupWiFi();
            setupMqtt();
        } else {
            Serial.println("ERROR: Invalid JSON configuration string.");
        }
    } 
    else if (cmd.startsWith("wifi add ")) {
        String args = cmd.substring(9);
        args.trim();
        int spaceIdx = args.indexOf(' ');
        String ssid = "";
        String pass = "";
        if (spaceIdx == -1) {
            ssid = args;
        } else {
            ssid = args.substring(0, spaceIdx);
            pass = args.substring(spaceIdx + 1);
        }
        ssid.trim();
        pass.trim();
        
        if (ssid.length() == 0) {
            Serial.println("ERROR: SSID cannot be empty.");
            return;
        }
        if (config.wifiCount >= MAX_WIFI_NETS) {
            Serial.println("ERROR: Maximum WiFi networks (3) reached.");
            return;
        }
        int idx = config.wifiCount++;
        strncpy(config.wifis[idx].ssid, ssid.c_str(), 32);
        config.wifis[idx].ssid[32] = '\0';
        strncpy(config.wifis[idx].password, pass.c_str(), 64);
        config.wifis[idx].password[64] = '\0';
        
        Settings::save();
        Serial.printf("SUCCESS: Added WiFi SSID: %s\n", ssid.c_str());
        setupWiFi();
    } 
    else if (cmd.equalsIgnoreCase("wifi clear")) {
        config.wifiCount = 0;
        memset(config.wifis, 0, sizeof(config.wifis));
        Settings::save();
        Serial.println("SUCCESS: Cleared all saved WiFi networks.");
        setupWiFi();
    } 
    else if (cmd.startsWith("mqtt set ")) {
        String args = cmd.substring(9);
        args.trim();
        int space1 = args.indexOf(' ');
        if (space1 == -1) {
            Serial.println("ERROR: Invalid format. Use: mqtt set <server> <port> [user] [pass]");
            return;
        }
        String serverStr = args.substring(0, space1);
        String remainder = args.substring(space1 + 1);
        remainder.trim();
        
        int space2 = remainder.indexOf(' ');
        String portStr = "";
        String userStr = "";
        String passStr = "";
        if (space2 == -1) {
            portStr = remainder;
        } else {
            portStr = remainder.substring(0, space2);
            String remainder2 = remainder.substring(space2 + 1);
            remainder2.trim();
            int space3 = remainder2.indexOf(' ');
            if (space3 == -1) {
                userStr = remainder2;
            } else {
                userStr = remainder2.substring(0, space3);
                passStr = remainder2.substring(space3 + 1);
            }
        }
        serverStr.trim();
        portStr.trim();
        userStr.trim();
        passStr.trim();

        int portVal = portStr.toInt();
        if (portVal <= 0) portVal = 1883;

        strncpy(config.mqtt.server, serverStr.c_str(), 64);
        config.mqtt.server[64] = '\0';
        config.mqtt.port = portVal;
        strncpy(config.mqtt.user, userStr.c_str(), 32);
        config.mqtt.user[32] = '\0';
        strncpy(config.mqtt.password, passStr.c_str(), 64);
        config.mqtt.password[64] = '\0';

        Settings::save();
        Serial.printf("SUCCESS: Configured MQTT Broker: %s:%d, User: %s\n", config.mqtt.server, config.mqtt.port, config.mqtt.user);
        setupMqtt();
    } 
    else if (cmd.equalsIgnoreCase("restart")) {
        Serial.println("SUCCESS: Restarting ESP32...");
        delay(500);
        ESP.restart();
    } 
    else if (cmd.equalsIgnoreCase("reset")) {
        Serial.println("SUCCESS: Performing factory reset...");
        Settings::reset();
        Settings::save();
        delay(500);
        ESP.restart();
    } 
    else {
        Serial.println("ERROR: Unknown command. Type 'help' for the command list.");
    }
}
