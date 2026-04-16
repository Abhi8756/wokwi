#include <Arduino.h>
#include <WiFi.h>
#include "timing.h"
#include "watchdog.h"
#include "ntp.h"
#include "mqtt.h"
#include "mqtt_tls.h"
#include "tasks.h"
#include "config.h"
#include "ota.h"
#include "diagnostics.h"
#include "memory.h"
#include "performance.h"

// Pin Definitions
#define CH4_PIN 34
#define H2S_PIN 35
#define WATER_LEVEL_PIN 32
#define BUZZER_PIN 4

// System Configuration
bool alarmSilenced = false;
const int ADC_MAX = 1023;
const float CH4_MAX_PPM = 2000.0f;
const float H2S_MAX_PPM = 50.0f;
const float MANHOLE_DEPTH_CM = 100.0f;

// WiFi Configuration (REQ-012)
const char* WIFI_SSID = "Wokwi-GUEST";  // Default Wokwi WiFi
const char* WIFI_PASSWORD = "";         // No password for Wokwi

// ADC smoothing - keep last 5 readings for averaging
const int SMOOTH_COUNT = 5;
int ch4_buffer[SMOOTH_COUNT] = {0};
int h2s_buffer[SMOOTH_COUNT] = {0};
int water_level_buffer[SMOOTH_COUNT] = {0};
int ch4_index = 0;
int h2s_index = 0;
int water_level_index = 0;

// Non-blocking timing variables (now handled by tasks)
// Timer_t sensorTimer;
// Timer_t serialOutputTimer;
// Timer_t statusTimer;
// Timer_t heartbeatTimer;

// System state
unsigned long bootTime;
unsigned long loopCount = 0;
unsigned long maxLoopTime = 0;

// Shared sensor data structure (thread-safe for single-core access)
struct SensorData {
  float ch4_ppm;
  float h2s_ppm;
  float distance_cm;
  float water_level_cm;
  bool alert_state;
  unsigned long timestamp;
} currentSensorData = {0, 0, 0, 0, false, 0};

int smoothReading(int new_value, int *buffer, int &index) {
  buffer[index] = new_value;
  index = (index + 1) % SMOOTH_COUNT;

  int sum = 0;
  for (int i = 0; i < SMOOTH_COUNT; i++) {
    sum += buffer[i];
  }
  return sum / SMOOTH_COUNT;
}

float mapToRange(int raw_value, float max_output) {
  float normalized = static_cast<float>(raw_value) / static_cast<float>(ADC_MAX);
  normalized = constrain(normalized, 0.0f, 1.0f);
  return normalized * max_output;
}

