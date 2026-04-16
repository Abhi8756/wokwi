#include "diagnostics.h"
#include <ArduinoJson.h>

// Global diagnostics instance
SystemDiagnostics systemDiagnostics;

// Brownout callback
brownout_callback_t brownout_callback = nullptr;

// ADC valid ranges (for 10-bit ADC: 0-1023)
#define ADC_MIN_VALID 10      // Below this = disconnected (too low)
#define ADC_MAX_VALID 1013    // Above this = disconnected (too high)
#define STUCK_THRESHOLD 10    // Number of identical readings to trigger stuck detection
#define STUCK_TOLERANCE 2     // Tolerance for "same" value detection

// WiFi RSSI thresholds
#define WIFI_EXCELLENT -50    // dBm
#define WIFI_GOOD -60         // dBm
#define WIFI_FAIR -70         // dBm
#define WIFI_POOR -80         // dBm

bool diagnostics_init() {
  Serial.println("🔍 Initializing diagnostics system...");
  
  // Initialize sensor health structures
  systemDiagnostics.ch4_sensor = {
    .status = SENSOR_STATUS_UNKNOWN,
    .last_value = 0,
    .stuck_value = 0,
    .stuck_count = 0,
    .consecutive_errors = 0,
    .last_update = 0,
    .self_test_passed = false
  };
  
  systemDiagnostics.h2s_sensor = systemDiagnostics.ch4_sensor;
  systemDiagnostics.water_sensor = systemDiagnostics.ch4_sensor;
  
  // Initialize WiFi quality
  systemDiagnostics.wifi = {
    .rssi = 0,
    .quality_percent = 0,
    .reconnections = 0,
    .last_disconnect = 0,
    .total_downtime_ms = 0,
    .is_connected = false
  };
  
  // Initialize system health
  systemDiagnostics.overall_health = SYSTEM_HEALTH_GOOD;
  systemDiagnostics.brownout_count = 0;
  systemDiagnostics.watchdog_resets = 0;
  systemDiagnostics.panic_count = 0;
  systemDiagnostics.min_free_heap = ESP.getFreeHeap();
  systemDiagnostics.heap_fragmentation = 0;
  systemDiagnostics.uptime_ms = 0;
  systemDiagnostics.last_diagnostic_update = millis();
  
  // Initialize fault flags
  systemDiagnostics.brownout_detected = false;
  systemDiagnostics.low_memory_warning = false;
  systemDiagnostics.wifi_quality_poor = false;
  systemDiagnostics.sensor_fault_detected = false;
  
  Serial.println("✅ Diagnostics system initialized");
  return true;
}

bool diagnostics_sensor_self_test(int ch4_pin, int h2s_pin, int water_pin) {
  Serial.println("🔍 Performing sensor self-test...");
  
  bool all_passed = true;
  
  // Test CH4 sensor
  int ch4_reading = analogRead(ch4_pin);
  if (diagnostics_is_sensor_disconnected(ch4_reading, ADC_MIN_VALID, ADC_MAX_VALID)) {
    Serial.printf("❌ CH4 sensor self-test FAILED (reading=%d)\n", ch4_reading);
    systemDiagnostics.ch4_sensor.status = SENSOR_STATUS_SELF_TEST_FAILED;
    systemDiagnostics.ch4_sensor.self_test_passed = false;
    all_passed = false;
  } else {
    Serial.printf("✅ CH4 sensor self-test PASSED (reading=%d)\n", ch4_reading);
    systemDiagnostics.ch4_sensor.status = SENSOR_STATUS_OK;
    systemDiagnostics.ch4_sensor.self_test_passed = true;
  }
  
  // Test H2S sensor
  int h2s_reading = analogRead(h2s_pin);
  if (diagnostics_is_sensor_disconnected(h2s_reading, ADC_MIN_VALID, ADC_MAX_VALID)) {
    Serial.printf("❌ H2S sensor self-test FAILED (reading=%d)\n", h2s_reading);
    systemDiagnostics.h2s_sensor.status = SENSOR_STATUS_SELF_TEST_FAILED;
    systemDiagnostics.h2s_sensor.self_test_passed = false;
    all_passed = false;
  } else {
    Serial.printf("✅ H2S sensor self-test PASSED (reading=%d)\n", h2s_reading);
    systemDiagnostics.h2s_sensor.status = SENSOR_STATUS_OK;
    systemDiagnostics.h2s_sensor.self_test_passed = true;
  }
  
  // Test Water Level sensor
  int water_reading = analogRead(water_pin);
  if (diagnostics_is_sensor_disconnected(water_reading, ADC_MIN_VALID, ADC_MAX_VALID)) {
    Serial.printf("❌ Water sensor self-test FAILED (reading=%d)\n", water_reading);
    systemDiagnostics.water_sensor.status = SENSOR_STATUS_SELF_TEST_FAILED;
    systemDiagnostics.water_sensor.self_test_passed = false;
    all_passed = false;
  } else {
    Serial.printf("✅ Water sensor self-test PASSED (reading=%d)\n", water_reading);
    systemDiagnostics.water_sensor.status = SENSOR_STATUS_OK;
    systemDiagnostics.water_sensor.self_test_passed = true;
  }
  
  if (all_passed) {
    Serial.println("✅ All sensors passed self-test");
  } else {
    Serial.println("⚠️  Some sensors failed self-test - system will continue with degraded functionality");
    systemDiagnostics.sensor_fault_detected = true;
  }
  
  return all_passed;
}

