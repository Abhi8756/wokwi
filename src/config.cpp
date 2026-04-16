#include "config.h"
#include <ArduinoJson.h>

// Global configuration instance
SystemConfig currentConfig;
Preferences preferences;

// Default configuration values
const SystemConfig DEFAULT_CONFIG = {
  CONFIG_VERSION,                 // version
  "MANHOLE_001",                  // device_id
  "location_001",                 // location_id
  "Wokwi-GUEST",                  // wifi_ssid
  "",                             // wifi_password
  "localhost",                    // mqtt_broker
  1883,                           // mqtt_port
  "",                             // mqtt_username
  "",                             // mqtt_password
  false,                          // mqtt_tls_enabled (TLS disabled by default for safety)
  true,                           // mqtt_tls_verify (Verify certificates when TLS enabled)
  8883,                           // mqtt_tls_port (Standard MQTT TLS port)
  0,                              // ntp_timezone_offset (UTC)
  1000,                           // sample_interval_ms
  1000.0f,                        // ch4_threshold_ppm
  10.0f,                          // h2s_threshold_ppm
  50.0f,                          // water_threshold_cm
  true,                           // buzzer_enabled
  true,                           // mqtt_enabled
  true,                           // ntp_enabled
  0                               // crc32
};

bool config_init() {
  Serial.println("🔧 Initializing configuration system...");
  
  // Open preferences in read-write mode
  if (!preferences.begin(CONFIG_NAMESPACE, false)) {
    Serial.println("❌ Failed to open preferences namespace");
    return false;
  }
  
  // Try to load existing configuration
  if (config_load()) {
    Serial.println("✅ Configuration loaded from NVS");
    config_print();
    return true;
  } else {
    Serial.println("⚠️  No valid configuration found, using defaults");
    config_load_defaults();
    
    // Save default configuration
    if (config_save()) {
      Serial.println("✅ Default configuration saved to NVS");
    } else {
      Serial.println("⚠️  Failed to save default configuration");
    }
    
    config_print();
    return true;
  }
}

bool config_load() {
  // Check if configuration exists
  if (!preferences.isKey(KEY_VERSION)) {
    Serial.println("⚠️  No configuration found in NVS");
    return false;
  }
  
  // Load configuration version
  currentConfig.version = preferences.getUChar(KEY_VERSION, CONFIG_VERSION);
  
  // Check version compatibility
  if (currentConfig.version != CONFIG_VERSION) {
    Serial.printf("⚠️  Configuration version mismatch (found %d, expected %d)\n", 
                 currentConfig.version, CONFIG_VERSION);
    return false;
  }
  
  // Load all configuration fields
  preferences.getString(KEY_DEVICE_ID, currentConfig.device_id, sizeof(currentConfig.device_id));
  preferences.getString(KEY_LOCATION_ID, currentConfig.location_id, sizeof(currentConfig.location_id));
  preferences.getString(KEY_WIFI_SSID, currentConfig.wifi_ssid, sizeof(currentConfig.wifi_ssid));
  preferences.getString(KEY_WIFI_PASSWORD, currentConfig.wifi_password, sizeof(currentConfig.wifi_password));
  preferences.getString(KEY_MQTT_BROKER, currentConfig.mqtt_broker, sizeof(currentConfig.mqtt_broker));
  currentConfig.mqtt_port = preferences.getUShort(KEY_MQTT_PORT, DEFAULT_CONFIG.mqtt_port);
  preferences.getString(KEY_MQTT_USER, currentConfig.mqtt_username, sizeof(currentConfig.mqtt_username));
  preferences.getString(KEY_MQTT_PASS, currentConfig.mqtt_password, sizeof(currentConfig.mqtt_password));
  currentConfig.mqtt_tls_enabled = preferences.getBool(KEY_MQTT_TLS_EN, DEFAULT_CONFIG.mqtt_tls_enabled);
  currentConfig.mqtt_tls_verify = preferences.getBool(KEY_MQTT_TLS_VERIFY, DEFAULT_CONFIG.mqtt_tls_verify);
  currentConfig.mqtt_tls_port = preferences.getUShort(KEY_MQTT_TLS_PORT, DEFAULT_CONFIG.mqtt_tls_port);
  currentConfig.ntp_timezone_offset = preferences.getShort(KEY_NTP_TIMEZONE, DEFAULT_CONFIG.ntp_timezone_offset);
  currentConfig.sample_interval_ms = preferences.getUShort(KEY_SAMPLE_INTERVAL, DEFAULT_CONFIG.sample_interval_ms);
  currentConfig.ch4_threshold_ppm = preferences.getFloat(KEY_CH4_THRESHOLD, DEFAULT_CONFIG.ch4_threshold_ppm);
  currentConfig.h2s_threshold_ppm = preferences.getFloat(KEY_H2S_THRESHOLD, DEFAULT_CONFIG.h2s_threshold_ppm);
  currentConfig.water_threshold_cm = preferences.getFloat(KEY_WATER_THRESHOLD, DEFAULT_CONFIG.water_threshold_cm);
  currentConfig.buzzer_enabled = preferences.getBool(KEY_BUZZER_ENABLED, DEFAULT_CONFIG.buzzer_enabled);
  currentConfig.mqtt_enabled = preferences.getBool(KEY_MQTT_ENABLED, DEFAULT_CONFIG.mqtt_enabled);
  currentConfig.ntp_enabled = preferences.getBool(KEY_NTP_ENABLED, DEFAULT_CONFIG.ntp_enabled);
  
  // Load stored CRC32
  currentConfig.crc32 = preferences.getUInt(CONFIG_CRC_KEY, 0);
  
  // Validate configuration
  if (!config_validate()) {
    Serial.println("❌ Configuration validation failed (CRC mismatch)");
    return false;
  }
  
  return true;
}

