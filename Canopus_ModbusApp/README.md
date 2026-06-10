# ModQ: ESP32 Modbus Gateway Application

The **Canopus_ModbusApp** is a comprehensive IoT gateway solution built for the ESP32 ModQ development board. It enables data collection from multiple Modbus RTU devices (via RS485), visualizes it through a responsive local Web UI (with multilingual support), and publishes telemetry data to an MQTT Broker.

---

## 🛠️ Key Features

- **Multi-WiFi Connection Manager**: Save up to 3 WiFi network configurations. The gateway automatically reconnects and switches to another available network if the current one drops.
- **Fallback Access Point**: Simultaneously hosts an open WiFi Access Point named `ModQ_AP` (default IP `192.168.4.1`) to ensure you never get locked out and can always configure settings locally.
- **Multilingual Web UI**: Supports 4 languages: **English (Default)**, **Vietnamese**, **Spanish**, and **Portuguese**.
- **Live Visual Dashboard**: Modbus values are refreshed every 1.5s with smooth flash animations on value changes. The UI displays descriptive Modbus RTU error alerts if queries fail.
- **Bi-directional MQTT**:
  - *Publish*: Bundles data from active devices into standard JSON format and sends it to the broker.
  - *Subscribe*: Listens for control commands to write values back to Modbus slave registers.
- **Hardware Status Indicators**:
  - `LED 1 (GPIO 25)`: Flashes during Web UI activity (HTTP REST API processing).
  - `LED 2 (GPIO 27)`: Flashes rapidly during Modbus RTU query transactions on the RS485 bus.

---

## ⚙️ Hardware Pinout Configuration

The gateway application is optimized for the **ESP32 ModQ** board using the following pin mapping:

| Interface | ESP32 GPIO | Description |
| :--- | :--- | :--- |
| **RS485 RX** | `GPIO 16` | Routed to Hardware Serial RX2 of ESP32 |
| **RS485 TX** | `GPIO 17` | Routed to Hardware Serial TX2 of ESP32 |
| **Flow Control** | *(Auto)* | IC SP485EE automatically toggles Transmit/Receive (DE/RE) |
| **LED 1 (SYS)** | `GPIO 25` | System, WiFi, and HTTP API activity indicator |
| **LED 2 (MODBUS)**| `GPIO 27` | RS485 Modbus RTU transaction indicator |

---

## 📚 Software Dependencies

Before compiling in the Arduino IDE, make sure to install the following libraries via the Library Manager:

