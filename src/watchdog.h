#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <Arduino.h>
#include "esp_task_wdt.h"
#include "esp_system.h"

/**
 * @file watchdog.h
 * @brief ESP32 Task Watchdog Timer utilities for system reliability
 * 
 * This module provides watchdog timer functionality to automatically
 * recover from system hangs and crashes. Critical for production
 * embedded systems that must operate unattended.
 * 
 * Key Features:
 * - Hardware watchdog timer with configurable timeout
 * - Automatic system recovery from hangs
 * - Boot reason detection and logging
 * - Watchdog status monitoring
 * - Test functionality for validation
 */

// Watchdog Configuration Constants
#define WDT_TIMEOUT_SECONDS 8           // Main watchdog timeout
#define WDT_PANIC_TIMEOUT_SECONDS 10    // Panic timeout (should be > WDT_TIMEOUT)

/**
 * @brief Initialize the ESP32 Task Watchdog Timer
 * @param timeout_seconds Watchdog timeout in seconds
 * @param panic_on_timeout Whether to panic (reboot) on timeout
 * @return true if initialization successful, false otherwise
 * 
 * This function configures the hardware watchdog timer and adds the
 * current task to watchdog monitoring. Must be called during setup().
 */
inline bool watchdog_init(uint32_t timeout_seconds = WDT_TIMEOUT_SECONDS, bool panic_on_timeout = true) {
  esp_err_t result = esp_task_wdt_init(timeout_seconds, panic_on_timeout);
  if (result == ESP_OK) {
    result = esp_task_wdt_add(NULL); // Add current task
    if (result == ESP_OK) {
      Serial.printf("Watchdog initialized: %lu second timeout\n", timeout_seconds);
      return true;
    } else {
      Serial.printf("ERROR: Failed to add task to watchdog: %s\n", esp_err_to_name(result));
      return false;
    }
  } else {
    Serial.printf("ERROR: Failed to initialize watchdog: %s\n", esp_err_to_name(result));
    return false;
  }
}

/**
 * @brief Reset the watchdog timer (feed the dog)
 * 
 * This function must be called regularly (more frequently than the timeout)
 * to prevent the watchdog from triggering a system reset. Should be called
 * in the main loop.
 */
inline void watchdog_reset() {
  esp_task_wdt_reset();
}

/**
 * @brief Get human-readable description of ESP32 reset reason
 * @param reset_reason The reset reason from esp_reset_reason()
 * @return String description of the reset reason
 */
inline const char* get_reset_reason_string(esp_reset_reason_t reset_reason) {
  switch (reset_reason) {
    case ESP_RST_POWERON:   return "Power-on reset";
    case ESP_RST_EXT:       return "External reset";
    case ESP_RST_SW:        return "Software reset";
    case ESP_RST_PANIC:     return "Exception/panic reset";
    case ESP_RST_INT_WDT:   return "WATCHDOG TIMEOUT - System recovered from hang!";
    case ESP_RST_TASK_WDT:  return "TASK WATCHDOG TIMEOUT - System recovered from hang!";
    case ESP_RST_WDT:       return "OTHER WATCHDOG TIMEOUT - System recovered!";
    case ESP_RST_DEEPSLEEP: return "Deep sleep reset";
    case ESP_RST_BROWNOUT:  return "Brownout reset";
    case ESP_RST_SDIO:      return "SDIO reset";
    default:                return "Unknown reset reason";
  }
}

/**
 * @brief Check if the last reset was due to watchdog timeout
 * @return true if last reset was watchdog-related, false otherwise
 */
inline bool was_watchdog_reset() {
  esp_reset_reason_t reason = esp_reset_reason();
  return (reason == ESP_RST_INT_WDT || 
          reason == ESP_RST_TASK_WDT || 
          reason == ESP_RST_WDT);
}

/**
 * @brief Print detailed boot information including reset reason
 * 
 * This function should be called early in setup() to log why the
 * system rebooted. Useful for debugging and monitoring system health.
 */
inline void print_boot_info() {
  esp_reset_reason_t reset_reason = esp_reset_reason();
  Serial.printf("Boot reason: %s\n", get_reset_reason_string(reset_reason));
  
  if (was_watchdog_reset()) {
    Serial.println("*** WATCHDOG RECOVERY DETECTED ***");
    Serial.println("*** Previous execution may have hung ***");
  }
  
  // Additional system information
  Serial.printf("ESP32 Chip ID: %08X\n", (uint32_t)ESP.getEfuseMac());
  Serial.printf("Flash size: %u bytes\n", ESP.getFlashChipSize());
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("CPU frequency: %u MHz\n", ESP.getCpuFreqMHz());
}

/**
 * @brief Intentionally hang the system to test watchdog functionality
 * @param warning_message Message to display before hanging
 * 
 * WARNING: This function will cause the system to hang and reboot!
 * Only use for testing watchdog functionality in development.
 */
inline void test_watchdog_hang(const char* warning_message = "Testing watchdog - system will reboot") {
  Serial.println("*** WATCHDOG TEST MODE ***");
  Serial.printf("WARNING: %s\n", warning_message);
  Serial.printf("System should reboot in %d seconds due to watchdog timeout.\n", WDT_TIMEOUT_SECONDS);
  Serial.flush(); // Ensure message is sent before hanging
  
  // Intentional infinite loop - DO NOT call esp_task_wdt_reset()
  unsigned long hang_start = millis();
  while(true) {
    // Show countdown to help with testing
    unsigned long elapsed = (millis() - hang_start) / 1000;
    if (elapsed < WDT_TIMEOUT_SECONDS) {
      Serial.printf("Hanging... %lu seconds elapsed\n", elapsed);
      delay(1000);
    } else {
      // Should not reach here - watchdog should have triggered
      Serial.println("ERROR: Watchdog did not trigger!");
      delay(1000);
    }
  }
}

/**
 * @brief Get watchdog status information
 * @return String with watchdog configuration details
 */
inline String get_watchdog_status() {
  String status = "Watchdog Status:\n";
  status += "  Timeout: " + String(WDT_TIMEOUT_SECONDS) + " seconds\n";
  status += "  Panic timeout: " + String(WDT_PANIC_TIMEOUT_SECONDS) + " seconds\n";
  status += "  Last reset: " + String(get_reset_reason_string(esp_reset_reason())) + "\n";
  status += "  Watchdog recovery: " + String(was_watchdog_reset() ? "YES" : "NO");
  return status;
}

#endif // WATCHDOG_H