bool config_save() {
  Serial.println("💾 Saving configuration to NVS...");
  
  // Calculate CRC32 before saving
  currentConfig.crc32 = config_calculate_crc32(&currentConfig);
  
  // Save all configuration fields
  preferences.putUChar(KEY_VERSION, currentConfig.version);
  preferences.putString(KEY_DEVICE_ID, currentConfig.device_id);
  preferences.putString(KEY_LOCATION_ID, currentConfig.location_id);
  preferences.putString(KEY_WIFI_SSID, currentConfig.wifi_ssid);
  preferences.putString(KEY_WIFI_PASSWORD, currentConfig.wifi_password);
  preferences.putString(KEY_MQTT_BROKER, currentConfig.mqtt_broker);
  preferences.putUShort(KEY_MQTT_PORT, currentConfig.mqtt_port);
  preferences.putString(KEY_MQTT_USER, currentConfig.mqtt_username);
  preferences.putString(KEY_MQTT_PASS, currentConfig.mqtt_password);
  preferences.putBool(KEY_MQTT_TLS_EN, currentConfig.mqtt_tls_enabled);
  preferences.putBool(KEY_MQTT_TLS_VERIFY, currentConfig.mqtt_tls_verify);
  preferences.putUShort(KEY_MQTT_TLS_PORT, currentConfig.mqtt_tls_port);
  preferences.putShort(KEY_NTP_TIMEZONE, currentConfig.ntp_timezone_offset);
  preferences.putUShort(KEY_SAMPLE_INTERVAL, currentConfig.sample_interval_ms);
  preferences.putFloat(KEY_CH4_THRESHOLD, currentConfig.ch4_threshold_ppm);
  preferences.putFloat(KEY_H2S_THRESHOLD, currentConfig.h2s_threshold_ppm);
  preferences.putFloat(KEY_WATER_THRESHOLD, currentConfig.water_threshold_cm);
  preferences.putBool(KEY_BUZZER_ENABLED, currentConfig.buzzer_enabled);
  preferences.putBool(KEY_MQTT_ENABLED, currentConfig.mqtt_enabled);
  preferences.putBool(KEY_NTP_ENABLED, currentConfig.ntp_enabled);
  
  // Save CRC32
  preferences.putUInt(CONFIG_CRC_KEY, currentConfig.crc32);
  
  Serial.println("✅ Configuration saved successfully");
  Serial.printf("   CRC32: 0x%08X\n", currentConfig.crc32);
  
  return true;
}

bool config_factory_reset() {
  Serial.println("🏭 Performing factory reset...");
  
  // Clear all preferences
  if (!preferences.clear()) {
    Serial.println("❌ Failed to clear preferences");
    return false;
  }
  
  // Load default configuration
  config_load_defaults();
  
  // Save default configuration
  if (!config_save()) {
    Serial.println("❌ Failed to save default configuration");
    return false;
  }
  
  Serial.println("✅ Factory reset complete");
  config_print();
  
  return true;
}

bool config_validate() {
  // Calculate CRC32 of current configuration (excluding the CRC field itself)
  uint32_t calculated_crc = config_calculate_crc32(&currentConfig);
  
  if (calculated_crc != currentConfig.crc32) {
    Serial.printf("❌ CRC mismatch: calculated=0x%08X, stored=0x%08X\n", 
                 calculated_crc, currentConfig.crc32);
    return false;
  }
  
  return true;
}

