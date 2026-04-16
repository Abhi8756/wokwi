#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <esp_ota_ops.h>

/**
 * @file ota.h
 * @brief Over-The-Air (OTA) firmware update system for ESP32
 * 
 * This module provides secure OTA firmware updates with progress reporting,
 * signature verification, and automatic rollback on failure.
 * 
 * Key Features:
 * - ArduinoOTA integration for WiFi-based updates
 * - Real-time progress reporting via MQTT
 * - Firmware signature verification (optional)
 * - Automatic rollback on boot failure
 * - Dual-partition OTA scheme
 * - Update status tracking
 */

// OTA Configuration
#define OTA_HOSTNAME "manhole-sensor"
#define OTA_PASSWORD "secure-ota-password"  // Change in production!
#define OTA_PORT 3232
#define OTA_MAX_RETRIES 3
#define OTA_BOOT_VALIDATION_TIMEOUT 30000  // 30 seconds

// OTA Status
typedef enum {
  OTA_STATUS_IDLE,
  OTA_STATUS_STARTING,
  OTA_STATUS_IN_PROGRESS,
  OTA_STATUS_COMPLETED,
  OTA_STATUS_FAILED,
  OTA_STATUS_VALIDATING
} ota_status_t;

// OTA State
struct OTAState {
  ota_status_t status;
  uint8_t progress;  // 0-100
  size_t total_size;
  size_t current_size;
  unsigned long start_time;
  unsigned long end_time;
  char error_message[128];
  bool initialized;
  uint32_t boot_count;
  bool validation_pending;
};

// Global OTA state
extern OTAState otaState;

/**
 * @brief Initialize OTA update system
 * @param hostname Device hostname for OTA
 * @param password OTA password (optional)
 * @return true if initialization successful, false otherwise
 */
bool ota_init(const char* hostname = OTA_HOSTNAME, const char* password = OTA_PASSWORD);

/**
 * @brief Handle OTA updates (call in main loop)
 */
void ota_handle();

/**
 * @brief Get OTA status
 * @return Current OTA status
 */
ota_status_t ota_get_status();

/**
 * @brief Get OTA status string
 * @return Human-readable status string
 */
const char* ota_get_status_string();

/**
 * @brief Get OTA progress percentage
 * @return Progress (0-100)
 */
uint8_t ota_get_progress();

/**
 * @brief Get OTA status information
 * @param buffer Buffer to store status information
 * @param bufferSize Size of the buffer
 */
void ota_get_status_info(char* buffer, size_t bufferSize);

/**
 * @brief Check if OTA update is in progress
 * @return true if update in progress, false otherwise
 */
inline bool ota_is_updating() {
  return otaState.status == OTA_STATUS_IN_PROGRESS || 
         otaState.status == OTA_STATUS_STARTING;
}

/**
 * @brief Validate current firmware after OTA update
 * @return true if firmware is valid, false otherwise
 */
bool ota_validate_firmware();

/**
 * @brief Mark current firmware as valid (prevents rollback)
 */
void ota_mark_valid();

/**
 * @brief Check if rollback is available
 * @return true if rollback partition exists, false otherwise
 */
bool ota_can_rollback();

/**
 * @brief Perform rollback to previous firmware
 * @return true if rollback initiated, false otherwise
 */
bool ota_rollback();

/**
 * @brief Get boot count (incremented on each boot)
 * @return Boot count
 */
uint32_t ota_get_boot_count();

/**
 * @brief Reset boot count (call after successful validation)
 */
void ota_reset_boot_count();

/**
 * @brief Check if firmware validation is pending
 * @return true if validation pending, false otherwise
 */
bool ota_is_validation_pending();

/**
 * @brief Get partition information
 * @param buffer Buffer to store partition info
 * @param bufferSize Size of the buffer
 */
void ota_get_partition_info(char* buffer, size_t bufferSize);

// OTA Callbacks (for MQTT progress reporting)
typedef void (*ota_progress_callback_t)(uint8_t progress);
typedef void (*ota_error_callback_t)(const char* error);
typedef void (*ota_complete_callback_t)();

extern ota_progress_callback_t ota_progress_callback;
extern ota_error_callback_t ota_error_callback;
extern ota_complete_callback_t ota_complete_callback;

/**
 * @brief Set OTA progress callback
 * @param callback Callback function
 */
inline void ota_set_progress_callback(ota_progress_callback_t callback) {
  ota_progress_callback = callback;
}

/**
 * @brief Set OTA error callback
 * @param callback Callback function
 */
inline void ota_set_error_callback(ota_error_callback_t callback) {
  ota_error_callback = callback;
}

/**
 * @brief Set OTA complete callback
 * @param callback Callback function
 */
inline void ota_set_complete_callback(ota_complete_callback_t callback) {
  ota_complete_callback = callback;
}

#endif // OTA_H