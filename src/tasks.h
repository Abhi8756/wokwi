#ifndef TASKS_H
#define TASKS_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

/**
 * @file tasks.h
 * @brief Dual-core task management for ESP32 IoT system
 * 
 * This module implements professional dual-core task architecture using FreeRTOS.
 * Separates sensor operations (Core 0) from communication tasks (Core 1) for
 * optimal performance and real-time responsiveness.
 * 
 * Key Features:
 * - Sensor task on Core 0 (dedicated to sensor reading and processing)
 * - Communication task on Core 1 (MQTT, WiFi, NTP)
 * - Inter-task communication via queues
 * - Mutex protection for shared resources
 * - Task monitoring and stack overflow detection
 * - Configurable task priorities and stack sizes
 */

// Task Configuration
#define SENSOR_TASK_STACK_SIZE 4096
#define COMM_TASK_STACK_SIZE 8192
#define MONITOR_TASK_STACK_SIZE 2048

#define SENSOR_TASK_PRIORITY 3      // High priority for real-time sensor reading
#define COMM_TASK_PRIORITY 2        // Medium priority for communication
#define MONITOR_TASK_PRIORITY 1     // Low priority for monitoring

#define SENSOR_TASK_CORE 0          // Core 0 for sensor operations
#define COMM_TASK_CORE 1            // Core 1 for communication
#define MONITOR_TASK_CORE 1         // Core 1 for monitoring

// Queue Configuration
#define SENSOR_QUEUE_SIZE 10        // Buffer 10 sensor readings
#define ALERT_QUEUE_SIZE 5          // Buffer 5 alert messages
#define COMMAND_QUEUE_SIZE 5        // Buffer 5 commands

// Task Timing Configuration
#define SENSOR_TASK_INTERVAL_MS 1000    // 1 second sensor reading
#define COMM_TASK_INTERVAL_MS 100       // 100ms communication check
#define MONITOR_TASK_INTERVAL_MS 10000  // 10 second monitoring

// Task States
typedef enum {
  TASK_STATE_STOPPED,
  TASK_STATE_STARTING,
  TASK_STATE_RUNNING,
  TASK_STATE_ERROR,
  TASK_STATE_SUSPENDED
} task_state_t;

// Sensor Data Structure for Inter-Task Communication
typedef struct {
  float ch4_ppm;
  float h2s_ppm;
  float distance_cm;
  float water_level_cm;
  bool alert_state;
  unsigned long timestamp;
  uint32_t sequence_number;
} SensorReading_t;

// Alert Message Structure
typedef struct {
  char alert_type[16];      // "CH4", "H2S", "FLOOD", "CLEAR"
  bool alert_active;
  float ch4_ppm;
  float h2s_ppm;
  float water_level_cm;
  unsigned long timestamp;
} AlertMessage_t;

// Command Structure
typedef struct {
  char command[32];
  char parameter[64];
  unsigned long timestamp;
} Command_t;

// Task Statistics
typedef struct {
  uint32_t executions;
  uint32_t max_execution_time_ms;
  uint32_t total_execution_time_ms;
  uint32_t stack_high_water_mark;
  task_state_t state;
  unsigned long last_execution;
  uint32_t errors;
} TaskStats_t;

// Global Task Handles
extern TaskHandle_t sensorTaskHandle;
extern TaskHandle_t commTaskHandle;
extern TaskHandle_t monitorTaskHandle;

// Global Queues
extern QueueHandle_t sensorQueue;
extern QueueHandle_t alertQueue;
extern QueueHandle_t commandQueue;

// Global Mutexes
extern SemaphoreHandle_t buzzerMutex;
extern SemaphoreHandle_t serialMutex;
extern SemaphoreHandle_t i2cMutex;

// Global Task Statistics
extern TaskStats_t sensorTaskStats;
extern TaskStats_t commTaskStats;
extern TaskStats_t monitorTaskStats;

/**
 * @brief Initialize dual-core task system
 * @return true if initialization successful, false otherwise
 */
bool tasks_init();

/**
 * @brief Start all tasks
 * @return true if all tasks started successfully, false otherwise
 */
bool tasks_start();

/**
 * @brief Stop all tasks gracefully
 */
void tasks_stop();

/**
 * @brief Get task system status
 * @param buffer Buffer to store status information
 * @param bufferSize Size of the buffer
 */
void tasks_get_status(char* buffer, size_t bufferSize);

/**
 * @brief Send sensor reading to communication task
 * @param reading Sensor reading data
 * @return true if sent successfully, false if queue full
 */
bool tasks_send_sensor_reading(const SensorReading_t* reading);

/**
 * @brief Send alert message to communication task
 * @param alert Alert message data
 * @return true if sent successfully, false if queue full
 */
bool tasks_send_alert(const AlertMessage_t* alert);

/**
 * @brief Send command to sensor task
 * @param command Command string
 * @param parameter Optional parameter
 * @return true if sent successfully, false if queue full
 */
bool tasks_send_command(const char* command, const char* parameter = nullptr);

/**
 * @brief Check if all tasks are running
 * @return true if all tasks are healthy, false otherwise
 */
bool tasks_are_healthy();

// Task Functions (implemented in tasks.cpp)
void sensorTask(void* parameter);
void commTask(void* parameter);
void monitorTask(void* parameter);

// Utility Functions
void tasks_update_stats(TaskStats_t* stats, uint32_t execution_time_ms);
const char* tasks_get_state_string(task_state_t state);
void tasks_print_stack_usage();

// Mutex Helper Functions
inline bool buzzer_lock(TickType_t timeout = portMAX_DELAY) {
  return xSemaphoreTake(buzzerMutex, timeout) == pdTRUE;
}

inline void buzzer_unlock() {
  xSemaphoreGive(buzzerMutex);
}

inline bool serial_lock(TickType_t timeout = portMAX_DELAY) {
  return xSemaphoreTake(serialMutex, timeout) == pdTRUE;
}

inline void serial_unlock() {
  xSemaphoreGive(serialMutex);
}

inline bool i2c_lock(TickType_t timeout = portMAX_DELAY) {
  return xSemaphoreTake(i2cMutex, timeout) == pdTRUE;
}

inline void i2c_unlock() {
  xSemaphoreGive(i2cMutex);
}

#endif // TASKS_H