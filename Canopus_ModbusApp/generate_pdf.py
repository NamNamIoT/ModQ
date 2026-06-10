import os
from fpdf import FPDF

class PDFManual(FPDF):
    def header(self):
        # Upper header
        self.set_font('Arial', 'I', 8)
        self.set_text_color(100, 100, 100)
        self.cell(0, 10, 'ESP32 ModQ Modbus Gateway - User Manual', 0, 0, 'R')
        self.ln(10)
        # Accent line
        self.set_draw_color(14, 165, 233) # Sky-500 (#0ea5e9)
        self.set_line_width(0.8)
        self.line(20, 20, 190, 20)
        self.ln(5)

    def footer(self):
        # Position at 1.5 cm from bottom
        self.set_y(-15)
        self.set_font('Arial', 'I', 8)
        self.set_text_color(100, 100, 100)
        # Accent line
        self.set_draw_color(226, 232, 240) # Slate-200
        self.set_line_width(0.4)
        self.line(20, 280, 190, 280)
        # Page number
        self.cell(0, 10, 'Page ' + str(self.page_no()) + ' / {nb}', 0, 0, 'C')

    def add_title(self, title_text, subtitle_text):
        self.add_page()
        # Spacer
        self.ln(10)
        # Main Title
        self.set_font('Arial', 'B', 24)
        self.set_text_color(15, 23, 42) # Slate-900
        self.multi_cell(0, 15, title_text, 0, 'C')
        self.ln(5)
        # Subtitle
        self.set_font('Arial', '', 12)
        self.set_text_color(100, 116, 139) # Slate-500
        self.multi_cell(0, 8, subtitle_text, 0, 'C')
        self.ln(15)
        # Large blue accent box
        self.set_fill_color(224, 242, 254) # Sky-100
        self.set_draw_color(14, 165, 233) # Sky-500
        self.set_line_width(0.5)
        self.rect(20, self.get_y(), 170, 45, 'DF')
        
        self.set_y(self.get_y() + 5)
        self.set_x(25)
        self.set_font('Arial', 'B', 10)
        self.set_text_color(2, 132, 199) # Sky-600
        self.cell(0, 6, 'Device Metadata:', 0, 1)
        
        self.set_x(25)
        self.set_font('Arial', '', 9)
        self.set_text_color(51, 65, 85) # Slate-700
        self.cell(0, 5, 'Hardware: ESP32 ModQ Development Board', 0, 1)
        self.set_x(25)
        self.cell(0, 5, 'Firmware Version: v1.0.0 (Multilingual Edition)', 0, 1)
        self.set_x(25)
        self.cell(0, 5, 'Protocols: Modbus RTU (RS485) & MQTT (over WiFi)', 0, 1)
        self.set_x(25)
        self.cell(0, 5, 'Web UI IP Default: 192.168.4.1 (AP Mode) / Port 80', 0, 1)
        self.add_page()

    def heading1(self, text):
        self.ln(8)
        self.set_font('Arial', 'B', 14)
        self.set_text_color(2, 132, 199) # Sky-600
        self.cell(0, 10, text, 0, 1, 'L')
        self.ln(2)

    def heading2(self, text):
        self.ln(4)
        self.set_font('Arial', 'B', 11)
        self.set_text_color(15, 23, 42) # Slate-900
        self.cell(0, 8, text, 0, 1, 'L')
        self.ln(1)

    def body_text(self, text):
        self.set_font('Arial', '', 9.5)
        self.set_text_color(51, 65, 85) # Slate-700
        self.multi_cell(0, 5.5, text, 0, 'L')
        self.ln(3)

    def bullet_point(self, title, desc):
        self.set_x(25)
        self.set_font('Arial', 'B', 9.5)
        self.set_text_color(15, 23, 42)
        
        full_title = ' - ' + title + ': '
        title_width = self.get_string_width(full_title)
        self.cell(title_width, 5.5, full_title, 0, 0, 'L')
        
        tab_stop = 65
        current_x = self.get_x()
        if current_x > tab_stop:
            self.ln(5.5)
            self.set_x(tab_stop)
        else:
            self.set_x(tab_stop)
            
        self.set_font('Arial', '', 9.5)
        self.set_text_color(51, 65, 85)
        
        old_margin = self.l_margin
        self.set_left_margin(tab_stop)
        self.multi_cell(0, 5.5, desc, 0, 'L')
        self.set_left_margin(old_margin)
        self.ln(1)

    def code_block(self, code_lines):
        self.set_fill_color(248, 250, 252) # Slate-50
        self.set_draw_color(226, 232, 240) # Slate-200
        self.set_line_width(0.2)
        
        # Calculate height needed
        line_height = 4.5
        height = len(code_lines) * line_height + 6
        
        self.rect(20, self.get_y(), 170, height, 'DF')
        self.set_y(self.get_y() + 3)
        self.set_font('Courier', '', 8.5)
        self.set_text_color(15, 23, 42)
        
        for line in code_lines:
            self.set_x(24)
            self.cell(0, line_height, line, 0, 1)
        self.ln(5)

    def draw_table(self, headers, rows, col_widths):
        self.set_font('Arial', 'B', 9)
        self.set_fill_color(2, 132, 199) # Sky-600
        self.set_text_color(255, 255, 255)
        self.set_draw_color(226, 232, 240)
        self.set_line_width(0.3)
        
        # Draw headers
        for i, header in enumerate(headers):
            self.cell(col_widths[i], 8, header, 1, 0, 'C', True)
        self.ln(8)
        
        self.set_font('Arial', '', 8.5)
        self.set_text_color(51, 65, 85)
        
        alternate = False
        for row in rows:
            if alternate:
                self.set_fill_color(240, 249, 255) # Light sky blue row tint
            else:
                self.set_fill_color(255, 255, 255)
            
            # Find max height for cells in row
            # For simplicity, since these are short strings, we use standard cells
            for i, val in enumerate(row):
                self.cell(col_widths[i], 7, val, 1, 0, 'L', True)
            self.ln(7)
            alternate = not alternate
        self.ln(4)

