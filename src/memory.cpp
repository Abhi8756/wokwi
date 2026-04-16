#include "memory.h"
#include <ArduinoJson.h>

// Global memory statistics
MemoryStats memoryStats;

// Memory corruption callback
memory_corruption_callback_t memory_corruption_callback = nullptr;

// Memory pool structures
struct MemoryBlock {
  bool in_use;
  void* data;
  size_t size;
  uint32_t magic;  // Magic number for corruption detection
};

#define MEMORY_MAGIC 0xDEADBEEF

// Memory pools
static MemoryBlock smallPool[MEMORY_POOL_SMALL_COUNT];
static MemoryBlock mediumPool[MEMORY_POOL_MEDIUM_COUNT];
static MemoryBlock largePool[MEMORY_POOL_LARGE_COUNT];

// Pool data storage
static uint8_t smallPoolData[MEMORY_POOL_SMALL_COUNT][MEMORY_POOL_SMALL_SIZE];
static uint8_t mediumPoolData[MEMORY_POOL_MEDIUM_COUNT][MEMORY_POOL_MEDIUM_SIZE];
static uint8_t largePoolData[MEMORY_POOL_LARGE_COUNT][MEMORY_POOL_LARGE_SIZE];

bool memory_init() {
  Serial.println("🧠 Initializing memory safety system...");
  
  // Initialize memory statistics
  memoryStats.total_heap_size = ESP.getHeapSize();
  memoryStats.free_heap = ESP.getFreeHeap();
  memoryStats.min_free_heap = memoryStats.free_heap;
  memoryStats.largest_free_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  memoryStats.heap_fragmentation_percent = 0;
  
  memoryStats.total_allocations = 0;
  memoryStats.total_frees = 0;
  memoryStats.current_allocations = 0;
  memoryStats.failed_allocations = 0;
  
  memoryStats.pool_small_used = 0;
  memoryStats.pool_medium_used = 0;
  memoryStats.pool_large_used = 0;
  memoryStats.pool_small_peak = 0;
  memoryStats.pool_medium_peak = 0;
  memoryStats.pool_large_peak = 0;
  
  memoryStats.heap_corruption_count = 0;
  memoryStats.stack_overflow_count = 0;
  memoryStats.heap_integrity_ok = true;
  
  memoryStats.last_update = millis();
  
  // Initialize small pool
  for (int i = 0; i < MEMORY_POOL_SMALL_COUNT; i++) {
    smallPool[i].in_use = false;
    smallPool[i].data = smallPoolData[i];
    smallPool[i].size = MEMORY_POOL_SMALL_SIZE;
    smallPool[i].magic = MEMORY_MAGIC;
  }
  
  // Initialize medium pool
  for (int i = 0; i < MEMORY_POOL_MEDIUM_COUNT; i++) {
    mediumPool[i].in_use = false;
    mediumPool[i].data = mediumPoolData[i];
    mediumPool[i].size = MEMORY_POOL_MEDIUM_SIZE;
    mediumPool[i].magic = MEMORY_MAGIC;
  }
  
  // Initialize large pool
  for (int i = 0; i < MEMORY_POOL_LARGE_COUNT; i++) {
    largePool[i].in_use = false;
    largePool[i].data = largePoolData[i];
    largePool[i].size = MEMORY_POOL_LARGE_SIZE;
    largePool[i].magic = MEMORY_MAGIC;
  }
  
  Serial.println("✅ Memory safety system initialized");
  Serial.printf("   Total heap: %u bytes\n", memoryStats.total_heap_size);
  Serial.printf("   Free heap: %u bytes\n", memoryStats.free_heap);
  Serial.printf("   Small pool: %d blocks x %d bytes\n", MEMORY_POOL_SMALL_COUNT, MEMORY_POOL_SMALL_SIZE);
  Serial.printf("   Medium pool: %d blocks x %d bytes\n", MEMORY_POOL_MEDIUM_COUNT, MEMORY_POOL_MEDIUM_SIZE);
  Serial.printf("   Large pool: %d blocks x %d bytes\n", MEMORY_POOL_LARGE_COUNT, MEMORY_POOL_LARGE_SIZE);
  
  return true;
}

bool memory_enable_heap_poisoning() {
  // ESP32 heap poisoning is enabled via menuconfig
  // This function serves as a placeholder for future enhancements
  Serial.println("ℹ️  Heap poisoning: Enable via menuconfig (CONFIG_HEAP_POISONING_COMPREHENSIVE)");
  return true;
}

