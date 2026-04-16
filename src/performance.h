#ifndef PERFORMANCE_H
#define PERFORMANCE_H

#include <Arduino.h>

/**
 * @file performance.h
 * @brief Lightweight performance monitoring for ESP32 IoT system
 * 
 * This module provides non-invasive performance metrics collection:
 * - Task execution time measurement
 * - CPU utilization tracking
 * - MQTT publish latency monitoring
 * - Loop performance statistics
 * 
 * SAFETY: This module is READ-ONLY and does not modify system behavior.
 * It only observes and reports metrics.
 */

// Performance metrics structure
struct PerformanceMetrics {
  // Loop performance
  uint32_t loop_count;
  uint32_t min_loop_time_us;
  uint32_t max_loop_time_us;
  uint32_t avg_loop_time_us;
  
  // Task execution times (microseconds)
  uint32_t sensor_task_time_us;
  uint32_t comm_task_time_us;
  uint32_t monitor_task_time_us;
  
  // CPU utilization (percentage)
  float cpu_utilization_percent;
  
  // MQTT performance
  uint32_t mqtt_publish_count;
  uint32_t mqtt_publish_failures;
  uint32_t mqtt_avg_latency_ms;
  
  // Memory performance
  uint32_t heap_alloc_count;
  uint32_t heap_free_count;
  
  // Timing
  unsigned long measurement_start_time;
  unsigned long last_update;
};

// Global performance metrics
extern PerformanceMetrics perfMetrics;

/**
 * @brief Initialize performance monitoring system
 * SAFETY: Does not modify any system behavior
 */
inline void performance_init() {
  perfMetrics.loop_count = 0;
  perfMetrics.min_loop_time_us = 0xFFFFFFFF;  // Start with max value
  perfMetrics.max_loop_time_us = 0;
  perfMetrics.avg_loop_time_us = 0;
  
  perfMetrics.sensor_task_time_us = 0;
  perfMetrics.comm_task_time_us = 0;
  perfMetrics.monitor_task_time_us = 0;
  
  perfMetrics.cpu_utilization_percent = 0.0f;
  
  perfMetrics.mqtt_publish_count = 0;
  perfMetrics.mqtt_publish_failures = 0;
  perfMetrics.mqtt_avg_latency_ms = 0;
  
  perfMetrics.heap_alloc_count = 0;
  perfMetrics.heap_free_count = 0;
  
  perfMetrics.measurement_start_time = millis();
  perfMetrics.last_update = millis();
}

/**
 * @brief Record loop execution time
 * SAFETY: Only updates metrics, no side effects
 * @param loop_time_us Loop execution time in microseconds
 */
inline void performance_record_loop_time(uint32_t loop_time_us) {
  perfMetrics.loop_count++;
  
  if (loop_time_us < perfMetrics.min_loop_time_us) {
    perfMetrics.min_loop_time_us = loop_time_us;
  }
  
  if (loop_time_us > perfMetrics.max_loop_time_us) {
    perfMetrics.max_loop_time_us = loop_time_us;
  }
  
  // Calculate running average
  perfMetrics.avg_loop_time_us = 
    (perfMetrics.avg_loop_time_us * (perfMetrics.loop_count - 1) + loop_time_us) / 
    perfMetrics.loop_count;
}

/**
 * @brief Record task execution time
 * SAFETY: Only updates metrics, no side effects
 */
inline void performance_record_task_time(const char* task_name, uint32_t time_us) {
  if (strcmp(task_name, "sensor") == 0) {
    perfMetrics.sensor_task_time_us = time_us;
  } else if (strcmp(task_name, "comm") == 0) {
    perfMetrics.comm_task_time_us = time_us;
  } else if (strcmp(task_name, "monitor") == 0) {
    perfMetrics.monitor_task_time_us = time_us;
  }
}

/**
 * @brief Calculate CPU utilization
 * SAFETY: Read-only calculation
 */
inline void performance_calculate_cpu_utilization() {
  unsigned long elapsed_ms = millis() - perfMetrics.measurement_start_time;
  
  if (elapsed_ms > 0 && perfMetrics.loop_count > 0) {
    // Total time spent in loops
    uint32_t total_loop_time_ms = (perfMetrics.avg_loop_time_us * perfMetrics.loop_count) / 1000;
    
    // CPU utilization = (time spent working / total time) * 100
    perfMetrics.cpu_utilization_percent = 
      (float)total_loop_time_ms / (float)elapsed_ms * 100.0f;
    
    // Cap at 100%
    if (perfMetrics.cpu_utilization_percent > 100.0f) {
      perfMetrics.cpu_utilization_percent = 100.0f;
    }
  }
}

/**
 * @brief Record MQTT publish event
 * SAFETY: Only updates counters
 */