# Create PDF Manual
pdf = PDFManual()
pdf.alias_nb_pages()

# Cover/Title Page
pdf.add_title(
    'ESP32 MODQ MODBUS GATEWAY',
    'Industrial RS485 Modbus RTU to Web UI and MQTT Broker Gateway\nUser & Technical Reference Manual'
)

# Section 1: Introduction
pdf.heading1('1. Introduction & Specifications')
pdf.body_text(
    'The ESP32 ModQ Modbus Gateway is an industrial-grade, versatile IoT gateway designed '
    'to aggregate telemetry from multiple Modbus RTU slave devices over a half-duplex RS485 bus, '
    'provide local data visualization via an offline-capable Web dashboard, and publish real-time data '
    'to MQTT brokers.'
)
pdf.body_text('Key system parameters and specifications:')
pdf.bullet_point('Multi-WiFi Connection', 'Saves up to 3 WiFi network credentials and automatically transitions between them if connection is lost.')
pdf.bullet_point('Fallback Access Point', 'Starts a local, open WiFi network "ModQ_AP" at IP 192.168.4.1 so the dashboard is always accessible.')
pdf.bullet_point('Multilingual Interface', 'Saves user language choice and localizes all elements into English (default), Vietnamese, Spanish, or Portuguese.')
pdf.bullet_point('Bi-directional MQTT', 'Supports publishing JSON telemetry per device and subscribing to remote command payloads to write registers.')
pdf.bullet_point('Visual LEDs Feedback', 'LED1 flashes on WiFi/Web activity; LED2 flashes on Modbus transactions.')

# Section 2: Hardware
pdf.heading1('2. Hardware Interface & Pinout')
pdf.body_text(
    'The gateway utilizes the hardware peripherals on the ESP32 ModQ board. Specifically, it employs the '
    'second hardware UART port (UART2) connected to an onboard SP485EE RS485 transceiver. Transmit/Receive flow '
    'direction control (DE/RE) is managed automatically by the hardware circuit.'
)

headers = ['Component / Signal', 'ESP32 GPIO Pin', 'Description']
rows = [
    ['RS485 RX', 'GPIO 16 (RX2)', 'Receives data packets from the Modbus RS485 bus.'],
    ['RS485 TX', 'GPIO 17 (TX2)', 'Transmits query packets to the Modbus RS485 bus.'],
    ['DE / RE Flow Control', 'Hardware managed', 'SP485EE chip auto-switches TX/RX states.'],
    ['LED 1 (SYS Status)', 'GPIO 25', 'Flashes during HTTP Web activity and network status updates.'],
    ['LED 2 (MODBUS Query)', 'GPIO 27', 'Flashes briefly during Modbus RTU read/write transactions.']
]
pdf.draw_table(headers, rows, [45, 30, 95])

# Section 3: Installation
pdf.add_page()
pdf.heading1('3. Installation & Compilation')
pdf.body_text(
    'The firmware is written as an Arduino sketch. To compile and upload it onto the ESP32 board, follow the steps below:'
)
pdf.bullet_point('Libraries Needed', "Install \"ArduinoJson\" (v6.x or v7.x) and \"PubSubClient\" (by Nick O'Leary) via the Library Manager.")
pdf.bullet_point('IDE Configuration', 'Open Canopus_ModbusApp.ino in Arduino IDE. Go to Tools > Board and select "ESP32 Dev Module".')
pdf.bullet_point('Wiring Details', 'Connect the USB Type-C port of the ModQ board to your computer and select the correct COM port.')
pdf.bullet_point('Firmware Upload', 'Click the "Upload" button. The board LEDs will flash three times on successful boot-up.')

# Section 4: Initial Configuration via COM Port
pdf.heading1('4. Initial COM Port Configuration (Serial CLI)')
pdf.body_text(
    'For brand new boards or situations where WiFi settings are missing, you can plug in the USB Type-C cable and '
    'configure the gateway over the serial interface using simple text commands. Open a Serial Monitor at 115200 baud.'
)