void setup() {
  // Record boot time
  bootTime = millis();
  
  Serial.begin(115200);
  delay(1000); // Allow serial to initialize (only blocking delay allowed)

  Serial.println("\n=== Manhole Monitoring System v2.0 ===");
  Serial.println("Non-blocking firmware architecture enabled");
  
  // Initialize configuration system (REQ-009)
  if (!config_init()) {
    Serial.println("ERROR: Configuration initialization failed!");
    // Continue with defaults
  }
  
  // Print boot information and check for watchdog recovery (REQ-002)
  print_boot_info();
  
  analogReadResolution(10);
  Serial.println("Use the Wokwi sliders to control CH4, H2S, and water level.");

  pinMode(BUZZER_PIN, OUTPUT);

  // Initialize diagnostics system (REQ-015)
  if (!diagnostics_init()) {
    Serial.println("ERROR: Diagnostics initialization failed!");
  }
  
  // Initialize memory safety system (REQ-016)
  if (!memory_init()) {
    Serial.println("ERROR: Memory safety initialization failed!");
  }
  
  // Initialize performance monitoring (REQ-017)
  performance_init();
  Serial.println("📊 Performance monitoring initialized");
  
  // Enable heap poisoning (REQ-016)
  memory_enable_heap_poisoning();
  
  // Set memory corruption callback (REQ-016)
  memory_set_corruption_callback([]() {
    Serial.println("❌ CRITICAL: Memory corruption detected - system will reboot");
    delay(1000);
    ESP.restart();
  });
  
  // Perform sensor self-test (REQ-015)
  diagnostics_sensor_self_test(CH4_PIN, H2S_PIN, WATER_LEVEL_PIN);

  // Initialize WiFi for NTP synchronization (REQ-012)
  Serial.println("Connecting to WiFi...");
  WiFi.begin(currentConfig.wifi_ssid, currentConfig.wifi_password);
  
  unsigned long wifiStartTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStartTime < 10000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.printf("✅ WiFi connected to %s\n", currentConfig.wifi_ssid);
    Serial.printf("   IP address: %s\n", WiFi.localIP().toString().c_str());
    
    // Initialize NTP time synchronization (REQ-012) if enabled
    if (currentConfig.ntp_enabled) {
      if (ntp_init(currentConfig.ntp_timezone_offset, 0)) {
        Serial.println("🕐 NTP synchronization initiated");
      } else {
        Serial.println("⚠️  NTP initialization failed");
      }
    } else {
      Serial.println("⚠️  NTP disabled in configuration");
    }
    
    // Initialize MQTT communication (REQ-007) if enabled
    if (currentConfig.mqtt_enabled) {
      if (mqtt_init(currentConfig.device_id, currentConfig.location_id)) {
        Serial.println("📡 MQTT client initialized");
        if (mqtt_connect()) {
          Serial.println("📡 MQTT connected successfully");
        } else {
          Serial.println("⚠️  MQTT connection failed - will retry automatically");
        }
      } else {
        Serial.println("⚠️  MQTT initialization failed");
      }
    } else {
      Serial.println("⚠️  MQTT disabled in configuration");
    }
    
    // Initialize OTA update system (REQ-008)
    if (ota_init(currentConfig.device_id, OTA_PASSWORD)) {
      Serial.println("🔄 OTA update system initialized");
      
      // Set OTA callbacks for MQTT progress reporting
      ota_set_progress_callback([](uint8_t progress) {
        char progressMsg[128];
        snprintf(progressMsg, sizeof(progressMsg), 
                "{\"ota_progress\":%u,\"status\":\"updating\"}", progress);
        mqtt_publish_diagnostics(progressMsg);
      });
      
      ota_set_error_callback([](const char* error) {
        char errorMsg[256];
        snprintf(errorMsg, sizeof(errorMsg), 
                "{\"ota_error\":\"%s\",\"status\":\"failed\"}", error);
        mqtt_publish_diagnostics(errorMsg);
      });
      
      ota_set_complete_callback([]() {
        char completeMsg[128];
        snprintf(completeMsg, sizeof(completeMsg), 
                "{\"ota_status\":\"complete\",\"message\":\"Update successful, restarting...\"}");
        mqtt_publish_diagnostics(completeMsg);
      });
      
      // Check if firmware validation is pending
      if (ota_is_validation_pending()) {
        Serial.println("⚠️  Firmware validation pending - validating...");
        if (ota_validate_firmware()) {
          ota_mark_valid();
          Serial.println("✅ Firmware validated successfully");
          
          // Report successful update via MQTT
          char validMsg[128];
          snprintf(validMsg, sizeof(validMsg), 
                  "{\"ota_status\":\"validated\",\"boot_count\":%u}", ota_get_boot_count());
          mqtt_publish_diagnostics(validMsg);
        } else {
          Serial.println("❌ Firmware validation failed - rollback may occur");
        }
      }
    } else {
      Serial.println("⚠️  OTA initialization failed");
    }
  } else {
    Serial.println();
    Serial.println("⚠️  WiFi connection failed - NTP, MQTT, and OTA will not be available");
  }

  // Initialize non-blocking timers (now handled by tasks)
  // timerInit(&sensorTimer, INTERVAL_1SEC, "SensorReading");
  // timerInit(&serialOutputTimer, INTERVAL_1SEC, "SerialOutput");
  // timerInit(&statusTimer, INTERVAL_10SEC, "StatusReport");
  // timerInit(&heartbeatTimer, INTERVAL_500MS, "Heartbeat");

  // Offset serial output timer by 100ms to prevent simultaneous execution
  // serialOutputTimer.lastTime = millis() + 100;

  // Initialize ESP32 Task Watchdog Timer (REQ-002)
  Serial.println("Initializing watchdog timer...");
  if (!watchdog_init(WDT_TIMEOUT_SECONDS, true)) {
    Serial.println("ERROR: Watchdog initialization failed!");
  }

  // Initialize dual-core task system (REQ-010)
  Serial.println("Initializing dual-core task system...");
  if (!tasks_init()) {
    Serial.println("ERROR: Task system initialization failed!");
    return;
  }

  // Start all tasks (REQ-010)
  if (!tasks_start()) {
    Serial.println("ERROR: Failed to start tasks!");
    return;
  }

  Serial.println("Setup complete! Dual-core task system operational.");
  Serial.printf("Boot time: %lu ms\n", millis() - bootTime);
}

