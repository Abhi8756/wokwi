#include "tasks.h"
#include "timing.h"
#include "watchdog.h"
#include "ntp.h"
#include "mqtt.h"
#include "diagnostics.h"

// Forward declaration for SensorData structure
struct SensorData {
  float ch4_ppm;
  float h2s_ppm;
  float distance_cm;
  float water_level_cm;
  bool alert_state;
  unsigned long timestamp;
};

// External sensor functions (from main.cpp)
extern void readSensors();
extern void processSerialCommands();
extern struct SensorData currentSensorData;
extern bool alarmSilenced;

// Pin definitions (from main.cpp)
#define BUZZER_PIN 4

// Global Task Handles
TaskHandle_t sensorTaskHandle = nullptr;
TaskHandle_t commTaskHandle = nullptr;
TaskHandle_t monitorTaskHandle = nullptr;

// Global Queues
QueueHandle_t sensorQueue = nullptr;
QueueHandle_t alertQueue = nullptr;
QueueHandle_t commandQueue = nullptr;

// Global Mutexes
SemaphoreHandle_t buzzerMutex = nullptr;
SemaphoreHandle_t serialMutex = nullptr;
SemaphoreHandle_t i2cMutex = nullptr;

// Global Task Statistics
TaskStats_t sensorTaskStats = {0};
TaskStats_t commTaskStats = {0};
TaskStats_t monitorTaskStats = {0};

// Sequence counter for sensor readings
static uint32_t sensorSequenceNumber = 0;

bool tasks_init() {
  Serial.println("🔧 Initializing dual-core task system...");
  
  // Create queues
  sensorQueue = xQueueCreate(SENSOR_QUEUE_SIZE, sizeof(SensorReading_t));
  alertQueue = xQueueCreate(ALERT_QUEUE_SIZE, sizeof(AlertMessage_t));
  commandQueue = xQueueCreate(COMMAND_QUEUE_SIZE, sizeof(Command_t));
  
  if (!sensorQueue || !alertQueue || !commandQueue) {
    Serial.println("❌ Failed to create task queues");
    return false;
  }
  
  // Create mutexes
  buzzerMutex = xSemaphoreCreateMutex();
  serialMutex = xSemaphoreCreateMutex();
  i2cMutex = xSemaphoreCreateMutex();
  
  if (!buzzerMutex || !serialMutex || !i2cMutex) {
    Serial.println("❌ Failed to create mutexes");
    return false;
  }
  
  // Initialize task statistics
  sensorTaskStats.state = TASK_STATE_STOPPED;
  commTaskStats.state = TASK_STATE_STOPPED;
  monitorTaskStats.state = TASK_STATE_STOPPED;
  
  Serial.println("✅ Task system initialized successfully");
  Serial.printf("   Sensor queue: %d slots\n", SENSOR_QUEUE_SIZE);
  Serial.printf("   Alert queue: %d slots\n", ALERT_QUEUE_SIZE);
  Serial.printf("   Command queue: %d slots\n", COMMAND_QUEUE_SIZE);
  
  return true;
}

bool tasks_start() {
  Serial.println("🚀 Starting dual-core tasks...");
  
  // Create sensor task on Core 0
  BaseType_t result = xTaskCreatePinnedToCore(
    sensorTask,
    "SensorTask",
    SENSOR_TASK_STACK_SIZE,
    nullptr,
    SENSOR_TASK_PRIORITY,
    &sensorTaskHandle,
    SENSOR_TASK_CORE
  );
  
  if (result != pdPASS) {
    Serial.println("❌ Failed to create sensor task");
    return false;
  }
  
  // Create communication task on Core 1
  result = xTaskCreatePinnedToCore(
    commTask,
    "CommTask",
    COMM_TASK_STACK_SIZE,
    nullptr,
    COMM_TASK_PRIORITY,
    &commTaskHandle,
    COMM_TASK_CORE
  );
  
  if (result != pdPASS) {
    Serial.println("❌ Failed to create communication task");
    return false;
  }
  
  // Create monitoring task on Core 1
  result = xTaskCreatePinnedToCore(
    monitorTask,
    "MonitorTask",
    MONITOR_TASK_STACK_SIZE,
    nullptr,
    MONITOR_TASK_PRIORITY,
    &monitorTaskHandle,
    MONITOR_TASK_CORE
  );
  
  if (result != pdPASS) {
    Serial.println("❌ Failed to create monitor task");
    return false;
  }
  
  Serial.println("✅ All tasks started successfully");
  Serial.printf("   Sensor task: Core %d, Priority %d\n", SENSOR_TASK_CORE, SENSOR_TASK_PRIORITY);
  Serial.printf("   Communication task: Core %d, Priority %d\n", COMM_TASK_CORE, COMM_TASK_PRIORITY);
  Serial.printf("   Monitor task: Core %d, Priority %d\n", MONITOR_TASK_CORE, MONITOR_TASK_PRIORITY);
  
  return true;
}