uint32_t config_calculate_crc32(const SystemConfig* config) {
  if (!config) {
    return 0;
  }
  
  // Calculate CRC32 over all fields except the crc32 field itself
  size_t data_size = offsetof(SystemConfig, crc32);
  return crc32_buffer((const uint8_t*)config, data_size);
}

void config_load_defaults() {
  memcpy(&currentConfig, &DEFAULT_CONFIG, sizeof(SystemConfig));
  currentConfig.crc32 = config_calculate_crc32(&currentConfig);
}

void config_print() {
  Serial.println("\n--- Configuration ---");
  Serial.printf("Version: %d\n", currentConfig.version);
  Serial.printf("Device ID: %s\n", currentConfig.device_id);
  Serial.printf("Location ID: %s\n", currentConfig.location_id);
  Serial.printf("WiFi SSID: %s\n", currentConfig.wifi_ssid);
  Serial.printf("WiFi Password: %s\n", currentConfig.wifi_password[0] ? "***" : "(empty)");
  Serial.printf("MQTT Broker: %s:%d\n", currentConfig.mqtt_broker, currentConfig.mqtt_port);
  Serial.printf("MQTT Username: %s\n", currentConfig.mqtt_username[0] ? currentConfig.mqtt_username : "(none)");
  Serial.printf("MQTT Password: %s\n", currentConfig.mqtt_password[0] ? "***" : "(none)");
  Serial.printf("MQTT TLS: %s\n", currentConfig.mqtt_tls_enabled ? "Enabled" : "Disabled");
  Serial.printf("MQTT TLS Port: %d\n", currentConfig.mqtt_tls_port);
  Serial.printf("MQTT TLS Verify: %s\n", currentConfig.mqtt_tls_verify ? "Enabled" : "Disabled");
  Serial.printf("NTP Timezone: %d seconds\n", currentConfig.ntp_timezone_offset);
  Serial.printf("Sample Interval: %d ms\n", currentConfig.sample_interval_ms);
  Serial.printf("CH4 Threshold: %.1f ppm\n", currentConfig.ch4_threshold_ppm);
  Serial.printf("H2S Threshold: %.1f ppm\n", currentConfig.h2s_threshold_ppm);
  Serial.printf("Water Threshold: %.1f cm\n", currentConfig.water_threshold_cm);
  Serial.printf("Buzzer: %s\n", currentConfig.buzzer_enabled ? "Enabled" : "Disabled");
  Serial.printf("MQTT: %s\n", currentConfig.mqtt_enabled ? "Enabled" : "Disabled");
  Serial.printf("NTP: %s\n", currentConfig.ntp_enabled ? "Enabled" : "Disabled");
  Serial.printf("CRC32: 0x%08X\n", currentConfig.crc32);
  Serial.println("--------------------\n");
}

bool config_update_from_json(const char* json) {
  if (!json) {
    return false;
  }
  
  Serial.println("📝 Updating configuration from JSON...");
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, json);
  
  if (error) {
    Serial.printf("❌ JSON parsing failed: %s\n", error.c_str());
    return false;
  }
  
  bool changed = false;
  
  // Update fields if present in JSON
  if (doc.containsKey("device_id")) {
    strncpy(currentConfig.device_id, doc["device_id"], sizeof(currentConfig.device_id) - 1);
    changed = true;
  }
  
  if (doc.containsKey("location_id")) {
    strncpy(currentConfig.location_id, doc["location_id"], sizeof(currentConfig.location_id) - 1);
    changed = true;
  }
  
  if (doc.containsKey("wifi_ssid")) {
    strncpy(currentConfig.wifi_ssid, doc["wifi_ssid"], sizeof(currentConfig.wifi_ssid) - 1);
    changed = true;
  }
  
  if (doc.containsKey("wifi_password")) {
    strncpy(currentConfig.wifi_password, doc["wifi_password"], sizeof(currentConfig.wifi_password) - 1);
    changed = true;
  }
  
  if (doc.containsKey("mqtt_broker")) {
    strncpy(currentConfig.mqtt_broker, doc["mqtt_broker"], sizeof(currentConfig.mqtt_broker) - 1);
    changed = true;
  }
  
  if (doc.containsKey("mqtt_port")) {
    currentConfig.mqtt_port = doc["mqtt_port"];
    changed = true;
  }
  
  if (doc.containsKey("ch4_threshold")) {
    currentConfig.ch4_threshold_ppm = doc["ch4_threshold"];
    changed = true;
  }
  
  if (doc.containsKey("h2s_threshold")) {
    currentConfig.h2s_threshold_ppm = doc["h2s_threshold"];
    changed = true;
  }
  
  if (doc.containsKey("water_threshold")) {
    currentConfig.water_threshold_cm = doc["water_threshold"];
    changed = true;
  }
  
  if (doc.containsKey("buzzer_enabled")) {
    currentConfig.buzzer_enabled = doc["buzzer_enabled"];
    changed = true;
  }
  
  if (doc.containsKey("mqtt_enabled")) {
    currentConfig.mqtt_enabled = doc["mqtt_enabled"];
    changed = true;
  }
  
  if (doc.containsKey("ntp_enabled")) {
    currentConfig.ntp_enabled = doc["ntp_enabled"];
    changed = true;
  }
  
  if (changed) {
    Serial.println("✅ Configuration updated from JSON");
    config_print();
    return config_save();
  }
  
  Serial.println("⚠️  No configuration changes detected");
  return false;
}