// Sensor reading function (non-blocking)
void readSensors() {
  int ch4_raw = smoothReading(analogRead(CH4_PIN), ch4_buffer, ch4_index);
  int h2s_raw = smoothReading(analogRead(H2S_PIN), h2s_buffer, h2s_index);
  int water_level_raw = smoothReading(
    analogRead(WATER_LEVEL_PIN),
    water_level_buffer,
    water_level_index
  );

  // Update diagnostics for sensor health (REQ-015)
  diagnostics_update_sensor(&systemDiagnostics.ch4_sensor, ch4_raw, 10, 1013);
  diagnostics_update_sensor(&systemDiagnostics.h2s_sensor, h2s_raw, 10, 1013);
  diagnostics_update_sensor(&systemDiagnostics.water_sensor, water_level_raw, 10, 1013);

  float ch4_ppm = mapToRange(ch4_raw, CH4_MAX_PPM);
  float h2s_ppm = mapToRange(h2s_raw, H2S_MAX_PPM);
  float water_level_cm = mapToRange(water_level_raw, MANHOLE_DEPTH_CM);
  float distance_cm = MANHOLE_DEPTH_CM - water_level_cm;

  // Threshold Logic: H2S > 10 ppm, CH4 > 1000 ppm, Water >= 50 cm
  bool ch4_alert = ch4_ppm > 1000;
  bool h2s_alert = h2s_ppm > 10;
  bool flood_alert = water_level_cm >= 50;
  bool alert_state = (ch4_alert || h2s_alert || flood_alert);

  // Check for alert state changes and publish MQTT alert (REQ-007)
  static bool previous_alert_state = false;
  if (alert_state != previous_alert_state) {
    char alertMessage[256];
    snprintf(alertMessage, sizeof(alertMessage),
      "{\"device_id\":\"MANHOLE_001\","
      "\"alert_type\":\"%s\","
      "\"alert_active\":%s,"
      "\"ch4_ppm\":%.2f,"
      "\"h2s_ppm\":%.2f,"
      "\"water_level_cm\":%.2f,"
      "\"timestamp\":%lu}",
      ch4_alert ? "CH4" : (h2s_alert ? "H2S" : (flood_alert ? "FLOOD" : "CLEAR")),
      alert_state ? "true" : "false",
      ch4_ppm,
      h2s_ppm,
      water_level_cm,
      millis()
    );
    
    mqtt_publish_alert(alertMessage);
    previous_alert_state = alert_state;
  }

  // Update buzzer state (immediate response)
  if (alert_state && !alarmSilenced) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    if (!alert_state) {
      alarmSilenced = false; // Reset silenced state
    }
  }

  // Update shared sensor data structure (atomic for single-core)
  currentSensorData.ch4_ppm = ch4_ppm;
  currentSensorData.h2s_ppm = h2s_ppm;
  currentSensorData.distance_cm = distance_cm;
  currentSensorData.water_level_cm = water_level_cm;
  currentSensorData.alert_state = alert_state;
  currentSensorData.timestamp = millis();
}

