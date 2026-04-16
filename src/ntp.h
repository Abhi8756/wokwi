#ifndef NTP_H
#define NTP_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <sys/time.h>
#include "esp_sntp.h"

/**
 * @file ntp.h
 * @brief NTP (Network Time Protocol) utilities for ESP32
 * 
 * This module provides NTP time synchronization functionality for accurate
 * timestamping in IoT applications. Critical for data logging, event correlation,
 * and system diagnostics.
 * 
 * Key Features:
 * - Multiple NTP server support with fallback
 * - Configurable timezone support
 * - Periodic synchronization
 * - Sync status monitoring
 * - Fallback to millis() offset if NTP fails
 */

// NTP Configuration Constants
#define NTP_SERVER_PRIMARY "pool.ntp.org"
#define NTP_SERVER_SECONDARY "time.nist.gov"
#define NTP_SERVER_TERTIARY "time.google.com"
#define NTP_SYNC_INTERVAL_HOURS 6
#define NTP_SYNC_TIMEOUT_MS 10000
#define NTP_MAX_SYNC_ATTEMPTS 3

// Timezone configuration (can be overridden via environment)
#ifndef NTP_TIMEZONE_OFFSET
#define NTP_TIMEZONE_OFFSET 0  // UTC by default
#endif

#ifndef NTP_DAYLIGHT_OFFSET
#define NTP_DAYLIGHT_OFFSET 0  // No daylight saving by default
#endif

// NTP Status
typedef enum {
  NTP_STATUS_NOT_INITIALIZED,
  NTP_STATUS_SYNCING,
  NTP_STATUS_SYNCED,
  NTP_STATUS_FAILED,
  NTP_STATUS_TIMEOUT
} ntp_status_t;

// NTP State
struct NTPState {
  ntp_status_t status;
  unsigned long lastSyncTime;
  unsigned long lastSyncAttempt;
  int syncAttempts;
  bool initialized;
  time_t bootTime;
  unsigned long bootMillis;
  int timezoneOffset;
  int daylightOffset;
};

// Global NTP state
extern NTPState ntpState;

/**
 * @brief Initialize NTP client
 * @param timezoneOffset Timezone offset in seconds (e.g., -28800 for PST)
 * @param daylightOffset Daylight saving offset in seconds (e.g., 3600 for DST)
 * @return true if initialization successful, false otherwise
 */
inline bool ntp_init(int timezoneOffset = NTP_TIMEZONE_OFFSET, int daylightOffset = NTP_DAYLIGHT_OFFSET) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️  NTP: WiFi not connected, cannot initialize NTP");
    ntpState.status = NTP_STATUS_FAILED;
    return false;
  }

  ntpState.timezoneOffset = timezoneOffset;
  ntpState.daylightOffset = daylightOffset;
  ntpState.bootTime = 0;
  ntpState.bootMillis = millis();
  ntpState.syncAttempts = 0;
  ntpState.initialized = true;

  // Configure SNTP
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, NTP_SERVER_PRIMARY);
  sntp_setservername(1, NTP_SERVER_SECONDARY);
  sntp_setservername(2, NTP_SERVER_TERTIARY);
  
  // Set timezone
  configTime(timezoneOffset, daylightOffset, NTP_SERVER_PRIMARY, NTP_SERVER_SECONDARY, NTP_SERVER_TERTIARY);
  
  sntp_init();
  
  ntpState.status = NTP_STATUS_SYNCING;
  ntpState.lastSyncAttempt = millis();
  
  Serial.println("🕐 NTP client initialized");
  Serial.printf("   Primary server: %s\n", NTP_SERVER_PRIMARY);
  Serial.printf("   Timezone offset: %d seconds\n", timezoneOffset);
  Serial.printf("   Daylight offset: %d seconds\n", daylightOffset);
  
  return true;
}

/**
 * @brief Check if NTP is synchronized
 * @return true if time is synchronized, false otherwise
 */
inline bool ntp_is_synced() {
  if (!ntpState.initialized) {
    return false;
  }
  
  time_t now;
  time(&now);
  return (now > 1000000000); // Valid timestamp (after year 2001)
}

/**
 * @brief Get current time as Unix timestamp
 * @return Unix timestamp, or 0 if not synchronized
 */
inline time_t ntp_get_time() {
  if (!ntpState.initialized) {
    return 0;
  }
  
  time_t now;
  time(&now);
  
  if (now > 1000000000) {
    return now;
  }
  
  // Fallback: estimate time based on boot time and millis()
  if (ntpState.bootTime > 0) {
    unsigned long elapsedSeconds = (millis() - ntpState.bootMillis) / 1000;
    return ntpState.bootTime + elapsedSeconds;
  }
  
  return 0;
}

/**
 * @brief Get current time as ISO 8601 string
 * @param buffer Buffer to store the formatted time string
 * @param bufferSize Size of the buffer
 * @return true if successful, false otherwise
 */