void tasks_stop() {
  Serial.println("🛑 Stopping all tasks...");
  
  if (sensorTaskHandle) {
    vTaskDelete(sensorTaskHandle);
    sensorTaskHandle = nullptr;
  }
  
  if (commTaskHandle) {
    vTaskDelete(commTaskHandle);
    commTaskHandle = nullptr;
  }
  
  if (monitorTaskHandle) {
    vTaskDelete(monitorTaskHandle);
    monitorTaskHandle = nullptr;
  }
  
  Serial.println("✅ All tasks stopped");
}

void sensorTask(void* parameter) {
  Serial.println("📊 Sensor task started on Core 0");
  sensorTaskStats.state = TASK_STATE_RUNNING;
  
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t taskInterval = pdMS_TO_TICKS(SENSOR_TASK_INTERVAL_MS);
  
  while (true) {
    uint32_t startTime = millis();
    sensorTaskStats.executions++;
    sensorTaskStats.last_execution = startTime;
    
    try {
      // Read sensors (this function is from main.cpp)
      readSensors();
      
      // Create sensor reading message
      SensorReading_t reading;
      reading.ch4_ppm = currentSensorData.ch4_ppm;
      reading.h2s_ppm = currentSensorData.h2s_ppm;
      reading.distance_cm = currentSensorData.distance_cm;
      reading.water_level_cm = currentSensorData.water_level_cm;
      reading.alert_state = currentSensorData.alert_state;
      reading.timestamp = currentSensorData.timestamp;
      reading.sequence_number = ++sensorSequenceNumber;
      
      // Send to communication task
      if (!tasks_send_sensor_reading(&reading)) {
        Serial.println("⚠️  Sensor queue full, dropping reading");
      }
      
      // Check for alert state changes
      static bool previousAlertState = false;
      if (reading.alert_state != previousAlertState) {
        AlertMessage_t alert;
        strncpy(alert.alert_type, 
                reading.ch4_ppm > 1000 ? "CH4" : 
                (reading.h2s_ppm > 10 ? "H2S" : 
                (reading.water_level_cm >= 50 ? "FLOOD" : "CLEAR")), 
                sizeof(alert.alert_type) - 1);
        alert.alert_active = reading.alert_state;
        alert.ch4_ppm = reading.ch4_ppm;
        alert.h2s_ppm = reading.h2s_ppm;
        alert.water_level_cm = reading.water_level_cm;
        alert.timestamp = reading.timestamp;
        
        tasks_send_alert(&alert);
        previousAlertState = reading.alert_state;
      }
      
      // Handle buzzer (with mutex protection)
      if (buzzer_lock(pdMS_TO_TICKS(10))) {
        if (reading.alert_state && !alarmSilenced) {
          digitalWrite(BUZZER_PIN, HIGH);
        } else {
          digitalWrite(BUZZER_PIN, LOW);
        }
        buzzer_unlock();
      }
      
      // Process commands from queue
      Command_t command;
      while (xQueueReceive(commandQueue, &command, 0) == pdTRUE) {
        // Handle sensor-specific commands
        if (strcmp(command.command, "SILENCE") == 0) {
          alarmSilenced = true;
        } else if (strcmp(command.command, "UNSILENCE") == 0) {
          alarmSilenced = false;
        }
      }
      
    } catch (...) {
      sensorTaskStats.errors++;
      Serial.println("❌ Error in sensor task");
    }
    
    // Update task statistics
    uint32_t executionTime = millis() - startTime;
    tasks_update_stats(&sensorTaskStats, executionTime);
    
    // Reset watchdog for this task
    watchdog_reset();
    
    // Wait for next interval
    vTaskDelayUntil(&lastWakeTime, taskInterval);
  }
}

