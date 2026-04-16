#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_system.h>

/**
 * @file diagnostics.h
 * @brief Fault detection and system diagnostics for ESP32 IoT system
 * 
 * This module provides comprehensive fault detection including sensor health
 * monitoring, brownout detection, WiFi quality monitoring, and system diagnostics.
 * 
 * Key Features:
 * - Sensor self-test on startup
 * - Sensor disconnection detection (ADC range check)
 * - Stuck sensor detection (same value 10+ times)
 * - Brownout detection and safe shutdown
 * - WiFi signal strength and connection quality monitoring
 * - System health status reporting
 */

// Sensor Health Status
typedef enum {
  SENSOR_STATUS_OK,
  SENSOR_STATUS_DISCONNECTED,
  SENSOR_STATUS_STUCK,
  SENSOR_STATUS_OUT_OF_RANGE,
  SENSOR_STATUS_SELF_TEST_FAILED,
  SENSOR_STATUS_UNKNOWN
} sensor_status_t;

// System Health Status
typedef enum {
  SYSTEM_HEALTH_GOOD,
  SYSTEM_HEALTH_WARNING,
  SYSTEM_HEALTH_CRITICAL,
  SYSTEM_HEALTH_FAILURE
} system_health_t;

// Sensor Health Structure
struct SensorHealth {
  sensor_status_t status;
  float last_value;
  float stuck_value;
  uint8_t stuck_count;
  uint8_t consecutive_errors;
  unsigned long last_update;
  bool self_test_passed;
};

// WiFi Quality Structure
struct WiFiQuality {
  int8_t rssi;              // Signal strength in dBm
  uint8_t quality_percent;  // 0-100%
  uint32_t reconnections;
  unsigned long last_disconnect;
  unsigned long total_downtime_ms;
  bool is_connected;
};

// System Diagnostics Structure
struct SystemDiagnostics {
  // Sensor health
  SensorHealth ch4_sensor;
  SensorHealth h2s_sensor;
  SensorHealth water_sensor;
  
  // WiFi quality
  WiFiQuality wifi;
  
  // System health
  system_health_t overall_health;
  uint32_t brownout_count;
  uint32_t watchdog_resets;
  uint32_t panic_count;
  
  // Memory health
  uint32_t min_free_heap;
  uint32_t heap_fragmentation;
  
  // Uptime
  unsigned long uptime_ms;
  unsigned long last_diagnostic_update;
  
  // Fault flags
  bool brownout_detected;
  bool low_memory_warning;
  bool wifi_quality_poor;
  bool sensor_fault_detected;
};

// Global diagnostics instance
extern SystemDiagnostics systemDiagnostics;

/**
 * @brief Initialize diagnostics system
 * @return true if initialization successful, false otherwise
 */
bool diagnostics_init();

/**
 * @brief Perform sensor self-test
 * @param ch4_pin CH4 sensor ADC pin
 * @param h2s_pin H2S sensor ADC pin
 * @param water_pin Water level sensor ADC pin
 * @return true if all sensors passed, false otherwise
 */
bool diagnostics_sensor_self_test(int ch4_pin, int h2s_pin, int water_pin);

/**
 * @brief Update sensor health status
 * @param sensor Sensor health structure
 * @param current_value Current sensor reading
 * @param min_valid Minimum valid ADC value
 * @param max_valid Maximum valid ADC value
 */
void diagnostics_update_sensor(SensorHealth* sensor, float current_value, int min_valid, int max_valid);

/**
 * @brief Check if sensor is disconnected (ADC at extremes)
 * @param adc_value ADC reading
 * @param min_valid Minimum valid value
 * @param max_valid Maximum valid value
 * @return true if disconnected, false otherwise
 */
bool diagnostics_is_sensor_disconnected(int adc_value, int min_valid, int max_valid);

/**
 * @brief Check if sensor is stuck (same value repeatedly)
 * @param sensor Sensor health structure
 * @param current_value Current sensor reading
 * @param threshold Number of consecutive same values to trigger stuck detection
 * @return true if stuck, false otherwise
 */
bool diagnostics_is_sensor_stuck(SensorHealth* sensor, float current_value, uint8_t threshold);

/**
 * @brief Update WiFi quality metrics
 */
void diagnostics_update_wifi();

/**
 * @brief Get WiFi signal quality as percentage
 * @param rssi RSSI value in dBm
 * @return Quality percentage (0-100)
 */
uint8_t diagnostics_wifi_quality_percent(int8_t rssi);

/**
 * @brief Check for brownout condition
 * @return true if brownout detected, false otherwise
 */
bool diagnostics_check_brownout();

/**
 * @brief Update system diagnostics
 * Should be called regularly (e.g., every second)
 */
void diagnostics_update();

/**
 * @brief Get overall system health status
 * @return System health status
 */
system_health_t diagnostics_get_system_health();

/**
 * @brief Get system health as string
 * @return Human-readable health string
 */
const char* diagnostics_get_health_string(system_health_t health);

/**
 * @brief Get sensor status as string
 * @return Human-readable sensor status string
 */
const char* diagnostics_get_sensor_status_string(sensor_status_t status);

/**
 * @brief Print comprehensive diagnostics report
 */
void diagnostics_print_report();

/**
 * @brief Get diagnostics as JSON string
 * @param buffer Buffer to store JSON string
 * @param bufferSize Size of the buffer
 * @return true if successful, false otherwise
 */
bool diagnostics_to_json(char* buffer, size_t bufferSize);

/**
 * @brief Reset diagnostics counters
 */
void diagnostics_reset_counters();

/**
 * @brief Check if any critical faults are present
 * @return true if critical faults detected, false otherwise
 */
bool diagnostics_has_critical_faults();

/**
 * @brief Get sensor health summary
 * @param buffer Buffer to store summary
 * @param bufferSize Size of the buffer
 */
void diagnostics_get_sensor_summary(char* buffer, size_t bufferSize);

// Brownout detection callback
typedef void (*brownout_callback_t)();
extern brownout_callback_t brownout_callback;

/**
 * @brief Set brownout detection callback
 * @param callback Callback function
 */
inline void diagnostics_set_brownout_callback(brownout_callback_t callback) {
  brownout_callback = callback;
}

#endif // DIAGNOSTICS_H