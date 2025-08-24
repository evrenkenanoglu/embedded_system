#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "WiFi.h"
#include "HTTPClient.h"
#include "Update.h"
#include "esp_task_wdt.h"

// Test 1: iot-hardcoded-wifi-credentials
void test_hardcoded_wifi_credentials() {
    const char* wifi_ssid = "MyHomeNetwork";
    const char* wifi_password = "password123";
    char ssid[] = "TestNetwork";
    char password[] = "secretpass";
    #define WIFI_SSID "HardcodedSSID"
    #define WIFI_PASSWORD "HardcodedPass"
    #define WPA_KEY "mysecretkey"
}

// Test 2: iot-mqtt-credentials-hardcoded
void test_hardcoded_mqtt_credentials() {
    const char* mqtt_user = "admin";
    const char* mqtt_pass = "mqttpassword";
    const char* broker_user = "broker_admin";
    const char* broker_pass = "brokerpass123";
    #define MQTT_CLIENT_ID "device_12345"
    #define BROKER_USER "hardcoded_user"
}

// Test 3: iot-api-key-hardcoded
void test_hardcoded_api_keys() {
    const char* api_key = "abcd1234567890efghij";
    const char* apikey = "xyz789abc123def456ghi";
    const char* access_token = "token_abcdef123456789012345";
    #define AUTH_TOKEN "auth_1234567890abcdef"
    #define SECRET_KEY "sk_live_1234567890abcdef"
}

// Test 4: iot-weak-encryption
void test_weak_encryption() {
    char data[] = "sensitive data";
    char key[] = "encryption_key";
    
    // These should trigger warnings
    MD5(data);
    SHA1(data);
    DES_encrypt(data, key);
    RC4_encrypt(data, key);
}

// Test 5: iot-insecure-random
void test_insecure_random() {
    int seed = 12345;
    srand(seed);
    int random_val = rand();
    int another_random = random();
}

// Test 6: iot-debug-info-leak
void test_debug_info_leak() {
    char password[] = "secret123";
    char api_key[] = "key_12345";
    char token[] = "token_abc";
    char secret[] = "topsecret";
    
    printf("User password: %s", password);
    Serial.print(api_key);
    Serial.println(token);
    ESP_LOGI("TAG", "Secret value: %s", secret);
}

// Test 7: iot-http-plaintext
void test_http_plaintext() {
    HTTPClient http;
    WiFiClient client;
    
    http.begin("http://api.example.com/data");
    client.begin("http://insecure-endpoint.com");
}

// Test 8: iot-buffer-overflow-sensor-data
void test_buffer_overflow_sensor_data() {
    char buffer[50];
    char sensor_data[] = "temperature:25.6,humidity:60.2,pressure:1013.25";
    char format[] = "Sensor: %s";
    
    sprintf(buffer, format, sensor_data);
    strcpy(buffer, sensor_data);
}

// Test 9: iot-firmware-version-exposure
void test_firmware_version_exposure() {
    const char* firmware_version = "v1.2.3";
    #define FIRMWARE_VERSION "v2.0.1"
    
    int socket = 1;
    char data[] = "firmware info";
    send(socket, data, strlen(data), 0);
}

// Test 10: iot-ota-update-insecure
void test_ota_update_insecure() {
    size_t update_size = 1024000;
    WiFiClient stream;
    
    // This should trigger warning - no integrity verification
    Update.begin(update_size);
    Update.writeStream(stream);
}

// Test 11: iot-device-id-hardcoded
void test_device_id_hardcoded() {
    const char* device_id = "ESP32_DEVICE_12345678";
    #define DEVICE_ID "HARDCODED_DEVICE_ABC123"
}

// Test 12: iot-watchdog-disabled
void test_watchdog_disabled() {
    TaskHandle_t task_handle;
    
    esp_task_wdt_delete(task_handle);
    esp_task_wdt_deinit();
}

// Test 13: iot-memory-fragmentation
void test_memory_fragmentation() {
    void* ptr;
    size_t size = 1024;
    
    // This should trigger warning - malloc in loop without free
    for (int i = 0; i < 100; i++) {
        ptr = malloc(size);
        // Missing free(ptr) - causes fragmentation
    }
}

// Test 14: iot-sensor-data-validation
void test_sensor_data_validation() {
    int pin = A0;
    int socket = 1;
    char data[100];
    
    // This should trigger warning - no validation
    int sensor_value = analogRead(pin);
    sprintf(data, "sensor:%d", sensor_value);
    send(socket, data, strlen(data), 0);
}

// Test 15: Additional edge cases
void test_edge_cases() {
    // Multiple credentials in one function
    const char* network_key = "netkey123";
    const char* wpa_key = "wpakey456";
    
    // Nested patterns
    for (int i = 0; i < 5; i++) {
        char* buffer = (char*)malloc(100);
        sprintf(buffer, "Data: %d", i);
        // Missing free - should trigger fragmentation warning
    }
    
    // Chained insecure operations
    srand(42);
    int random1 = rand();
    int random2 = random();
    printf("Random numbers: %d, %d", random1, random2);
}

int main() {
    printf("Running IoT Security Tests...\n");
    
    test_hardcoded_wifi_credentials();
    test_hardcoded_mqtt_credentials();
    test_hardcoded_api_keys();
    test_weak_encryption();
    test_insecure_random();
    test_debug_info_leak();
    test_http_plaintext();
    test_buffer_overflow_sensor_data();
    test_firmware_version_exposure();
    test_ota_update_insecure();
    test_device_id_hardcoded();
    test_watchdog_disabled();
    test_memory_fragmentation();
    test_sensor_data_validation();
    test_edge_cases();
    
    printf("All tests completed.\n");
    return 0;
}