void commTask(void* parameter) {
  Serial.println("📡 Communication task started on Core 1");
  commTaskStats.state = TASK_STATE_RUNNING;
  
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t taskInterval = pdMS_TO_TICKS(COMM_TASK_INTERVAL_MS);
  
  // Diagnostics update tracking
  unsigned long lastDiagnosticsUpdate = 0;
  const unsigned long DIAGNOSTICS_UPDATE_INTERVAL = 60000; // 60 seconds
  
  // Memory monitoring tracking
  unsigned long lastMemoryCheck = 0;
  const unsigned long MEMORY_CHECK_INTERVAL = 30000; // 30 seconds
  
  while (true) {
    uint32_t startTime = millis();
    commTaskStats.executions++;
    commTaskStats.last_execution = startTime;
    
    try {
      // Process sensor readings from queue
      SensorReading_t reading;
      while (xQueueReceive(sensorQueue, &reading, 0) == pdTRUE) {
        // Build JSON payload for MQTT
        char jsonPayload[512];
        snprintf(jsonPayload, sizeof(jsonPayload),
          "{\"device_id\":\"MANHOLE_001\","
          "\"ch4\":%.2f,"
          "\"h2s\":%.2f,"
          "\"water\":%.2f,"
          "\"waterLevel\":%.2f,"
          "\"alert\":%s,"
          "\"timestamp\":%lu,"
          "\"sequence\":%u}",
          reading.ch4_ppm,
          reading.h2s_ppm,
          reading.distance_cm,
          reading.water_level_cm,
          reading.alert_state ? "true" : "false",
          reading.timestamp,
          reading.sequence_number
        );
        
        // Publish to MQTT
        if (mqtt_is_connected()) {
          mqtt_publish_sensors(jsonPayload);
        }
        
        // Also output to serial (with mutex protection)
        if (serial_lock(pdMS_TO_TICKS(10))) {
          Serial.println(jsonPayload);
          serial_unlock();
        }
      }
      
      // Process alert messages from queue
      AlertMessage_t alert;
      while (xQueueReceive(alertQueue, &alert, 0) == pdTRUE) {
        char alertPayload[256];
        snprintf(alertPayload, sizeof(alertPayload),
          "{\"device_id\":\"MANHOLE_001\","
          "\"alert_type\":\"%s\","
          "\"alert_active\":%s,"
          "\"ch4_ppm\":%.2f,"
          "\"h2s_ppm\":%.2f,"
          "\"water_level_cm\":%.2f,"
          "\"timestamp\":%lu}",
          alert.alert_type,
          alert.alert_active ? "true" : "false",
          alert.ch4_ppm,
          alert.h2s_ppm,
          alert.water_level_cm,
          alert.timestamp
        );
        
        // Publish alert to MQTT
        if (mqtt_is_connected()) {
          mqtt_publish_alert(alertPayload);
        }
      }
      
      // Update MQTT connection
      mqtt_update();
      
      // Update NTP synchronization
      ntp_update();
      
      // Periodic diagnostics update and MQTT publishing
      unsigned long currentTime = millis();
      if (currentTime - lastDiagnosticsUpdate >= DIAGNOSTICS_UPDATE_INTERVAL) {
        // Update diagnostics
        extern void diagnostics_update();
        diagnostics_update();
        
        // Publish diagnostics to MQTT
        char diagJson[1024];
        extern bool diagnostics_to_json(char* buffer, size_t bufferSize);
        if (diagnostics_to_json(diagJson, sizeof(diagJson))) {
          if (mqtt_is_connected()) {
            mqtt_publish_diagnostics(diagJson);
          }
        }
        
        lastDiagnosticsUpdate = currentTime;
      }
      
      // Periodic memory monitoring and integrity checks (REQ-016)
      if (currentTime - lastMemoryCheck >= MEMORY_CHECK_INTERVAL) {
        // Update memory statistics
        extern void memory_update_stats();
        memory_update_stats();
        
        // Check heap integrity
        extern bool memory_check_heap_integrity();
        if (!memory_check_heap_integrity()) {
          if (serial_lock(pdMS_TO_TICKS(10))) {
            Serial.println("⚠️  WARNING: Heap integrity check failed in comm task");
            serial_unlock();
          }
        }
        
        // Publish memory stats to MQTT
        char memJson[1024];
        extern bool memory_stats_to_json(char* buffer, size_t bufferSize);
        if (memory_stats_to_json(memJson, sizeof(memJson))) {
          if (mqtt_is_connected()) {
            mqtt_publish_diagnostics(memJson);
          }
        }
        
        lastMemoryCheck = currentTime;
      }
      
      // Process serial commands (with mutex protection)
      if (serial_lock(pdMS_TO_TICKS(5))) {
        processSerialCommands();
        serial_unlock();
      }
      
    } catch (...) {
      commTaskStats.errors++;
      Serial.println("❌ Error in communication task");
    }
    
    // Update task statistics
    uint32_t executionTime = millis() - startTime;
    tasks_update_stats(&commTaskStats, executionTime);
    
    // Wait for next interval
    vTaskDelayUntil(&lastWakeTime, taskInterval);
  }
}

