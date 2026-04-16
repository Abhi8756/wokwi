#ifndef MEMORY_H
#define MEMORY_H

#include <Arduino.h>
#include <esp_heap_caps.h>
#include <esp_system.h>

/**
 * @file memory.h
 * @brief Memory safety and corruption detection for ESP32 IoT system
 * 
 * This module provides comprehensive memory safety features including:
 * - Stack canary monitoring for all FreeRTOS tasks
 * - Heap poisoning for use-after-free detection
 * - Stack high-water mark monitoring
 * - Heap integrity checks
 * - Memory pool allocator for fixed-size allocations
 * 
 * Key Features:
 * - Automatic stack overflow detection
 * - Heap corruption detection with automatic reboot
 * - Memory usage monitoring and reporting
 * - Static allocation for critical structures
 */

// Memory pool configuration
#define MEMORY_POOL_SMALL_SIZE 32      // Small allocations (32 bytes)
#define MEMORY_POOL_MEDIUM_SIZE 128    // Medium allocations (128 bytes)
#define MEMORY_POOL_LARGE_SIZE 512     // Large allocations (512 bytes)

#define MEMORY_POOL_SMALL_COUNT 20     // 20 small blocks
#define MEMORY_POOL_MEDIUM_COUNT 10    // 10 medium blocks
#define MEMORY_POOL_LARGE_COUNT 5      // 5 large blocks

// Heap poisoning patterns
#define HEAP_POISON_PATTERN_ALLOC 0xA5  // Pattern for allocated memory
#define HEAP_POISON_PATTERN_FREE 0x5A   // Pattern for freed memory

// Memory health thresholds
#define MEMORY_WARNING_THRESHOLD 20000   // Warn if free heap < 20KB
#define MEMORY_CRITICAL_THRESHOLD 10000  // Critical if free heap < 10KB

// Memory statistics structure
struct MemoryStats {
  // Heap statistics
  uint32_t total_heap_size;
  uint32_t free_heap;
  uint32_t min_free_heap;
  uint32_t largest_free_block;
  uint32_t heap_fragmentation_percent;
  
  // Allocation statistics
  uint32_t total_allocations;
  uint32_t total_frees;
  uint32_t current_allocations;
  uint32_t failed_allocations;
  
  // Pool statistics
  uint32_t pool_small_used;
  uint32_t pool_medium_used;
  uint32_t pool_large_used;
  uint32_t pool_small_peak;
  uint32_t pool_medium_peak;
  uint32_t pool_large_peak;
  
  // Corruption detection
  uint32_t heap_corruption_count;
  uint32_t stack_overflow_count;
  bool heap_integrity_ok;
  
  // Last update
  unsigned long last_update;
};

// Global memory statistics
extern MemoryStats memoryStats;

/**
 * @brief Initialize memory safety system
 * @return true if initialization successful, false otherwise
 */
bool memory_init();

/**
 * @brief Enable heap poisoning for use-after-free detection
 * @return true if successful, false otherwise
 */
bool memory_enable_heap_poisoning();

/**
 * @brief Check heap integrity
 * @return true if heap is intact, false if corruption detected
 */
bool memory_check_heap_integrity();

/**
 * @brief Update memory statistics
 * Should be called regularly (e.g., every second)
 */
void memory_update_stats();

/**
 * @brief Get memory statistics
 * @param stats Pointer to MemoryStats structure to fill
 */
void memory_get_stats(MemoryStats* stats);

/**
 * @brief Print memory statistics report
 */
void memory_print_report();

/**
 * @brief Get memory statistics as JSON string
 * @param buffer Buffer to store JSON string
 * @param bufferSize Size of the buffer
 * @return true if successful, false otherwise
 */
bool memory_stats_to_json(char* buffer, size_t bufferSize);

/**
 * @brief Check if memory is in warning state
 * @return true if free heap below warning threshold
 */
bool memory_is_warning();

/**
 * @brief Check if memory is in critical state
 * @return true if free heap below critical threshold
 */
bool memory_is_critical();

/**
 * @brief Get memory health status string
 * @return Human-readable memory health string
 */
const char* memory_get_health_string();

/**
 * @brief Allocate memory from pool (if available) or heap
 * @param size Size of memory to allocate
 * @return Pointer to allocated memory, or nullptr if failed
 */
void* memory_pool_alloc(size_t size);

/**
 * @brief Free memory allocated from pool or heap
 * @param ptr Pointer to memory to free
 */
void memory_pool_free(void* ptr);

/**
 * @brief Get pool usage statistics
 * @param buffer Buffer to store statistics string
 * @param bufferSize Size of the buffer
 */
void memory_get_pool_stats(char* buffer, size_t bufferSize);

/**
 * @brief Reset memory statistics counters
 */
void memory_reset_stats();

/**
 * @brief Perform memory leak detection
 * Compares current allocations with baseline
 * @return Number of potential leaks detected
 */
uint32_t memory_detect_leaks();

/**
 * @brief Set memory corruption callback
 * Called when heap corruption is detected
 */
typedef void (*memory_corruption_callback_t)();
extern memory_corruption_callback_t memory_corruption_callback;

inline void memory_set_corruption_callback(memory_corruption_callback_t callback) {
  memory_corruption_callback = callback;
}

/**
 * @brief Dump memory information for debugging
 * @param address Starting address to dump
 * @param length Number of bytes to dump
 */
void memory_dump(void* address, size_t length);

#endif // MEMORY_H