void diagnostics_update_sensor(SensorHealth* sensor, float current_value, int min_valid, int max_valid) {
  if (!sensor) return;
  
  sensor->last_update = millis();
  
  // Check for disconnection (ADC at extremes)
  int adc_value = (int)current_value;  // Assuming raw ADC value passed
  if (diagnostics_is_sensor_disconnected(adc_value, min_valid, max_valid)) {
    sensor->status = SENSOR_STATUS_DISCONNECTED;
    sensor->consecutive_errors++;
    systemDiagnostics.sensor_fault_detected = true;
    return;
  }
  
  // Check for stuck sensor
  if (diagnostics_is_sensor_stuck(sensor, current_value, STUCK_THRESHOLD)) {
    sensor->status = SENSOR_STATUS_STUCK;
    sensor->consecutive_errors++;
    systemDiagnostics.sensor_fault_detected = true;
    return;
  }
  
  // Sensor is healthy
  if (sensor->status != SENSOR_STATUS_OK) {
    Serial.printf("✅ Sensor recovered (was %s)\n", diagnostics_get_sensor_status_string(sensor->status));
  }
  
  sensor->status = SENSOR_STATUS_OK;
  sensor->consecutive_errors = 0;
  sensor->last_value = current_value;
}

bool diagnostics_is_sensor_disconnected(int adc_value, int min_valid, int max_valid) {
  return (adc_value < min_valid || adc_value > max_valid);
}

bool diagnostics_is_sensor_stuck(SensorHealth* sensor, float current_value, uint8_t threshold) {
  if (!sensor) return false;
  
  // Check if value is within tolerance of stuck value
  if (abs(current_value - sensor->stuck_value) <= STUCK_TOLERANCE) {
    sensor->stuck_count++;
    
    if (sensor->stuck_count >= threshold) {
      return true;
    }
  } else {
    // Value changed, reset stuck detection
    sensor->stuck_value = current_value;
    sensor->stuck_count = 1;
  }
  
  return false;
}

void diagnostics_update_wifi() {
  if (WiFi.status() == WL_CONNECTED) {
    systemDiagnostics.wifi.is_connected = true;
    systemDiagnostics.wifi.rssi = WiFi.RSSI();
    systemDiagnostics.wifi.quality_percent = diagnostics_wifi_quality_percent(systemDiagnostics.wifi.rssi);
    
    // Check for poor WiFi quality
    if (systemDiagnostics.wifi.quality_percent < 30) {
      systemDiagnostics.wifi_quality_poor = true;
    } else {
      systemDiagnostics.wifi_quality_poor = false;
    }
  } else {
    if (systemDiagnostics.wifi.is_connected) {
      // Just disconnected
      systemDiagnostics.wifi.reconnections++;
      systemDiagnostics.wifi.last_disconnect = millis();
    }
    
    systemDiagnostics.wifi.is_connected = false;
    systemDiagnostics.wifi.rssi = -100;  // Worst possible
    systemDiagnostics.wifi.quality_percent = 0;
    systemDiagnostics.wifi_quality_poor = true;
    
    // Track downtime
    if (systemDiagnostics.wifi.last_disconnect > 0) {
      systemDiagnostics.wifi.total_downtime_ms += millis() - systemDiagnostics.wifi.last_disconnect;
    }
  }
}