bool memory_check_heap_integrity() {
  // Check memory pool magic numbers
  bool integrity_ok = true;
  
  // Check small pool
  for (int i = 0; i < MEMORY_POOL_SMALL_COUNT; i++) {
    if (smallPool[i].magic != MEMORY_MAGIC) {
      Serial.printf("❌ Small pool corruption detected at block %d\n", i);
      integrity_ok = false;
      memoryStats.heap_corruption_count++;
    }
  }
  
  // Check medium pool
  for (int i = 0; i < MEMORY_POOL_MEDIUM_COUNT; i++) {
    if (mediumPool[i].magic != MEMORY_MAGIC) {
      Serial.printf("❌ Medium pool corruption detected at block %d\n", i);
      integrity_ok = false;
      memoryStats.heap_corruption_count++;
    }
  }
  
  // Check large pool
  for (int i = 0; i < MEMORY_POOL_LARGE_COUNT; i++) {
    if (largePool[i].magic != MEMORY_MAGIC) {
      Serial.printf("❌ Large pool corruption detected at block %d\n", i);
      integrity_ok = false;
      memoryStats.heap_corruption_count++;
    }
  }
  
  // Check ESP32 heap integrity
  if (!heap_caps_check_integrity_all(true)) {
    Serial.println("❌ ESP32 heap corruption detected");
    integrity_ok = false;
    memoryStats.heap_corruption_count++;
  }
  
  memoryStats.heap_integrity_ok = integrity_ok;
  
  if (!integrity_ok && memory_corruption_callback) {
    memory_corruption_callback();
  }
  
  return integrity_ok;
}

void memory_update_stats() {
  // Update heap statistics
  memoryStats.free_heap = ESP.getFreeHeap();
  memoryStats.largest_free_block = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  
  // Track minimum free heap
  if (memoryStats.free_heap < memoryStats.min_free_heap) {
    memoryStats.min_free_heap = memoryStats.free_heap;
  }
  
  // Calculate heap fragmentation
  if (memoryStats.free_heap > 0) {
    memoryStats.heap_fragmentation_percent = 
      100 - ((memoryStats.largest_free_block * 100) / memoryStats.free_heap);
  } else {
    memoryStats.heap_fragmentation_percent = 100;
  }
  
  // Update pool usage
  uint32_t small_used = 0, medium_used = 0, large_used = 0;
  
  for (int i = 0; i < MEMORY_POOL_SMALL_COUNT; i++) {
    if (smallPool[i].in_use) small_used++;
  }
  for (int i = 0; i < MEMORY_POOL_MEDIUM_COUNT; i++) {
    if (mediumPool[i].in_use) medium_used++;
  }
  for (int i = 0; i < MEMORY_POOL_LARGE_COUNT; i++) {
    if (largePool[i].in_use) large_used++;
  }
  
  memoryStats.pool_small_used = small_used;
  memoryStats.pool_medium_used = medium_used;
  memoryStats.pool_large_used = large_used;
  
  // Track peak usage
  if (small_used > memoryStats.pool_small_peak) {
    memoryStats.pool_small_peak = small_used;
  }
  if (medium_used > memoryStats.pool_medium_peak) {
    memoryStats.pool_medium_peak = medium_used;
  }
  if (large_used > memoryStats.pool_large_peak) {
    memoryStats.pool_large_peak = large_used;
  }
  
  memoryStats.last_update = millis();
}

void memory_get_stats(MemoryStats* stats) {
  if (!stats) return;
  *stats = memoryStats;
}