1. **ArduinoJson** (v6.x or v7.x) - For processing configuration and API payloads.
2. **PubSubClient** (by Nick O'Leary) - For connecting to the MQTT Broker.

---

## 🚀 Installation & Compilation

1. Download or clone the `Canopus_ModbusApp` directory.
2. Open the main sketch `Canopus_ModbusApp.ino` in **Arduino IDE**.
3. Connect the **ESP32 ModQ** board to your PC via a USB Type-C cable.
4. Select the correct COM Port and select **ESP32 Dev Module** (or your specific ESP32 board) under Tools.
5. Click **Upload** to compile and flash the firmware onto the board.

---

## 🔌 Initial Configuration via COM Port (USB Type-C)

When the board is brand new or lacks WiFi configurations, you can plug in the USB Type-C cable to your PC and open the **Serial Monitor** at **115200 baud** to configure the device using simple text commands.

Available Serial CLI commands:

- **`help`**: Prints the list of available commands.
- **`status`**: Shows current status (connected WiFi, IP, MQTT broker connection status, uptime, free heap memory).
- **`config`**: Prints the current configuration in JSON format.
- **`config set <json_string>`**: Allows pasting a complete JSON string to overwrite the configuration directly.
- **`wifi add <ssid> <password>`**: Adds a new WiFi network credentials (supports up to 3 saved networks).
- **`wifi clear`**: Clears all saved WiFi network credentials from the NVS memory.
- **`mqtt set <server> <port> [user] [pass]`**: Sets the MQTT Broker host address, port, username, and password (optional).
- **`restart`**: Reboots the ESP32 Gateway.
- **`reset`**: Restores the device to factory defaults and restarts.

---

## 💻 Web UI Usage Guide

Once the device is powered and running:

1. Connect your PC or mobile device to the open WiFi network named **`ModQ_AP`** hosted by the gateway.
2. Open your browser and navigate to: **`http://192.168.4.1`**.
3. Select your preferred language (English, Vietnamese, Spanish, or Portuguese) from the sidebar.
4. **WiFi Setup**: Go to the **WiFi Networks** tab, scan for surrounding networks, select your office/home network, and enter the password. The gateway will connect and receive an IP address from your router.
5. **Modbus Setup**:
   - Go to the **Modbus Devices** tab.
   - Click **Add Device** on a slot, fill in the device name, Modbus Slave ID (1 - 247), Baudrate, and read interval.
   - Click **+ Add Register** to configure registers: name, register address, function code (Holding or Input register), datatype (UINT16, INT16, UINT32, INT32, FLOAT), multiplier, and unit.
6. **MQTT Setup**: Go to the **MQTT Config** tab, fill in the broker address, port, topics, and click save.

---

## 📡 MQTT API Specification

### 1. Telemetry Publish (Data)
Whenever a Modbus device is successfully polled, the gateway publishes the data as a JSON payload to the topic:
`modq/device/data/<slave_id>` (or your custom data topic).

**JSON Payload Format:**
```json
{
  "name": "Cabinet Sensor 1",
  "sid": 1,
  "online": true,
  "error": 0,
  "regs": {
    "Temperature": 28.5,
    "Humidity": 62.4
  }
}
```

### 2. Control Subscribe (Commands)
The gateway subscribes to the topic `modq/device/cmd` (or your custom command topic) to receive write commands.

To write a value to a Modbus slave register, publish a JSON payload to that topic:
```json
{
  "sid": 1,
  "addr": 102,
  "val": 450
}
```
*Note:* This command will write the value `450` to register address `102` (using Function Code 06 - Write Single Register) on Slave ID `1`.

The gateway will execute the write command and publish the result to `modq/device/data/response`:
```json
{
  "sid": 1,
  "addr": 102,
  "val": 450,
  "success": true,
  "code": 0
}
```

---

## 🌐 HTTP REST API Endpoints

Integrate the gateway with SCADA, dashboard softwares, or custom systems using HTTP REST APIs:

| Method | Endpoint | Response Type | Description |
| :---: | :--- | :---: | :--- |
| **GET** | `/` | HTML | Serves the main Single-Page Web UI dashboard |
| **GET** | `/api/config` | JSON | Retrieves current WiFi, MQTT, and device configurations |
| **POST**| `/api/config` | JSON | Saves and applies new configurations |
| **GET** | `/api/data` | JSON | Retrieves real-time polled Modbus register values |
| **GET** | `/api/status` | JSON | Retrieves connection statuses, IP address, RAM heap, and uptime |
| **GET** | `/api/wifi-scan` | JSON | Triggers and returns surrounding WiFi networks scan results |
| **POST**| `/api/restart` | JSON | Reboots the ESP32 Gateway |
| **POST**| `/api/reset` | JSON | Restores gateway configuration to defaults and restarts |

---

## 🔧 Performance, Stability & Safety Enhancements

The codebase includes several modifications specifically designed for industrial deployment:
*   **Modbus Boundary Checks**: Added strict bounds checking on response frame parsing to prevent out-of-bounds stack writes and heap corruption if slave devices return corrupted payloads.
*   **Non-Blocking Polling Abort**: If a Modbus slave times out on a register query, the system immediately marks the device offline and skips remaining registers for that device. This prevents the typical 3-second thread block in Arduino loops when a device is powered off, keeping the WebUI responsive and maintaining MQTT broker connectivity.
*   **FreeRTOS CPU Yielding**: Added scheduling delays within tight serial loops to yield CPU execution to FreeRTOS, preventing watchdog resets and ensuring the network processor stack is not starved.
*   **WiFi Rate Limiting**: WiFi connection attempts are throttled to run at most once every 5 seconds, avoiding scanning overhead that degrades main thread execution speed.

---

## 🧪 Desktop Unit Testing Suite

The repository includes a desktop-based unit testing suite that mocks the ESP32 and Arduino environments to verify logic without physical hardware:
*   **Mocks**: Under [tests/mocks/](file:///c:/Users/ADMIN/Documents/MEGA/Github/ESP32_CANOPUS/Canopus_ModbusApp/tests/mocks), mocks are provided for `Arduino.h`, `Preferences.h`, and `ArduinoJson.h`.
*   **Test Cases**: Coverage includes Modbus CRC16 lookup validity, packet composition accuracy, register buffer decoding, exception response propagation, communication timeouts, and JSON serialization.

### Running Unit Tests
To compile and execute the test harness locally:
1.  Ensure you have `g++` (GCC) installed and configured in your environment PATH.
2.  Open **PowerShell** in the `Canopus_ModbusApp` directory and run:
    ```powershell
    powershell -ExecutionPolicy Bypass -File .\tests\run_tests.ps1
    ```
3.  The runner will verify directories, download dependencies, compile files, and execute the test suite.

---

## 📐 System Design & Diagrams

For full structural details, state machines, and sequence diagrams of the boot-up and polling flows, refer to the [System Design Document (SYSTEM_DESIGN.md)](file:///c:/Users/ADMIN/Documents/MEGA/Github/ESP32_CANOPUS/Canopus_ModbusApp/SYSTEM_DESIGN.md).