// Serial output function (non-blocking, separate from sensor reading)
void outputSensorData() {
  // Get NTP timestamp if available (REQ-012)
  char ntpTimestamp[32];
  bool hasNtpTime = ntp_get_iso_string(ntpTimestamp, sizeof(ntpTimestamp));
  
  // Build JSON payload for MQTT (REQ-007)
  char jsonPayload[512];
  snprintf(jsonPayload, sizeof(jsonPayload),
    "{\"device_id\":\"MANHOLE_001\","
    "\"ch4\":%.2f,"
    "\"h2s\":%.2f,"
    "\"water\":%.2f,"
    "\"waterLevel\":%.2f,"
    "\"alert\":%s,"
    "\"uptime\":%lu,"
    "\"loops\":%lu,"
    "\"maxLoopTime\":%lu,"
    "\"timestamp\":%lu"
    "%s%s%s"
    ",\"ntpStatus\":\"%s\""
    ",\"freeHeap\":%u}",
    currentSensorData.ch4_ppm,
    currentSensorData.h2s_ppm,
    currentSensorData.distance_cm,
    currentSensorData.water_level_cm,
    currentSensorData.alert_state ? "true" : "false",
    millis() - bootTime,
    loopCount,
    maxLoopTime,
    currentSensorData.timestamp,
    hasNtpTime ? ",\"ntpTime\":\"" : "",
    hasNtpTime ? ntpTimestamp : "",
    hasNtpTime ? "\"" : "",
    ntp_get_status_string(),
    ESP.getFreeHeap()
  );
  
  // Publish to MQTT (REQ-007)
  if (mqtt_is_connected()) {
    mqtt_publish_sensors(jsonPayload);
  }
  
  // Also output to serial for debugging
  Serial.println(jsonPayload);
}

// Status report function (every 10 seconds)
void reportStatus() {
  unsigned long uptime = millis() - bootTime;
  float uptimeHours = uptime / 3600000.0;
  float avgLoopTime = loopCount > 0 ? (float)uptime / loopCount : 0;
  
  Serial.println("\n--- System Status ---");
  Serial.printf("Uptime: %.2f hours (%lu ms)\n", uptimeHours, uptime);
  Serial.printf("Loop count: %lu\n", loopCount);
  Serial.printf("Max loop time: %lu ms\n", maxLoopTime);
  Serial.printf("Avg loop time: %.3f ms\n", avgLoopTime);
  Serial.printf("Loop frequency: %.1f Hz\n", loopCount > 0 ? 1000.0 * loopCount / uptime : 0);
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("CPU frequency: %u MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Watchdog timeout: %d seconds\n", WDT_TIMEOUT_SECONDS);
  Serial.printf("Last sensor reading: %lu ms ago\n", millis() - currentSensorData.timestamp);
  Serial.printf("Current readings - CH4: %.1f ppm, H2S: %.1f ppm, Water: %.1f cm\n", 
                currentSensorData.ch4_ppm, currentSensorData.h2s_ppm, currentSensorData.water_level_cm);
  Serial.printf("Alert state: %s\n", currentSensorData.alert_state ? "ACTIVE" : "Normal");
  
  // NTP status (REQ-012)
  char ntpStatus[300];
  ntp_get_status_info(ntpStatus, sizeof(ntpStatus));
  Serial.println(ntpStatus);
  
  // MQTT status (REQ-007)
  char mqttStatus[300];
  mqtt_get_status_info(mqttStatus, sizeof(mqttStatus));
  Serial.println(mqttStatus);
  
  // Task system status (REQ-010)
  char taskStatus[800];
  tasks_get_status(taskStatus, sizeof(taskStatus));
  Serial.println(taskStatus);
  
  // System diagnostics (REQ-015)
  diagnostics_update();  // Update diagnostics before printing
  diagnostics_print_report();
  
  // Memory safety report (REQ-016)
  memory_update_stats();  // Update memory stats before printing
  memory_print_report();
  
  // Performance metrics report (REQ-017)
  performance_update();  // Update performance metrics before printing
  performance_print_report();
  
  // Check heap integrity (REQ-016)
  if (!memory_check_heap_integrity()) {
    Serial.println("⚠️  WARNING: Heap integrity check failed!");
  }
  
  // Check for memory leaks (REQ-016)
  uint32_t leaks = memory_detect_leaks();
  if (leaks > 0) {
    Serial.printf("⚠️  WARNING: %u potential memory leaks detected\n", leaks);
  }
  
  Serial.println("Status: OPERATIONAL (Non-blocking)");
  Serial.println("--------------------\n");
  
  // Reset max loop time for next period
  maxLoopTime = 0;
}