bool config_to_json(char* buffer, size_t bufferSize) {
  if (!buffer || bufferSize < 512) {
    return false;
  }
  
  DynamicJsonDocument doc(1024);
  
  doc["version"] = currentConfig.version;
  doc["device_id"] = currentConfig.device_id;
  doc["location_id"] = currentConfig.location_id;
  doc["wifi_ssid"] = currentConfig.wifi_ssid;
  doc["mqtt_broker"] = currentConfig.mqtt_broker;
  doc["mqtt_port"] = currentConfig.mqtt_port;
  doc["ntp_timezone"] = currentConfig.ntp_timezone_offset;
  doc["sample_interval"] = currentConfig.sample_interval_ms;
  doc["ch4_threshold"] = currentConfig.ch4_threshold_ppm;
  doc["h2s_threshold"] = currentConfig.h2s_threshold_ppm;
  doc["water_threshold"] = currentConfig.water_threshold_cm;
  doc["buzzer_enabled"] = currentConfig.buzzer_enabled;
  doc["mqtt_enabled"] = currentConfig.mqtt_enabled;
  doc["ntp_enabled"] = currentConfig.ntp_enabled;
  doc["crc32"] = currentConfig.crc32;
  
  size_t written = serializeJson(doc, buffer, bufferSize);
  return written > 0 && written < bufferSize;
}

// Setter functions
bool config_set_device_id(const char* device_id) {
  if (!device_id) return false;
  strncpy(currentConfig.device_id, device_id, sizeof(currentConfig.device_id) - 1);
  return config_save();
}

bool config_set_location_id(const char* location_id) {
  if (!location_id) return false;
  strncpy(currentConfig.location_id, location_id, sizeof(currentConfig.location_id) - 1);
  return config_save();
}

bool config_set_wifi(const char* ssid, const char* password) {
  if (!ssid) return false;
  strncpy(currentConfig.wifi_ssid, ssid, sizeof(currentConfig.wifi_ssid) - 1);
  if (password) {
    strncpy(currentConfig.wifi_password, password, sizeof(currentConfig.wifi_password) - 1);
  }
  return config_save();
}

bool config_set_mqtt(const char* broker, uint16_t port, const char* username, const char* password) {
  if (!broker) return false;
  strncpy(currentConfig.mqtt_broker, broker, sizeof(currentConfig.mqtt_broker) - 1);
  currentConfig.mqtt_port = port;
  if (username) {
    strncpy(currentConfig.mqtt_username, username, sizeof(currentConfig.mqtt_username) - 1);
  }
  if (password) {
    strncpy(currentConfig.mqtt_password, password, sizeof(currentConfig.mqtt_password) - 1);
  }
  return config_save();
}

bool config_set_thresholds(float ch4_threshold, float h2s_threshold, float water_threshold) {
  currentConfig.ch4_threshold_ppm = ch4_threshold;
  currentConfig.h2s_threshold_ppm = h2s_threshold;
  currentConfig.water_threshold_cm = water_threshold;
  return config_save();
}

bool config_set_buzzer_enabled(bool enabled) {
  currentConfig.buzzer_enabled = enabled;
  return config_save();
}

bool config_set_mqtt_enabled(bool enabled) {
  currentConfig.mqtt_enabled = enabled;
  return config_save();
}

bool config_set_ntp_enabled(bool enabled) {
  currentConfig.ntp_enabled = enabled;
  return config_save();
}