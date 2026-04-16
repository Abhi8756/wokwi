#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Preferences.h>

/**
 * @file config.h
 * @brief EEPROM-based configuration management for ESP32 IoT system
 * 
 * This module provides persistent configuration storage using ESP32's NVS
 * (Non-Volatile Storage) with CRC32 validation for data integrity.
 * 
 * Key Features:
 * - Persistent configuration across power cycles
 * - CRC32 checksum validation for corruption detection
 * - Factory reset capability
 * - Remote configuration updates via MQTT
 * - Default configuration fallback
 * - Configuration versioning for migration
 */

// Configuration Version
#define CONFIG_VERSION 1
#define CONFIG_NAMESPACE "iot_config"
#define CONFIG_CRC_KEY "config_crc"

// Configuration Keys
#define KEY_VERSION "version"
#define KEY_DEVICE_ID "device_id"
#define KEY_LOCATION_ID "location_id"
#define KEY_WIFI_SSID "wifi_ssid"
#define KEY_WIFI_PASSWORD "wifi_pass"
#define KEY_MQTT_BROKER "mqtt_broker"
#define KEY_MQTT_PORT "mqtt_port"
#define KEY_MQTT_USER "mqtt_user"
#define KEY_MQTT_PASS "mqtt_pass"
#define KEY_MQTT_TLS_EN "mqtt_tls_en"
#define KEY_MQTT_TLS_VERIFY "mqtt_tls_ver"
#define KEY_MQTT_TLS_PORT "mqtt_tls_port"
#define KEY_NTP_TIMEZONE "ntp_tz"
#define KEY_SAMPLE_INTERVAL "sample_int"
#define KEY_CH4_THRESHOLD "ch4_thresh"
#define KEY_H2S_THRESHOLD "h2s_thresh"
#define KEY_WATER_THRESHOLD "water_thresh"
#define KEY_BUZZER_ENABLED "buzzer_en"
#define KEY_MQTT_ENABLED "mqtt_en"
#define KEY_NTP_ENABLED "ntp_en"

// Configuration Structure
struct SystemConfig {
  // Version and identification
  uint8_t version;
  char device_id[16];
  char location_id[32];
  
  // WiFi Configuration
  char wifi_ssid[32];
  char wifi_password[64];
  
  // MQTT Configuration
  char mqtt_broker[64];
  uint16_t mqtt_port;
  char mqtt_username[32];
  char mqtt_password[64];
  
  // MQTT TLS Configuration (REQ-014)
  bool mqtt_tls_enabled;
  bool mqtt_tls_verify;
  uint16_t mqtt_tls_port;
  
  // NTP Configuration
  int16_t ntp_timezone_offset;  // Seconds
  
  // Sensor Configuration
  uint16_t sample_interval_ms;
  float ch4_threshold_ppm;
  float h2s_threshold_ppm;
  float water_threshold_cm;
  
  // Feature Flags
  bool buzzer_enabled;
  bool mqtt_enabled;
  bool ntp_enabled;
  
  // CRC32 checksum (calculated over all fields except this one)
  uint32_t crc32;
};

// Global configuration instance
extern SystemConfig currentConfig;
extern Preferences preferences;

/**
 * @brief Initialize configuration system
 * @return true if initialization successful, false otherwise
 */
bool config_init();

/**
 * @brief Load configuration from NVS
 * @return true if loaded successfully, false if using defaults
 */
bool config_load();

/**
 * @brief Save configuration to NVS
 * @return true if saved successfully, false otherwise
 */
bool config_save();

/**
 * @brief Reset configuration to factory defaults
 * @return true if reset successful, false otherwise
 */
bool config_factory_reset();

/**
 * @brief Validate configuration integrity
 * @return true if configuration is valid, false if corrupted
 */
bool config_validate();

/**
 * @brief Calculate CRC32 checksum for configuration
 * @param config Configuration structure to checksum
 * @return CRC32 checksum value
 */
uint32_t config_calculate_crc32(const SystemConfig* config);

/**
 * @brief Load default configuration
 */
void config_load_defaults();

/**
 * @brief Print current configuration to serial
 */
void config_print();

/**
 * @brief Update configuration from JSON string (for MQTT updates)
 * @param json JSON string containing configuration updates
 * @return true if updated successfully, false otherwise
 */
bool config_update_from_json(const char* json);

/**
 * @brief Get configuration as JSON string
 * @param buffer Buffer to store JSON string
 * @param bufferSize Size of the buffer
 * @return true if successful, false otherwise
 */
bool config_to_json(char* buffer, size_t bufferSize);

/**
 * @brief Set device ID
 * @param device_id Device ID string
 * @return true if set successfully, false otherwise
 */
bool config_set_device_id(const char* device_id);

/**
 * @brief Set location ID
 * @param location_id Location ID string
 * @return true if set successfully, false otherwise
 */
bool config_set_location_id(const char* location_id);

/**
 * @brief Set WiFi credentials
 * @param ssid WiFi SSID
 * @param password WiFi password
 * @return true if set successfully, false otherwise
 */
bool config_set_wifi(const char* ssid, const char* password);

/**
 * @brief Set MQTT broker configuration
 * @param broker MQTT broker address
 * @param port MQTT broker port
 * @param username MQTT username (optional)
 * @param password MQTT password (optional)
 * @return true if set successfully, false otherwise
 */
bool config_set_mqtt(const char* broker, uint16_t port, const char* username = nullptr, const char* password = nullptr);

/**
 * @brief Set sensor thresholds
 * @param ch4_threshold CH4 threshold in ppm
 * @param h2s_threshold H2S threshold in ppm
 * @param water_threshold Water level threshold in cm
 * @return true if set successfully, false otherwise
 */
bool config_set_thresholds(float ch4_threshold, float h2s_threshold, float water_threshold);

/**
 * @brief Enable or disable buzzer
 * @param enabled true to enable, false to disable
 * @return true if set successfully, false otherwise
 */
bool config_set_buzzer_enabled(bool enabled);

/**
 * @brief Enable or disable MQTT
 * @param enabled true to enable, false to disable
 * @return true if set successfully, false otherwise
 */
bool config_set_mqtt_enabled(bool enabled);

/**
 * @brief Enable or disable NTP
 * @param enabled true to enable, false to disable
 * @return true if set successfully, false otherwise
 */
bool config_set_ntp_enabled(bool enabled);

// CRC32 calculation helper
inline uint32_t crc32_byte(uint32_t crc, uint8_t byte) {
  crc ^= byte;
  for (int i = 0; i < 8; i++) {
    crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
  }
  return crc;
}

inline uint32_t crc32_buffer(const uint8_t* buffer, size_t length) {
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < length; i++) {
    crc = crc32_byte(crc, buffer[i]);
  }
  return ~crc;
}

#endif // CONFIG_H