void memory_print_report() {
  Serial.println("\n=== Memory Safety Report ===");
  
  // Heap statistics
  Serial.println("\nHeap Statistics:");
  Serial.printf("  Total Heap: %u bytes\n", memoryStats.total_heap_size);
  Serial.printf("  Free Heap: %u bytes (%.1f%%)\n", 
                memoryStats.free_heap,
                (memoryStats.free_heap * 100.0) / memoryStats.total_heap_size);
  Serial.printf("  Min Free Heap: %u bytes\n", memoryStats.min_free_heap);
  Serial.printf("  Largest Free Block: %u bytes\n", memoryStats.largest_free_block);
  Serial.printf("  Fragmentation: %u%%\n", memoryStats.heap_fragmentation_percent);
  
  // Allocation statistics
  Serial.println("\nAllocation Statistics:");
  Serial.printf("  Total Allocations: %u\n", memoryStats.total_allocations);
  Serial.printf("  Total Frees: %u\n", memoryStats.total_frees);
  Serial.printf("  Current Allocations: %u\n", memoryStats.current_allocations);
  Serial.printf("  Failed Allocations: %u\n", memoryStats.failed_allocations);
  
  // Pool statistics
  Serial.println("\nMemory Pool Usage:");
  Serial.printf("  Small Pool (%d bytes): %u/%d used (peak: %u)\n",
                MEMORY_POOL_SMALL_SIZE,
                memoryStats.pool_small_used,
                MEMORY_POOL_SMALL_COUNT,
                memoryStats.pool_small_peak);
  Serial.printf("  Medium Pool (%d bytes): %u/%d used (peak: %u)\n",
                MEMORY_POOL_MEDIUM_SIZE,
                memoryStats.pool_medium_used,
                MEMORY_POOL_MEDIUM_COUNT,
                memoryStats.pool_medium_peak);
  Serial.printf("  Large Pool (%d bytes): %u/%d used (peak: %u)\n",
                MEMORY_POOL_LARGE_SIZE,
                memoryStats.pool_large_used,
                MEMORY_POOL_LARGE_COUNT,
                memoryStats.pool_large_peak);
  
  // Corruption detection
  Serial.println("\nCorruption Detection:");
  Serial.printf("  Heap Integrity: %s\n", memoryStats.heap_integrity_ok ? "OK" : "CORRUPTED");
  Serial.printf("  Heap Corruption Count: %u\n", memoryStats.heap_corruption_count);
  Serial.printf("  Stack Overflow Count: %u\n", memoryStats.stack_overflow_count);
  
  // Memory health
  Serial.println("\nMemory Health:");
  Serial.printf("  Status: %s\n", memory_get_health_string());
  
  Serial.println("============================\n");
}

bool memory_stats_to_json(char* buffer, size_t bufferSize) {
  if (!buffer || bufferSize < 512) {
    return false;
  }
  
  DynamicJsonDocument doc(1024);
  
  JsonObject heap = doc.createNestedObject("heap");
  heap["total"] = memoryStats.total_heap_size;
  heap["free"] = memoryStats.free_heap;
  heap["min_free"] = memoryStats.min_free_heap;
  heap["largest_block"] = memoryStats.largest_free_block;
  heap["fragmentation"] = memoryStats.heap_fragmentation_percent;
  
  JsonObject alloc = doc.createNestedObject("allocations");
  alloc["total"] = memoryStats.total_allocations;
  alloc["frees"] = memoryStats.total_frees;
  alloc["current"] = memoryStats.current_allocations;
  alloc["failed"] = memoryStats.failed_allocations;
  
  JsonObject pools = doc.createNestedObject("pools");
  pools["small_used"] = memoryStats.pool_small_used;
  pools["medium_used"] = memoryStats.pool_medium_used;
  pools["large_used"] = memoryStats.pool_large_used;
  pools["small_peak"] = memoryStats.pool_small_peak;
  pools["medium_peak"] = memoryStats.pool_medium_peak;
  pools["large_peak"] = memoryStats.pool_large_peak;
  
  JsonObject corruption = doc.createNestedObject("corruption");
  corruption["heap_ok"] = memoryStats.heap_integrity_ok;
  corruption["heap_corruption_count"] = memoryStats.heap_corruption_count;
  corruption["stack_overflow_count"] = memoryStats.stack_overflow_count;
  
  doc["health"] = memory_get_health_string();
  
  size_t written = serializeJson(doc, buffer, bufferSize);
  return written > 0 && written < bufferSize;
}

bool memory_is_warning() {
  return memoryStats.free_heap < MEMORY_WARNING_THRESHOLD;
}

bool memory_is_critical() {
  return memoryStats.free_heap < MEMORY_CRITICAL_THRESHOLD;
}

const char* memory_get_health_string() {
  if (!memoryStats.heap_integrity_ok) {
    return "CORRUPTED";
  } else if (memory_is_critical()) {
    return "CRITICAL";
  } else if (memory_is_warning()) {
    return "WARNING";
  } else {
    return "GOOD";
  }
}

