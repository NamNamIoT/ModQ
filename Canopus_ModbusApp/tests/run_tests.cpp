#include <iostream>

// Declare external test suites
extern void test_crc16_basic();
extern void test_modbus_read_holding_success();
extern void test_modbus_read_input_registers_success();
extern void test_modbus_write_single_register_success();
extern void test_modbus_exception_response();
extern void test_modbus_invalid_crc();
extern void test_modbus_invalid_slave_id();
extern void test_modbus_invalid_function_code();
extern void test_modbus_timeout();
extern void test_modbus_size_overflow_protection();
extern void test_modbus_null_serial();
extern void test_modbus_get_response_buffer_oob();

extern void test_settings_defaults();
extern void test_settings_json_serialization();
extern void test_settings_save_load_nvs();
extern void test_settings_json_import();
extern void test_settings_invalid_json();
extern void test_settings_empty_json();
extern void test_settings_max_wifi_nets();
extern void test_settings_max_devices();

int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "      Canopus Modbus App Unit Tests        " << std::endl;
    std::cout << "===========================================" << std::endl;

    // Run CRC Tests
    test_crc16_basic();

    // Run Modbus Master Protocol Tests
    test_modbus_read_holding_success();
    test_modbus_read_input_registers_success();
    test_modbus_write_single_register_success();
    test_modbus_exception_response();
    test_modbus_invalid_crc();
    test_modbus_invalid_slave_id();
    test_modbus_invalid_function_code();
    test_modbus_timeout();
    test_modbus_size_overflow_protection();
    test_modbus_null_serial();
    test_modbus_get_response_buffer_oob();

    // Run Settings Storage & JSON Configuration Tests
    test_settings_defaults();
    test_settings_json_serialization();
    test_settings_save_load_nvs();
    test_settings_json_import();
    test_settings_invalid_json();
    test_settings_empty_json();
    test_settings_max_wifi_nets();
    test_settings_max_devices();

    std::cout << "===========================================" << std::endl;
    std::cout << "   ALL TESTS PASSED SUCCESSFULLY! (20/20)  " << std::endl;
    std::cout << "===========================================" << std::endl;
    return 0;
}