void monitorTask(void* parameter) {
  Serial.println("🔍 Monitor task started on Core 1");
  monitorTaskStats.state = TASK_STATE_RUNNING;
  
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t taskInterval = pdMS_TO_TICKS(MONITOR_TASK_INTERVAL_MS);
  
  while (true) {
    uint32_t startTime = millis();
    monitorTaskStats.executions++;
    monitorTaskStats.last_execution = startTime;
    
    try {
      // Update stack high water marks
      if (sensorTaskHandle) {
        sensorTaskStats.stack_high_water_mark = uxTaskGetStackHighWaterMark(sensorTaskHandle);
      }
      if (commTaskHandle) {
        commTaskStats.stack_high_water_mark = uxTaskGetStackHighWaterMark(commTaskHandle);
      }
      if (monitorTaskHandle) {
        monitorTaskStats.stack_high_water_mark = uxTaskGetStackHighWaterMark(monitorTaskHandle);
      }
      
      // Check for stack overflow warnings
      if (sensorTaskStats.stack_high_water_mark < 512) {
        Serial.printf("⚠️  Sensor task stack low: %u bytes remaining\n", 
                     sensorTaskStats.stack_high_water_mark);
      }
      if (commTaskStats.stack_high_water_mark < 1024) {
        Serial.printf("⚠️  Communication task stack low: %u bytes remaining\n", 
                     commTaskStats.stack_high_water_mark);
      }
      
      // Print task statistics (with serial mutex)
      if (serial_lock(pdMS_TO_TICKS(50))) {
        tasks_print_stack_usage();
        serial_unlock();
      }
      
    } catch (...) {
      monitorTaskStats.errors++;
      Serial.println("❌ Error in monitor task");
    }
    
    // Update task statistics
    uint32_t executionTime = millis() - startTime;
    tasks_update_stats(&monitorTaskStats, executionTime);
    
    // Wait for next interval
    vTaskDelayUntil(&lastWakeTime, taskInterval);
  }
}

bool tasks_send_sensor_reading(const SensorReading_t* reading) {
  if (!reading || !sensorQueue) {
    return false;
  }
  
  return xQueueSend(sensorQueue, reading, 0) == pdTRUE;
}

bool tasks_send_alert(const AlertMessage_t* alert) {
  if (!alert || !alertQueue) {
    return false;
  }
  
  return xQueueSend(alertQueue, alert, 0) == pdTRUE;
}

bool tasks_send_command(const char* command, const char* parameter) {
  if (!command || !commandQueue) {
    return false;
  }
  
  Command_t cmd;
  strncpy(cmd.command, command, sizeof(cmd.command) - 1);
  if (parameter) {
    strncpy(cmd.parameter, parameter, sizeof(cmd.parameter) - 1);
  } else {
    cmd.parameter[0] = '\0';
  }
  cmd.timestamp = millis();
  
  return xQueueSend(commandQueue, &cmd, 0) == pdTRUE;
}