pdf.heading2('Available Command Reference:')
pdf.bullet_point('help', 'Displays the Serial Command reference help menu.')
pdf.bullet_point('status', 'Prints IP addresses, WiFi/MQTT connection state, uptime, and free RAM.')
pdf.bullet_point('config', 'Prints the entire active gateway settings configuration structure in JSON.')
pdf.bullet_point('config set <json>', 'Allows setting the complete configuration by pasting a JSON string.')
pdf.bullet_point('wifi add <ssid> <pass>', 'Saves a WiFi network configuration (supports up to 3 slots).')
pdf.bullet_point('wifi clear', 'Clears all configured WiFi network credentials from flash.')
pdf.bullet_point('mqtt set <srv> <port> [u] [p]', 'Sets MQTT broker server address, port, and optional user/pass.')
pdf.bullet_point('restart / reset', 'Restarts the gateway or performs a factory reset back to defaults.')

pdf.heading2('Example WiFi Serial Configuration:')
pdf.code_block([
    'Received command: wifi add MyHomeWiFi SecretPassword123',
    'SUCCESS: Added WiFi SSID: MyHomeWiFi',
    'Configuring Network Interfaces...',
    'Access Point \'ModQ_AP\' started. IP: 192.168.4.1',
    'Registered 1 WiFi networks.',
    ' - Added network SSID: MyHomeWiFi'
])

# Section 5: Web UI User Guide
pdf.add_page()
pdf.heading1('5. Web UI Interface (Multilingual)')
pdf.body_text(
    'The gateway hosts an offline-capable Single-Page Web application on port 80. Connect your device to the '
    '"ModQ_AP" WiFi network and navigate to http://192.168.4.1 on your browser.'
)
pdf.bullet_point('Language Toggle', 'Select English, Vietnamese, Spanish, or Portuguese from the top dropdown in the sidebar to translate the dashboard dynamically.')
pdf.bullet_point('WiFi Setup', 'Under WiFi Networks, scan nearby APs, select your network, and enter the password. Multi-WiFi operates in the background.')
pdf.bullet_point('Modbus Configuration', 'Configure up to 8 Modbus slave devices. Click "Add Device", assign Slave ID, Baudrate, and Polling Interval.')
pdf.bullet_point('Registers Addition', 'Define up to 16 registers per device. Supported Datatypes include UINT16, INT16, UINT32, INT32, and FLOAT.')

# Section 6: MQTT API Integration
pdf.heading1('6. MQTT API Specification')
pdf.body_text(
    'The gateway supports two-way communication using MQTT over WiFi. The default topics can be adjusted '
    'on the Web UI.'
)

pdf.heading2('6.1 Telemetry JSON Payload Format (Publish):')
pdf.body_text(
    'The gateway publishes Modbus values to the topic "modq/device/data/<slave_id>" every query interval:'
)
pdf.code_block([
    '{',
    '  "name": "Industrial Sensor 1",',
    '  "sid": 1,',
    '  "online": true,',
    '  "error": 0,',
    '  "regs": {',
    '    "Temperature": 24.85,',
    '    "Humidity": 55.40',
    '  }',
    '}'
])

pdf.heading2('6.2 Control Commands (Subscribe):')
pdf.body_text(
    'Write values to slave registers by publishing JSON payloads to "modq/device/cmd":'
)
pdf.code_block([
    '# Publish to topic: modq/device/cmd',
    '{',
    '  "sid": 1,',
    '  "addr": 100,',
    '  "val": 1250',
    '}'
])

# Section 7: HTTP REST API
pdf.add_page()
pdf.heading1('7. HTTP REST API Reference')
pdf.body_text(
    'Developers can integrate the gateway with third-party dashboards or SCADA systems using HTTP REST endpoints.'
)

headers = ['Method', 'Endpoint', 'Response', 'Description']
rows = [
    ['GET', '/', 'HTML', 'Serves the main dashboard Single Page Web UI.'],
    ['GET', '/api/config', 'JSON', 'Returns WiFi, MQTT, and device registers setup.'],
    ['POST', '/api/config', 'JSON', 'Submits and applies new configurations.'],
    ['GET', '/api/data', 'JSON', 'Returns current polled Modbus values.'],
    ['GET', '/api/status', 'JSON', 'Returns WiFi/MQTT connection state, IP, RAM, uptime.'],
    ['GET', '/api/wifi-scan', 'JSON', 'Triggers scan of surrounding WiFi APs.'],
    ['POST', '/api/restart', 'JSON', 'Reboots the ESP32 gateway module.'],
    ['POST', '/api/reset', 'JSON', 'Wipes all flash settings and restarts gateway.']
]
pdf.draw_table(headers, rows, [17, 33, 22, 98])

# Output the file
output_path = os.path.join(os.path.dirname(__file__), 'ModQ_ModbusApp_User_Manual.pdf')
pdf.output(output_path)
print(f"SUCCESS: Generated PDF manual at {output_path}")