// Heartbeat function (LED blink or status indicator)
void heartbeat() {
  static bool ledState = false;
  ledState = !ledState;
  // If you have an LED on GPIO 2, uncomment:
  // digitalWrite(2, ledState ? HIGH : LOW);
}

// Command processing function (non-blocking)
void processSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toUpperCase();
    
    unsigned long cmdStart = millis();
    
    if (command == "STATUS") {
      reportStatus();
    } else if (command == "PING") {
      Serial.printf("PONG - Response time: %lu ms\n", millis() - cmdStart);
    } else if (command == "BENCHMARK") {
      // Test command response time during sensor operations
      unsigned long benchStart = millis();
      for (int i = 0; i < 10; i++) {
        Serial.printf("Benchmark %d - Time: %lu ms\n", i, millis() - benchStart);
        yield(); // Allow other tasks to run
      }
      Serial.printf("Benchmark complete - Total time: %lu ms\n", millis() - benchStart);
    } else if (command == "RESET") {
      Serial.println("Resetting system...");
      ESP.restart();
    } else if (command == "SILENCE") {
      alarmSilenced = true;
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println("Alarm silenced");
    } else if (command == "UNSILENCE") {
      alarmSilenced = false;
      Serial.println("Alarm unsilenced");
    } else if (command == "HANG") {
      test_watchdog_hang("Manual watchdog test via HANG command");
    } else if (command == "WATCHDOG") {
      Serial.println(get_watchdog_status());
    } else if (command == "NTP") {
      char ntpStatus[300];
      ntp_get_status_info(ntpStatus, sizeof(ntpStatus));
      Serial.println(ntpStatus);
    } else if (command == "SYNCTIME") {
      if (ntp_force_sync()) {
        Serial.println("NTP synchronization initiated");
      } else {
        Serial.println("Failed to initiate NTP sync (check WiFi connection)");
      }
    } else if (command == "TIME") {
      char timeStr[32];
      if (ntp_get_iso_string(timeStr, sizeof(timeStr))) {
        Serial.printf("Current time: %s\n", timeStr);
      } else {
        Serial.println("Time not available (NTP not synchronized)");
      }
    } else if (command == "MQTT") {
      char mqttStatus[300];
      mqtt_get_status_info(mqttStatus, sizeof(mqttStatus));
      Serial.println(mqttStatus);
    } else if (command == "MQTTRECONNECT") {
      if (mqtt_force_reconnect()) {
        Serial.println("MQTT reconnection initiated");
      } else {
        Serial.println("Failed to initiate MQTT reconnection");
      }
    } else if (command == "MQTTTEST") {
      char testMessage[128];
      snprintf(testMessage, sizeof(testMessage), 
        "{\"test\":true,\"timestamp\":%lu,\"message\":\"Manual test message\"}", millis());
      if (mqtt_publish_sensors(testMessage)) {
        Serial.println("Test message published to MQTT");
      } else {
        Serial.println("Failed to publish test message (buffered if offline)");
      }
    } else if (command == "TLS") {
      mqtt_tls_print_status();
    } else if (command == "TLSTEST") {
      if (mqtt_tls_test_connection(currentConfig.mqtt_broker, currentConfig.mqtt_tls_port)) {
        Serial.println("TLS test connection successful");
      } else {
        Serial.println("TLS test connection failed");
      }
    } else if (command == "TLSINFO") {
      char tlsInfo[512];
      mqtt_tls_get_info(tlsInfo, sizeof(tlsInfo));
      Serial.println("TLS Configuration:");
      Serial.println(tlsInfo);
    } else if (command == "TASKS") {
      char taskStatus[800];
      tasks_get_status(taskStatus, sizeof(taskStatus));
      Serial.println(taskStatus);
    } else if (command == "TASKSTOP") {
      tasks_stop();
      Serial.println("All tasks stopped");
    } else if (command == "TASKSTART") {
      if (tasks_start()) {
        Serial.println("All tasks started");
      } else {
        Serial.println("Failed to start tasks");
      }
    } else if (command == "CONFIG") {
      config_print();
    } else if (command == "CONFIGSAVE") {
      if (config_save()) {
        Serial.println("Configuration saved");
      } else {
        Serial.println("Failed to save configuration");
      }
    } else if (command == "CONFIGRESET") {
      if (config_factory_reset()) {
        Serial.println("Factory reset complete - system will restart");
        delay(1000);
        ESP.restart();
      } else {
        Serial.println("Factory reset failed");
      }
    } else if (command == "CONFIGJSON") {
      char jsonBuffer[512];
      if (config_to_json(jsonBuffer, sizeof(jsonBuffer))) {
        Serial.println("Configuration as JSON:");
        Serial.println(jsonBuffer);
      } else {
        Serial.println("Failed to generate JSON");
      }
    } else if (command == "OTA") {
      char otaStatus[300];
      ota_get_status_info(otaStatus, sizeof(otaStatus));
      Serial.println(otaStatus);
    } else if (command == "OTAVALIDATE") {
      if (ota_validate_firmware()) {
        ota_mark_valid();
        Serial.println("Firmware validated and marked as valid");
      } else {
        Serial.println("Firmware validation failed");
      }
    } else if (command == "OTAROLLBACK") {
      if (ota_rollback()) {
        Serial.println("Rollback initiated - system will restart");
        delay(1000);
        ESP.restart();
      } else {
        Serial.println("Rollback failed or not available");
      }
    } else if (command == "OTAPARTITIONS") {
      char partInfo[256];
      ota_get_partition_info(partInfo, sizeof(partInfo));
      Serial.println(partInfo);
    } else if (command == "DIAG") {
      diagnostics_update();
      diagnostics_print_report();
    } else if (command == "DIAGRESET") {
      diagnostics_reset_counters();
      Serial.println("Diagnostics counters reset");
    } else if (command == "DIAGJSON") {
      char jsonBuffer[1024];
      if (diagnostics_to_json(jsonBuffer, sizeof(jsonBuffer))) {
        Serial.println("Diagnostics as JSON:");
        Serial.println(jsonBuffer);
      } else {
        Serial.println("Failed to generate diagnostics JSON");
      }
    } else if (command == "MEMORY") {
      memory_update_stats();
      memory_print_report();
    } else if (command == "MEMCHECK") {
      if (memory_check_heap_integrity()) {
        Serial.println("✅ Heap integrity check passed");
      } else {
        Serial.println("❌ Heap integrity check FAILED");
      }
    } else if (command == "MEMLEAK") {
      uint32_t leaks = memory_detect_leaks();
      if (leaks == 0) {
        Serial.println("✅ No memory leaks detected");
      } else {
        Serial.printf("⚠️  %u potential memory leaks detected\n", leaks);
      }
    } else if (command == "MEMRESET") {
      memory_reset_stats();
      Serial.println("Memory statistics reset");
    } else if (command == "MEMJSON") {
      char jsonBuffer[1024];
      if (memory_stats_to_json(jsonBuffer, sizeof(jsonBuffer))) {
        Serial.println("Memory statistics as JSON:");
        Serial.println(jsonBuffer);
      } else {
        Serial.println("Failed to generate memory JSON");
      }
    } else if (command == "MEMPOOL") {
      char poolStats[256];
      memory_get_pool_stats(poolStats, sizeof(poolStats));
      Serial.println(poolStats);
    } else if (command == "PERF") {
      performance_update();
      performance_print_report();
    } else if (command == "PERFRESET") {
      performance_reset();
      Serial.println("Performance metrics reset");
    } else if (command == "PERFJSON") {
      char jsonBuffer[512];
      if (performance_to_json(jsonBuffer, sizeof(jsonBuffer))) {
        Serial.println("Performance metrics as JSON:");
        Serial.println(jsonBuffer);
      } else {
        Serial.println("Failed to generate performance JSON");
      }
    } else if (command == "HELP") {
      Serial.println("Available commands:");
      Serial.println("  STATUS        - Show system status");
      Serial.println("  PING          - Test response time");
      Serial.println("  BENCHMARK     - Test response during operations");
      Serial.println("  RESET         - Restart system");
      Serial.println("  SILENCE       - Silence alarm");
      Serial.println("  UNSILENCE     - Enable alarm");
      Serial.println("  WATCHDOG      - Show watchdog status");
      Serial.println("  NTP           - Show NTP status");
      Serial.println("  SYNCTIME      - Force NTP synchronization");
      Serial.println("  TIME          - Show current time");
      Serial.println("  MQTT          - Show MQTT status");
      Serial.println("  MQTTRECONNECT - Force MQTT reconnection");
      Serial.println("  MQTTTEST      - Send test message via MQTT");
      Serial.println("  TLS           - Show TLS/SSL status");
      Serial.println("  TLSTEST       - Test TLS connection to broker");
      Serial.println("  TLSINFO       - Show TLS configuration details");
      Serial.println("  TASKS         - Show task system status");
      Serial.println("  TASKSTOP      - Stop all tasks");
      Serial.println("  TASKSTART     - Start all tasks");
      Serial.println("  CONFIG        - Show current configuration");
      Serial.println("  CONFIGSAVE    - Save configuration to NVS");
      Serial.println("  CONFIGRESET   - Factory reset (restart required)");
      Serial.println("  CONFIGJSON    - Show configuration as JSON");
      Serial.println("  OTA           - Show OTA status");
      Serial.println("  OTAVALIDATE   - Validate and mark firmware as valid");
      Serial.println("  OTAROLLBACK   - Rollback to previous firmware");
      Serial.println("  OTAPARTITIONS - Show partition information");
      Serial.println("  DIAG          - Show system diagnostics");
      Serial.println("  DIAGRESET     - Reset diagnostics counters");
      Serial.println("  DIAGJSON      - Show diagnostics as JSON");
      Serial.println("  MEMORY        - Show memory statistics");
      Serial.println("  MEMCHECK      - Check heap integrity");
      Serial.println("  MEMLEAK       - Detect memory leaks");
      Serial.println("  MEMRESET      - Reset memory statistics");
      Serial.println("  MEMJSON       - Show memory stats as JSON");
      Serial.println("  MEMPOOL       - Show memory pool usage");
      Serial.println("  PERF          - Show performance metrics");
      Serial.println("  PERFRESET     - Reset performance metrics");
      Serial.println("  PERFJSON      - Show performance as JSON");
      Serial.println("  HANG          - Test watchdog (causes reboot)");
      Serial.println("  HELP          - Show this help");
    } else if (command.length() > 0) {
      Serial.printf("Unknown command: %s (type HELP for commands)\n", command.c_str());
    }
    
    unsigned long cmdTime = millis() - cmdStart;
    if (cmdTime > 10) {
      Serial.printf("Warning: Command took %lu ms\n", cmdTime);
    }
  }
}

void loop() {
  // Simple main loop - most work is now done by FreeRTOS tasks (REQ-010)
  
  // Performance monitoring (REQ-017) - non-invasive measurement
  unsigned long loopStartMicros = micros();
  
  // Handle OTA updates (REQ-008)
  ota_handle();
  
  // Check task health every 5 seconds
  static unsigned long lastHealthCheck = 0;
  if (millis() - lastHealthCheck > 5000) {
    if (!tasks_are_healthy()) {
      Serial.println("⚠️  Task system unhealthy - attempting restart");
      tasks_stop();
      delay(1000);
      if (!tasks_start()) {
        Serial.println("❌ Failed to restart tasks - system may be unstable");
      }
    }
    lastHealthCheck = millis();
  }
  
  // Reset watchdog timer to prevent system reboot (REQ-002)
  watchdog_reset();
  
  // Record loop performance (REQ-017) - at end of loop
  unsigned long loopTimeMicros = micros() - loopStartMicros;
  performance_record_loop_time(loopTimeMicros);
  
  // Small delay to prevent watchdog issues and allow tasks to run
  delay(100);
}