bool tasks_are_healthy() {
  return (sensorTaskStats.state == TASK_STATE_RUNNING &&
          commTaskStats.state == TASK_STATE_RUNNING &&
          monitorTaskStats.state == TASK_STATE_RUNNING);
}

void tasks_update_stats(TaskStats_t* stats, uint32_t execution_time_ms) {
  if (!stats) return;
  
  stats->total_execution_time_ms += execution_time_ms;
  if (execution_time_ms > stats->max_execution_time_ms) {
    stats->max_execution_time_ms = execution_time_ms;
  }
}

const char* tasks_get_state_string(task_state_t state) {
  switch (state) {
    case TASK_STATE_STOPPED: return "Stopped";
    case TASK_STATE_STARTING: return "Starting";
    case TASK_STATE_RUNNING: return "Running";
    case TASK_STATE_ERROR: return "Error";
    case TASK_STATE_SUSPENDED: return "Suspended";
    default: return "Unknown";
  }
}

void tasks_print_stack_usage() {
  Serial.println("\n--- Task Stack Usage ---");
  Serial.printf("Sensor Task: %u bytes remaining (%.1f%% used)\n",
                sensorTaskStats.stack_high_water_mark,
                100.0 * (SENSOR_TASK_STACK_SIZE - sensorTaskStats.stack_high_water_mark) / SENSOR_TASK_STACK_SIZE);
  Serial.printf("Comm Task: %u bytes remaining (%.1f%% used)\n",
                commTaskStats.stack_high_water_mark,
                100.0 * (COMM_TASK_STACK_SIZE - commTaskStats.stack_high_water_mark) / COMM_TASK_STACK_SIZE);
  Serial.printf("Monitor Task: %u bytes remaining (%.1f%% used)\n",
                monitorTaskStats.stack_high_water_mark,
                100.0 * (MONITOR_TASK_STACK_SIZE - monitorTaskStats.stack_high_water_mark) / MONITOR_TASK_STACK_SIZE);
  Serial.println("------------------------\n");
}

void tasks_get_status(char* buffer, size_t bufferSize) {
  if (!buffer || bufferSize < 500) {
    return;
  }
  
  snprintf(buffer, bufferSize,
    "Task System Status:\n"
    "  Sensor Task: %s (Core %d)\n"
    "    Executions: %u, Errors: %u\n"
    "    Max exec time: %u ms\n"
    "    Stack remaining: %u bytes\n"
    "  Communication Task: %s (Core %d)\n"
    "    Executions: %u, Errors: %u\n"
    "    Max exec time: %u ms\n"
    "    Stack remaining: %u bytes\n"
    "  Monitor Task: %s (Core %d)\n"
    "    Executions: %u, Errors: %u\n"
    "    Max exec time: %u ms\n"
    "    Stack remaining: %u bytes\n"
    "  Queue Status:\n"
    "    Sensor queue: %d/%d\n"
    "    Alert queue: %d/%d\n"
    "    Command queue: %d/%d",
    tasks_get_state_string(sensorTaskStats.state), SENSOR_TASK_CORE,
    sensorTaskStats.executions, sensorTaskStats.errors,
    sensorTaskStats.max_execution_time_ms,
    sensorTaskStats.stack_high_water_mark,
    tasks_get_state_string(commTaskStats.state), COMM_TASK_CORE,
    commTaskStats.executions, commTaskStats.errors,
    commTaskStats.max_execution_time_ms,
    commTaskStats.stack_high_water_mark,
    tasks_get_state_string(monitorTaskStats.state), MONITOR_TASK_CORE,
    monitorTaskStats.executions, monitorTaskStats.errors,
    monitorTaskStats.max_execution_time_ms,
    monitorTaskStats.stack_high_water_mark,
    sensorQueue ? (SENSOR_QUEUE_SIZE - uxQueueSpacesAvailable(sensorQueue)) : 0, SENSOR_QUEUE_SIZE,
    alertQueue ? (ALERT_QUEUE_SIZE - uxQueueSpacesAvailable(alertQueue)) : 0, ALERT_QUEUE_SIZE,
    commandQueue ? (COMMAND_QUEUE_SIZE - uxQueueSpacesAvailable(commandQueue)) : 0, COMMAND_QUEUE_SIZE
  );
}