uint8_t diagnostics_wifi_quality_percent(int8_t rssi) {
  // Convert RSSI to quality percentage
  // RSSI typically ranges from -30 (excellent) to -90 (unusable)
  
  if (rssi >= WIFI_EXCELLENT) {
    return 100;
  } else if (rssi >= WIFI_GOOD) {
    return 80 + ((rssi - WIFI_GOOD) * 20) / (WIFI_EXCELLENT - WIFI_GOOD);
  } else if (rssi >= WIFI_FAIR) {
    return 60 + ((rssi - WIFI_FAIR) * 20) / (WIFI_GOOD - WIFI_FAIR);
  } else if (rssi >= WIFI_POOR) {
    return 40 + ((rssi - WIFI_POOR) * 20) / (WIFI_FAIR - WIFI_POOR);
  } else if (rssi >= -90) {
    return ((rssi + 90) * 40) / 10;
  } else {
    return 0;
  }
}

bool diagnostics_check_brownout() {
  // Check supply voltage (ESP32 specific)
  // Note: This is a simplified check. Real brownout detection would use hardware interrupts
  
  uint32_t free_heap = ESP.getFreeHeap();
  
  // Check for critically low memory (potential brownout symptom)
  if (free_heap < 10000) {  // Less than 10KB free
    systemDiagnostics.brownout_detected = true;
    systemDiagnostics.brownout_count++;
    
    if (brownout_callback) {
      brownout_callback();
    }
    
    return true;
  }
  
  return false;
}

void diagnostics_update() {
  unsigned long current_time = millis();
  systemDiagnostics.uptime_ms = current_time;
  
  // Update WiFi quality
  diagnostics_update_wifi();
  
  // Update memory health
  uint32_t current_free_heap = ESP.getFreeHeap();
  if (current_free_heap < systemDiagnostics.min_free_heap) {
    systemDiagnostics.min_free_heap = current_free_heap;
  }
  
  // Check for low memory warning
  if (current_free_heap < 20000) {  // Less than 20KB free
    systemDiagnostics.low_memory_warning = true;
  } else {
    systemDiagnostics.low_memory_warning = false;
  }
  
  // Calculate heap fragmentation (simplified)
  uint32_t total_heap = ESP.getHeapSize();
  systemDiagnostics.heap_fragmentation = ((total_heap - current_free_heap) * 100) / total_heap;
  
  // Check for brownout
  diagnostics_check_brownout();
  
  // Update overall system health
  systemDiagnostics.overall_health = diagnostics_get_system_health();
  
  systemDiagnostics.last_diagnostic_update = current_time;
}

system_health_t diagnostics_get_system_health() {
  // Determine overall system health based on various factors
  
  // Critical conditions
  if (systemDiagnostics.brownout_detected ||
      systemDiagnostics.ch4_sensor.status == SENSOR_STATUS_SELF_TEST_FAILED ||
      systemDiagnostics.h2s_sensor.status == SENSOR_STATUS_SELF_TEST_FAILED ||
      systemDiagnostics.water_sensor.status == SENSOR_STATUS_SELF_TEST_FAILED) {
    return SYSTEM_HEALTH_CRITICAL;
  }
  
  // Failure conditions
  if ((systemDiagnostics.ch4_sensor.status == SENSOR_STATUS_DISCONNECTED &&
       systemDiagnostics.h2s_sensor.status == SENSOR_STATUS_DISCONNECTED) ||
      systemDiagnostics.low_memory_warning) {
    return SYSTEM_HEALTH_FAILURE;
  }
  
  // Warning conditions
  if (systemDiagnostics.sensor_fault_detected ||
      systemDiagnostics.wifi_quality_poor ||
      systemDiagnostics.ch4_sensor.status != SENSOR_STATUS_OK ||
      systemDiagnostics.h2s_sensor.status != SENSOR_STATUS_OK ||
      systemDiagnostics.water_sensor.status != SENSOR_STATUS_OK) {
    return SYSTEM_HEALTH_WARNING;
  }
  
  return SYSTEM_HEALTH_GOOD;
}