inline bool ntp_get_iso_string(char* buffer, size_t bufferSize) {
  if (!buffer || bufferSize < 25) {
    return false;
  }
  
  time_t now = ntp_get_time();
  if (now == 0) {
    snprintf(buffer, bufferSize, "1970-01-01T00:00:00Z");
    return false;
  }
  
  struct tm* timeinfo = gmtime(&now);
  strftime(buffer, bufferSize, "%Y-%m-%dT%H:%M:%SZ", timeinfo);
  return true;
}

/**
 * @brief Update NTP synchronization status
 * Should be called regularly in main loop
 */
inline void ntp_update() {
  if (!ntpState.initialized) {
    return;
  }
  
  unsigned long currentTime = millis();
  
  // Check if we need to attempt sync
  if (ntpState.status == NTP_STATUS_SYNCING) {
    if (ntp_is_synced()) {
      ntpState.status = NTP_STATUS_SYNCED;
      ntpState.lastSyncTime = currentTime;
      ntpState.syncAttempts = 0;
      
      // Store boot time for fallback calculations
      time_t now;
      time(&now);
      ntpState.bootTime = now - (currentTime / 1000);
      
      Serial.println("✅ NTP synchronized successfully");
      Serial.printf("   Current time: %s\n", ctime(&now));
    } else if (currentTime - ntpState.lastSyncAttempt > NTP_SYNC_TIMEOUT_MS) {
      // Sync attempt timed out
      ntpState.syncAttempts++;
      
      if (ntpState.syncAttempts >= NTP_MAX_SYNC_ATTEMPTS) {
        ntpState.status = NTP_STATUS_FAILED;
        Serial.printf("❌ NTP sync failed after %d attempts\n", NTP_MAX_SYNC_ATTEMPTS);
      } else {
        Serial.printf("⚠️  NTP sync timeout (attempt %d/%d), retrying...\n", 
                     ntpState.syncAttempts, NTP_MAX_SYNC_ATTEMPTS);
        ntpState.lastSyncAttempt = currentTime;
      }
    }
  }
  
  // Check if we need periodic resync
  if (ntpState.status == NTP_STATUS_SYNCED) {
    unsigned long timeSinceLastSync = currentTime - ntpState.lastSyncTime;
    if (timeSinceLastSync > (NTP_SYNC_INTERVAL_HOURS * 3600000UL)) {
      Serial.println("🕐 Initiating periodic NTP resync");
      ntpState.status = NTP_STATUS_SYNCING;
      ntpState.lastSyncAttempt = currentTime;
      ntpState.syncAttempts = 0;
    }
  }
}

/**
 * @brief Get NTP status string
 * @return Human-readable status string
 */
inline const char* ntp_get_status_string() {
  switch (ntpState.status) {
    case NTP_STATUS_NOT_INITIALIZED: return "Not initialized";
    case NTP_STATUS_SYNCING: return "Syncing";
    case NTP_STATUS_SYNCED: return "Synchronized";
    case NTP_STATUS_FAILED: return "Failed";
    case NTP_STATUS_TIMEOUT: return "Timeout";
    default: return "Unknown";
  }
}

/**
 * @brief Get detailed NTP status information
 * @param buffer Buffer to store status information
 * @param bufferSize Size of the buffer
 */
inline void ntp_get_status_info(char* buffer, size_t bufferSize) {
  if (!buffer || bufferSize < 200) {
    return;
  }
  
  char timeStr[32];
  ntp_get_iso_string(timeStr, sizeof(timeStr));
  
  snprintf(buffer, bufferSize,
    "NTP Status: %s\n"
    "  Current time: %s\n"
    "  Sync attempts: %d\n"
    "  Last sync: %lu ms ago\n"
    "  Timezone offset: %d seconds\n"
    "  Servers: %s, %s, %s",
    ntp_get_status_string(),
    timeStr,
    ntpState.syncAttempts,
    ntpState.lastSyncTime > 0 ? millis() - ntpState.lastSyncTime : 0,
    ntpState.timezoneOffset,
    NTP_SERVER_PRIMARY,
    NTP_SERVER_SECONDARY,
    NTP_SERVER_TERTIARY
  );
}

/**
 * @brief Force NTP synchronization
 * @return true if sync initiated, false otherwise
 */
inline bool ntp_force_sync() {
  if (!ntpState.initialized) {
    return false;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️  NTP: Cannot sync - WiFi not connected");
    return false;
  }
  
  Serial.println("🕐 Forcing NTP synchronization...");
  ntpState.status = NTP_STATUS_SYNCING;
  ntpState.lastSyncAttempt = millis();
  ntpState.syncAttempts = 0;
  
  // Restart SNTP
  sntp_stop();
  sntp_init();
  
  return true;
}

#endif // NTP_H