void* memory_pool_alloc(size_t size) {
  memoryStats.total_allocations++;
  
  // Try to allocate from appropriate pool
  if (size <= MEMORY_POOL_SMALL_SIZE) {
    for (int i = 0; i < MEMORY_POOL_SMALL_COUNT; i++) {
      if (!smallPool[i].in_use) {
        smallPool[i].in_use = true;
        memoryStats.current_allocations++;
        return smallPool[i].data;
      }
    }
  } else if (size <= MEMORY_POOL_MEDIUM_SIZE) {
    for (int i = 0; i < MEMORY_POOL_MEDIUM_COUNT; i++) {
      if (!mediumPool[i].in_use) {
        mediumPool[i].in_use = true;
        memoryStats.current_allocations++;
        return mediumPool[i].data;
      }
    }
  } else if (size <= MEMORY_POOL_LARGE_SIZE) {
    for (int i = 0; i < MEMORY_POOL_LARGE_COUNT; i++) {
      if (!largePool[i].in_use) {
        largePool[i].in_use = true;
        memoryStats.current_allocations++;
        return largePool[i].data;
      }
    }
  }
  
  // Pool exhausted or size too large, fall back to heap
  void* ptr = malloc(size);
  if (ptr) {
    memoryStats.current_allocations++;
  } else {
    memoryStats.failed_allocations++;
  }
  return ptr;
}

void memory_pool_free(void* ptr) {
  if (!ptr) return;
  
  memoryStats.total_frees++;
  
  // Check if pointer is from small pool
  for (int i = 0; i < MEMORY_POOL_SMALL_COUNT; i++) {
    if (smallPool[i].data == ptr) {
      smallPool[i].in_use = false;
      memoryStats.current_allocations--;
      return;
    }
  }
  
  // Check if pointer is from medium pool
  for (int i = 0; i < MEMORY_POOL_MEDIUM_COUNT; i++) {
    if (mediumPool[i].data == ptr) {
      mediumPool[i].in_use = false;
      memoryStats.current_allocations--;
      return;
    }
  }
  
  // Check if pointer is from large pool
  for (int i = 0; i < MEMORY_POOL_LARGE_COUNT; i++) {
    if (largePool[i].data == ptr) {
      largePool[i].in_use = false;
      memoryStats.current_allocations--;
      return;
    }
  }
  
  // Not from pool, free from heap
  free(ptr);
  memoryStats.current_allocations--;
}

void memory_get_pool_stats(char* buffer, size_t bufferSize) {
  if (!buffer || bufferSize < 200) {
    return;
  }
  
  snprintf(buffer, bufferSize,
    "Pool Stats: Small=%u/%d, Medium=%u/%d, Large=%u/%d",
    memoryStats.pool_small_used, MEMORY_POOL_SMALL_COUNT,
    memoryStats.pool_medium_used, MEMORY_POOL_MEDIUM_COUNT,
    memoryStats.pool_large_used, MEMORY_POOL_LARGE_COUNT
  );
}

void memory_reset_stats() {
  memoryStats.total_allocations = 0;
  memoryStats.total_frees = 0;
  memoryStats.failed_allocations = 0;
  memoryStats.heap_corruption_count = 0;
  memoryStats.stack_overflow_count = 0;
  memoryStats.pool_small_peak = 0;
  memoryStats.pool_medium_peak = 0;
  memoryStats.pool_large_peak = 0;
  Serial.println("✅ Memory statistics reset");
}

uint32_t memory_detect_leaks() {
  // Simple leak detection: current allocations should match pool usage
  uint32_t pool_allocations = memoryStats.pool_small_used + 
                               memoryStats.pool_medium_used + 
                               memoryStats.pool_large_used;
  
  if (memoryStats.current_allocations > pool_allocations) {
    uint32_t potential_leaks = memoryStats.current_allocations - pool_allocations;
    Serial.printf("⚠️  Potential memory leaks detected: %u allocations\n", potential_leaks);
    return potential_leaks;
  }
  
  return 0;
}

void memory_dump(void* address, size_t length) {
  if (!address || length == 0) {
    Serial.println("Invalid memory dump parameters");
    return;
  }
  
  Serial.printf("\n=== Memory Dump at 0x%08X (%u bytes) ===\n", (uint32_t)address, length);
  
  uint8_t* ptr = (uint8_t*)address;
  for (size_t i = 0; i < length; i += 16) {
    Serial.printf("0x%08X: ", (uint32_t)(ptr + i));
    
    // Print hex values
    for (size_t j = 0; j < 16 && (i + j) < length; j++) {
      Serial.printf("%02X ", ptr[i + j]);
    }
    
    // Padding
    for (size_t j = length - i; j < 16; j++) {
      Serial.print("   ");
    }
    
    Serial.print(" | ");
    
    // Print ASCII values
    for (size_t j = 0; j < 16 && (i + j) < length; j++) {
      char c = ptr[i + j];
      Serial.print((c >= 32 && c <= 126) ? c : '.');
    }
    
    Serial.println();
  }
  
  Serial.println("=====================================\n");
}
