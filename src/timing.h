#ifndef TIMING_H
#define TIMING_H

#include <Arduino.h>

/**
 * @file timing.h
 * @brief Non-blocking timing utilities for ESP32 firmware
 * 
 * This module provides utilities for non-blocking timing operations,
 * replacing blocking delay() calls with millis()-based timing.
 * 
 * Key Features:
 * - Handles millis() overflow (49.7 days)
 * - Multiple concurrent timers
 * - Microsecond precision available
 * - Zero CPU overhead when not triggered
 */

/**
 * @brief Check if a time interval has elapsed (non-blocking)
 * @param lastTime Reference to the last execution time (will be updated)
 * @param interval Interval in milliseconds
 * @return true if interval has elapsed, false otherwise
 * 
 * Example usage:
 *   unsigned long lastBlink = 0;
 *   if (intervalElapsed(lastBlink, 1000)) {
 *     digitalWrite(LED_PIN, !digitalRead(LED_PIN));
 *   }
 * 
 * Note: Handles millis() overflow automatically via unsigned arithmetic
 */
inline bool intervalElapsed(unsigned long &lastTime, unsigned long interval) {
  unsigned long currentTime = millis();
  if (currentTime - lastTime >= interval) {
    lastTime = currentTime;
    return true;
  }
  return false;
}

/**
 * @brief Get elapsed time since a reference point
 * @param startTime The reference time (from millis())
 * @return Elapsed milliseconds (handles overflow correctly)
 */
inline unsigned long getElapsedTime(unsigned long startTime) {
  return millis() - startTime;
}

/**
 * @brief Check if timeout has occurred without updating reference time
 * @param startTime The reference time
 * @param timeout Timeout in milliseconds
 * @return true if timeout occurred, false otherwise
 */
inline bool hasTimedOut(unsigned long startTime, unsigned long timeout) {
  return (millis() - startTime) >= timeout;
}

/**
 * @brief High-precision interval check using microseconds
 * @param lastTime Reference to last execution time in microseconds
 * @param interval Interval in microseconds
 * @return true if interval elapsed, false otherwise
 * 
 * Use for sub-millisecond timing (interrupt latency measurement, etc.)
 */
inline bool intervalElapsedMicros(unsigned long &lastTime, unsigned long interval) {
  unsigned long currentTime = micros();
  if (currentTime - lastTime >= interval) {
    lastTime = currentTime;
    return true;
  }
  return false;
}

/**
 * @brief Timer structure for managing multiple concurrent timers
 */
typedef struct {
  unsigned long lastTime;    // Last execution time
  unsigned long interval;    // Timer interval in ms
  bool enabled;              // Timer enable flag
  const char* name;          // Timer name for debugging
} Timer_t;

/**
 * @brief Initialize a timer structure
 * @param timer Pointer to timer structure
 * @param interval Timer interval in milliseconds
 * @param name Timer name for debugging (optional)
 */
inline void timerInit(Timer_t* timer, unsigned long interval, const char* name = nullptr) {
  timer->lastTime = millis();
  timer->interval = interval;
  timer->enabled = true;
  timer->name = name;
}

/**
 * @brief Check if timer has elapsed
 * @param timer Pointer to timer structure
 * @return true if timer elapsed, false otherwise
 */
inline bool timerElapsed(Timer_t* timer) {
  if (!timer->enabled) return false;
  
  unsigned long currentTime = millis();
  if (currentTime - timer->lastTime >= timer->interval) {
    timer->lastTime = currentTime;
    return true;
  }
  return false;
}

/**
 * @brief Reset timer to current time
 * @param timer Pointer to timer structure
 */
inline void timerReset(Timer_t* timer) {
  timer->lastTime = millis();
}

/**
 * @brief Enable/disable timer
 * @param timer Pointer to timer structure
 * @param enabled Enable state
 */
inline void timerSetEnabled(Timer_t* timer, bool enabled) {
  timer->enabled = enabled;
  if (enabled) {
    timer->lastTime = millis(); // Reset when re-enabled
  }
}

// Predefined common intervals (in milliseconds)
#define INTERVAL_1MS      1
#define INTERVAL_10MS     10
#define INTERVAL_50MS     50
#define INTERVAL_100MS    100
#define INTERVAL_500MS    500
#define INTERVAL_1SEC     1000
#define INTERVAL_5SEC     5000
#define INTERVAL_10SEC    10000
#define INTERVAL_1MIN     60000
#define INTERVAL_5MIN     300000
#define INTERVAL_1HOUR    3600000

#endif // TIMING_H