const char* diagnostics_get_health_string(system_health_t health) {
  switch (health) {
    case SYSTEM_HEALTH_GOOD: return "Good";
    case SYSTEM_HEALTH_WARNING: return "Warning";
    case SYSTEM_HEALTH_CRITICAL: return "Critical";
    case SYSTEM_HEALTH_FAILURE: return "Failure";
    default: return "Unknown";
  }
}

const char* diagnostics_get_sensor_status_string(sensor_status_t status) {
  switch (status) {
    case SENSOR_STATUS_OK: return "OK";
    case SENSOR_STATUS_DISCONNECTED: return "Disconnected";
    case SENSOR_STATUS_STUCK: return "Stuck";
    case SENSOR_STATUS_OUT_OF_RANGE: return "Out of Range";
    case SENSOR_STATUS_SELF_TEST_FAILED: return "Self-Test Failed";
    case SENSOR_STATUS_UNKNOWN: return "Unknown";
    default: return "Invalid";
  }
}

void diagnostics_print_report() {
  Serial.println("\n=== System Diagnostics Report ===");
  
  // Overall health
  Serial.printf("Overall Health: %s\n", diagnostics_get_health_string(systemDiagnostics.overall_health));
  Serial.printf("Uptime: %lu ms (%.2f hours)\n", systemDiagnostics.uptime_ms, systemDiagnostics.uptime_ms / 3600000.0);
  
  // Sensor health
  Serial.println("\nSensor Health:");
  Serial.printf("  CH4 Sensor: %s (errors=%u, last_update=%lu ms ago)\n",
                diagnostics_get_sensor_status_string(systemDiagnostics.ch4_sensor.status),
                systemDiagnostics.ch4_sensor.consecutive_errors,
                millis() - systemDiagnostics.ch4_sensor.last_update);
  Serial.printf("  H2S Sensor: %s (errors=%u, last_update=%lu ms ago)\n",
                diagnostics_get_sensor_status_string(systemDiagnostics.h2s_sensor.status),
                systemDiagnostics.h2s_sensor.consecutive_errors,
                millis() - systemDiagnostics.h2s_sensor.last_update);
  Serial.printf("  Water Sensor: %s (errors=%u, last_update=%lu ms ago)\n",
                diagnostics_get_sensor_status_string(systemDiagnostics.water_sensor.status),
                systemDiagnostics.water_sensor.consecutive_errors,
                millis() - systemDiagnostics.water_sensor.last_update);
  
  // WiFi quality
  Serial.println("\nWiFi Quality:");
  Serial.printf("  Connected: %s\n", systemDiagnostics.wifi.is_connected ? "Yes" : "No");
  Serial.printf("  RSSI: %d dBm\n", systemDiagnostics.wifi.rssi);
  Serial.printf("  Quality: %u%%\n", systemDiagnostics.wifi.quality_percent);
  Serial.printf("  Reconnections: %u\n", systemDiagnostics.wifi.reconnections);
  Serial.printf("  Total Downtime: %lu ms\n", systemDiagnostics.wifi.total_downtime_ms);
  
  // Memory health
  Serial.println("\nMemory Health:");
  Serial.printf("  Free Heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("  Min Free Heap: %u bytes\n", systemDiagnostics.min_free_heap);
  Serial.printf("  Heap Fragmentation: %u%%\n", systemDiagnostics.heap_fragmentation);
  
  // Fault counters
  Serial.println("\nFault Counters:");
  Serial.printf("  Brownout Events: %u\n", systemDiagnostics.brownout_count);
  Serial.printf("  Watchdog Resets: %u\n", systemDiagnostics.watchdog_resets);
  Serial.printf("  Panic Count: %u\n", systemDiagnostics.panic_count);
  
  // Fault flags
  Serial.println("\nActive Faults:");
  Serial.printf("  Brownout Detected: %s\n", systemDiagnostics.brownout_detected ? "YES" : "No");
  Serial.printf("  Low Memory Warning: %s\n", systemDiagnostics.low_memory_warning ? "YES" : "No");
  Serial.printf("  WiFi Quality Poor: %s\n", systemDiagnostics.wifi_quality_poor ? "YES" : "No");
  Serial.printf("  Sensor Fault: %s\n", systemDiagnostics.sensor_fault_detected ? "YES" : "No");
  
  Serial.println("================================\n");
}

bool diagnostics_to_json(char* buffer, size_t bufferSize) {
  if (!buffer || bufferSize < 512) {
    return false;
  }
  
  DynamicJsonDocument doc(1024);
  
  doc["overall_health"] = diagnostics_get_health_string(systemDiagnostics.overall_health);
  doc["uptime_ms"] = systemDiagnostics.uptime_ms;
  
  JsonObject sensors = doc.createNestedObject("sensors");
  sensors["ch4_status"] = diagnostics_get_sensor_status_string(systemDiagnostics.ch4_sensor.status);
  sensors["h2s_status"] = diagnostics_get_sensor_status_string(systemDiagnostics.h2s_sensor.status);
  sensors["water_status"] = diagnostics_get_sensor_status_string(systemDiagnostics.water_sensor.status);
  
  JsonObject wifi = doc.createNestedObject("wifi");
  wifi["connected"] = systemDiagnostics.wifi.is_connected;
  wifi["rssi"] = systemDiagnostics.wifi.rssi;
  wifi["quality_percent"] = systemDiagnostics.wifi.quality_percent;
  wifi["reconnections"] = systemDiagnostics.wifi.reconnections;
  
  JsonObject memory = doc.createNestedObject("memory");
  memory["free_heap"] = ESP.getFreeHeap();
  memory["min_free_heap"] = systemDiagnostics.min_free_heap;
  memory["fragmentation"] = systemDiagnostics.heap_fragmentation;
  
  JsonObject faults = doc.createNestedObject("faults");
  faults["brownout_count"] = systemDiagnostics.brownout_count;
  faults["watchdog_resets"] = systemDiagnostics.watchdog_resets;
  faults["low_memory"] = systemDiagnostics.low_memory_warning;
  faults["wifi_poor"] = systemDiagnostics.wifi_quality_poor;
  faults["sensor_fault"] = systemDiagnostics.sensor_fault_detected;
  
  size_t written = serializeJson(doc, buffer, bufferSize);
  return written > 0 && written < bufferSize;
}

void diagnostics_reset_counters() {
  systemDiagnostics.brownout_count = 0;
  systemDiagnostics.watchdog_resets = 0;
  systemDiagnostics.panic_count = 0;
  systemDiagnostics.wifi.reconnections = 0;
  systemDiagnostics.wifi.total_downtime_ms = 0;
  Serial.println("✅ Diagnostics counters reset");
}

bool diagnostics_has_critical_faults() {
  return (systemDiagnostics.overall_health == SYSTEM_HEALTH_CRITICAL ||
          systemDiagnostics.overall_health == SYSTEM_HEALTH_FAILURE);
}

void diagnostics_get_sensor_summary(char* buffer, size_t bufferSize) {
  if (!buffer || bufferSize < 200) {
    return;
  }
  
  snprintf(buffer, bufferSize,
    "Sensors: CH4=%s, H2S=%s, Water=%s",
    diagnostics_get_sensor_status_string(systemDiagnostics.ch4_sensor.status),
    diagnostics_get_sensor_status_string(systemDiagnostics.h2s_sensor.status),
    diagnostics_get_sensor_status_string(systemDiagnostics.water_sensor.status)
  );
}