inline void performance_record_mqtt_publish(bool success, uint32_t latency_ms) {
  if (success) {
    perfMetrics.mqtt_publish_count++;
    
    // Update average latency
    if (perfMetrics.mqtt_publish_count == 1) {
      perfMetrics.mqtt_avg_latency_ms = latency_ms;
    } else {
      perfMetrics.mqtt_avg_latency_ms = 
        (perfMetrics.mqtt_avg_latency_ms * (perfMetrics.mqtt_publish_count - 1) + latency_ms) / 
        perfMetrics.mqtt_publish_count;
    }
  } else {
    perfMetrics.mqtt_publish_failures++;
  }
}

/**
 * @brief Update performance metrics
 * SAFETY: Only recalculates metrics
 */
inline void performance_update() {
  performance_calculate_cpu_utilization();
  perfMetrics.last_update = millis();
}

/**
 * @brief Print performance report
 * SAFETY: Read-only, only prints to serial
 */
inline void performance_print_report() {
  Serial.println("\n=== Performance Metrics ===");
  
  // Loop performance
  Serial.println("\nLoop Performance:");
  Serial.printf("  Total loops: %u\n", perfMetrics.loop_count);
  Serial.printf("  Min loop time: %u μs\n", perfMetrics.min_loop_time_us);
  Serial.printf("  Max loop time: %u μs\n", perfMetrics.max_loop_time_us);
  Serial.printf("  Avg loop time: %u μs\n", perfMetrics.avg_loop_time_us);
  Serial.printf("  Loop frequency: %.1f Hz\n", 
                perfMetrics.avg_loop_time_us > 0 ? 1000000.0f / perfMetrics.avg_loop_time_us : 0);
  
  // CPU utilization
  Serial.println("\nCPU Utilization:");
  Serial.printf("  CPU usage: %.2f%%\n", perfMetrics.cpu_utilization_percent);
  Serial.printf("  CPU idle: %.2f%%\n", 100.0f - perfMetrics.cpu_utilization_percent);
  
  // Task performance
  Serial.println("\nTask Execution Times:");
  Serial.printf("  Sensor task: %u μs\n", perfMetrics.sensor_task_time_us);
  Serial.printf("  Communication task: %u μs\n", perfMetrics.comm_task_time_us);
  Serial.printf("  Monitor task: %u μs\n", perfMetrics.monitor_task_time_us);
  
  // MQTT performance
  Serial.println("\nMQTT Performance:");
  Serial.printf("  Publishes: %u\n", perfMetrics.mqtt_publish_count);
  Serial.printf("  Failures: %u\n", perfMetrics.mqtt_publish_failures);
  Serial.printf("  Success rate: %.1f%%\n", 
                perfMetrics.mqtt_publish_count > 0 ? 
                (float)(perfMetrics.mqtt_publish_count - perfMetrics.mqtt_publish_failures) / 
                perfMetrics.mqtt_publish_count * 100.0f : 0);
  Serial.printf("  Avg latency: %u ms\n", perfMetrics.mqtt_avg_latency_ms);
  
  // Uptime
  unsigned long uptime_ms = millis() - perfMetrics.measurement_start_time;
  Serial.println("\nUptime:");
  Serial.printf("  Total: %.2f hours\n", uptime_ms / 3600000.0f);
  
  Serial.println("==========================\n");
}

/**
 * @brief Get performance metrics as JSON
 * SAFETY: Read-only, only formats data
 */
inline bool performance_to_json(char* buffer, size_t bufferSize) {
  if (!buffer || bufferSize < 256) {
    return false;
  }
  
  snprintf(buffer, bufferSize,
    "{\"loop_count\":%u,"
    "\"min_loop_us\":%u,"
    "\"max_loop_us\":%u,"
    "\"avg_loop_us\":%u,"
    "\"cpu_percent\":%.2f,"
    "\"mqtt_publishes\":%u,"
    "\"mqtt_failures\":%u,"
    "\"mqtt_latency_ms\":%u,"
    "\"uptime_ms\":%lu}",
    perfMetrics.loop_count,
    perfMetrics.min_loop_time_us,
    perfMetrics.max_loop_time_us,
    perfMetrics.avg_loop_time_us,
    perfMetrics.cpu_utilization_percent,
    perfMetrics.mqtt_publish_count,
    perfMetrics.mqtt_publish_failures,
    perfMetrics.mqtt_avg_latency_ms,
    millis() - perfMetrics.measurement_start_time
  );
  
  return true;
}

/**
 * @brief Reset performance counters
 * SAFETY: Only resets metrics, doesn't affect system operation
 */
inline void performance_reset() {
  performance_init();
  Serial.println("✅ Performance metrics reset");
}

#endif // PERFORMANCE_H
