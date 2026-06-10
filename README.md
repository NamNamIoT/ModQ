# ESP32 Canopus Modbus-to-MQTT Gateway

An industrial-grade, ESP32-based Modbus RTU (RS485) to MQTT & HTTP web gateway. The Canopus gateway enables polling up to 8 Modbus slave devices, mapping register data types dynamically, and publishing them to MQTT brokers or exposing them via a modern WebUI and HTTP API.

---

## 🚀 Key Features

- **Multi-Client Web UI**: Exposes an interactive HTML5 Web dashboard on port 80 to view live telemetry, trigger manual Modbus writes, scan WiFi networks, and upload settings.
- **Dynamic Register Mapping**: Configure up to 8 devices and 16 registers per device, supporting diverse data types (`UINT16`, `INT16`, `UINT32`, `INT32`, `FLOAT`) and custom scaling multipliers.
- **Failover WiFi & AP**: Configures up to 3 WiFi networks with auto-failover, while simultaneously hosting a fallback Access Point (`Canopus_AP`) for localized configuration.
- **Bi-Directional MQTT Broker Integration**:
  - Publishes polled data JSON packets to `canopus/device/data/<slave_id>`.
  - Subscribes to write command packets on `canopus/device/cmd` and publishes results back to `canopus/device/data/response`.
- **Serial CLI Commands**: A comprehensive shell interface over the USB port (115200 bps) for debugging and quick configuration.
- **Automated Unit Testing**: Complete desktop unit testing suite (20 tests) leveraging C++ mock drivers for NVS Preferences, HardwareSerial, and Arduino base libraries.

---

## 🔌 Hardware Specifications (v1.0)

| **Component** | **Specifications & Details** | **System Pinout / Notes** |
| :--- | :--- | :--- |
| **MCU** | ESP32 WROOM 32D (Dual-Core, 240MHz, 4MB Flash) | Core Gateway controller |
| **Power Input** | Step-down MC34063A (5–35 VDC) or USB Type-C | dual input auto-switching |
| **RS485 Transceiver** | SP485EE with automatic DE/RE flow control | `Serial2` (TX2: IO17, RX2: IO16) |
| **Ethernet Controller** | LAN8720A (RJ45 with integrated magnetics) | RMII Mode, EN Pin: IO14 |
| **I2C Interface** | Auxiliary sensors & OLED display | SDA: IO33, SCL: IO4 |
| **Status Indicators** | Blue LED (System Status) & Yellow LED (Modbus Activity) | Yellow: IO2, Blue: IO15 (System: IO25, Modbus: IO27) |
| **Control Button** | User configuration trigger / Factory reset | IO36 |
| **Enclosure** | Industrial DIN-Rail Mountable Plastic Case | DIN Rail standard |

---

## 🛠️ Software Setup & Installation

### Step 1: Install IDE and Drivers
1. Download and install [Arduino IDE](https://www.arduino.cc/en/software).
2. Install the **CH340 USB-to-UART Driver** to interface with the ESP32 via USB-C.

### Step 2: Install ESP32 Board Support
1. In Arduino IDE, go to **File > Preferences**.
2. Enter the following URL in **Additional Board Manager URLs**:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Go to **Tools > Board > Boards Manager**, search for **esp32** and install the latest version.
4. Select your target board: **ESP32 Dev Module**.

### Step 3: Library Dependencies
Install the following libraries via the Arduino Library Manager:
- **PubSubClient** (by Nick O'Leary) - MQTT Client
- **ArduinoJson** (v6.x or v7.x) - JSON Serialization/Deserialization

### Step 4: Flash the Firmware
1. Open [Canopus_ModbusApp.ino](file:///c:/Users/ADMIN/Documents/MEGA\Github/ESP32_CANOPUS/Canopus_ModbusApp/Canopus_ModbusApp.ino) in the Arduino IDE.
2. Connect the Canopus board using a USB-C cable.
3. Select the correct COM port and hit **Upload**.

---

## 🌐 Configuration Interfaces

### 1. Web Dashboard (HTTP)
When the board boots up, it launches a local WiFi Access Point named `Canopus_AP` (IP: `192.168.4.1`). 
1. Connect your PC/phone to the `Canopus_AP` network.
2. Open your web browser and navigate to `http://192.168.4.1`.
3. Use the dashboard to configure WiFi networks, MQTT credentials, and Modbus register mappings.

### 2. Serial CLI Interface (USB)
Connect via serial monitor (Baud Rate: `115200`, Line Ending: `Newline`) to issue the following commands:

- `help` - Show available console commands.
- `status` - Dump system status (IP, WiFi status, AP active, MQTT connection status, uptime, free heap).
- `config` - Prints the current configuration as a JSON string.
- `config set <json_string>` - Loads and saves configuration settings from a JSON payload.
- `wifi add <ssid> <password>` - Save a new WiFi network (up to 3).
- `wifi clear` - Wipe all saved WiFi networks.
- `mqtt set <server> <port> [user] [pass]` - Configure the MQTT broker connection.
- `restart` - Reboot the ESP32.
- `reset` - Perform factory reset and reboot.

---

## 📡 MQTT Payload Formats

### 1. Polled Modbus Data Publish
The gateway regularly polls registers and publishes JSON data to `canopus/device/data/<slave_id>`:
```json
{
  "name": "Demo Temp Sensor",
  "sid": 1,
  "online": true,
  "error": 0,
  "regs": {
    "Temperature": 28.5,
    "Humidity": 62.1
  }
}
```

### 2. Remote Write Commands (Subscribe)
Publish a write command to `canopus/device/cmd`:
```json
{
  "sid": 1,
  "addr": 10,
  "val": 123
}
```
The gateway will write `123` to holding register `10` on Modbus slave `1`, and reply on `canopus/device/data/response`:
```json
{
  "sid": 1,
  "addr": 10,
  "val": 123,
  "success": true,
  "code": 0
}
```

---

## 🧪 Developer Unit Testing Guide

The project includes an advanced, off-board desktop unit testing environment that mocks the ESP32 NVS, HardwareSerial, and Arduino libraries, allowing you to compile and run tests directly on your Windows PC using `g++` (no physical board required).

### 1. Install GCC Compiler (Windows)
If you don't have a C++ compiler in your PATH, you can quickly install it via Windows Package Manager (`winget`):
```powershell
winget install --id BrechtSanders.WinLibs.POSIX.UCRT --scope user --silent --accept-source-agreements --accept-package-agreements
```

### 2. Run the Unit Tests
Open PowerShell, navigate to the `Canopus_ModbusApp` directory, and execute the test runner:
```powershell
powershell -ExecutionPolicy Bypass -File tests/run_tests.ps1
```

The script will automatically download the correct single-header `ArduinoJson.h` if missing, locate the installed compiler, compile, and execute the **20 unit tests** checking:
- **CRC-16 Modbus** validation algorithm.
- **Modbus Master** transactions (successful holding/input register reads, writes, timeouts, exception codes, bad CRC error handling, byte count validation).
- **NVS Settings** serialization, deserialization, empty configuration handling, and array truncation protection.
