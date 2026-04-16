#include "ota.h"
#include "config.h"

// Global OTA state
OTAState otaState = {
  OTA_STATUS_IDLE,                // status
  0,                              // progress
  0,                              // total_size
  0,                              // current_size
  0,                              // start_time
  0,                              // end_time
  "",                             // error_message
  false,                          // initialized
  0,                              // boot_count
  false                           // validation_pending
};

// OTA Callbacks
ota_progress_callback_t ota_progress_callback = nullptr;
ota_error_callback_t ota_error_callback = nullptr;
ota_complete_callback_t ota_complete_callback = nullptr;

// Boot count storage in RTC memory (survives resets but not power cycles)
RTC_DATA_ATTR uint32_t rtc_boot_count = 0;
RTC_DATA_ATTR bool rtc_validation_pending = false;

bool ota_init(const char* hostname, const char* password) {
  Serial.println("🔄 Initializing OTA update system...");
  
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ OTA: WiFi not connected");
    return false;
  }
  
  // Increment boot count
  rtc_boot_count++;
  otaState.boot_count = rtc_boot_count;
  otaState.validation_pending = rtc_validation_pending;
  
  Serial.printf("   Boot count: %u\n", otaState.boot_count);
  
  // Check if validation is pending from previous OTA update
  if (otaState.validation_pending) {
    Serial.println("⚠️  Firmware validation pending from previous OTA update");
    
    // Check if boot count exceeds threshold (indicates boot loop)
    if (otaState.boot_count > 3) {
      Serial.println("❌ Boot loop detected - firmware validation failed");
      Serial.println("🔄 Attempting rollback to previous firmware...");
      
      if (ota_rollback()) {
        Serial.println("✅ Rollback initiated - system will restart");
        delay(1000);
        ESP.restart();
      } else {
        Serial.println("❌ Rollback failed - manual intervention required");
      }
      
      return false;
    }
  }
  
  // Configure ArduinoOTA
  ArduinoOTA.setHostname(hostname);
  
  if (password && strlen(password) > 0) {
    ArduinoOTA.setPassword(password);
    Serial.printf("   OTA password: %s\n", "***");
  }
  
  ArduinoOTA.setPort(OTA_PORT);
  
  // OTA Start callback
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_SPIFFS
      type = "filesystem";
    }
    
    Serial.println("🔄 OTA Update Started: " + type);
    otaState.status = OTA_STATUS_STARTING;
    otaState.progress = 0;
    otaState.start_time = millis();
    otaState.error_message[0] = '\0';
    
    // Stop critical tasks during OTA
    // tasks_stop();  // Uncomment if needed
  });
  
  // OTA End callback
  ArduinoOTA.onEnd([]() {
    Serial.println("\n✅ OTA Update Complete");
    otaState.status = OTA_STATUS_COMPLETED;
    otaState.progress = 100;
    otaState.end_time = millis();
    
    // Mark validation as pending
    rtc_validation_pending = true;
    rtc_boot_count = 0;  // Reset boot count for new firmware
    
    if (ota_complete_callback) {
      ota_complete_callback();
    }
    
    Serial.println("🔄 System will restart to apply update...");
  });
  
  // OTA Progress callback
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    otaState.status = OTA_STATUS_IN_PROGRESS;
    otaState.current_size = progress;
    otaState.total_size = total;
    otaState.progress = (progress * 100) / total;
    
    // Report progress every 10%
    static uint8_t last_reported_progress = 0;
    if (otaState.progress >= last_reported_progress + 10) {
      Serial.printf("📊 OTA Progress: %u%%\n", otaState.progress);
      last_reported_progress = otaState.progress;
      
      if (ota_progress_callback) {
        ota_progress_callback(otaState.progress);
      }
    }
  });
  
  // OTA Error callback
  ArduinoOTA.onError([](ota_error_t error) {
    otaState.status = OTA_STATUS_FAILED;
    otaState.end_time = millis();
    
    const char* error_str;
    switch (error) {
      case OTA_AUTH_ERROR:
        error_str = "Auth Failed";
        break;
      case OTA_BEGIN_ERROR:
        error_str = "Begin Failed";
        break;
      case OTA_CONNECT_ERROR:
        error_str = "Connect Failed";
        break;
      case OTA_RECEIVE_ERROR:
        error_str = "Receive Failed";
        break;
      case OTA_END_ERROR:
        error_str = "End Failed";
        break;
      default:
        error_str = "Unknown Error";
        break;
    }
    
    snprintf(otaState.error_message, sizeof(otaState.error_message), 
             "OTA Error[%u]: %s", error, error_str);
    
    Serial.printf("❌ %s\n", otaState.error_message);
    
    if (ota_error_callback) {
      ota_error_callback(otaState.error_message);
    }
  });
  
  // Start OTA service
  ArduinoOTA.begin();
  
  otaState.initialized = true;
  
  Serial.println("✅ OTA update system initialized");
  Serial.printf("   Hostname: %s\n", hostname);
  Serial.printf("   Port: %d\n", OTA_PORT);
  
  // Print partition information
  char partInfo[256];
  ota_get_partition_info(partInfo, sizeof(partInfo));
  Serial.println(partInfo);
  
  return true;
}

void ota_handle() {
  if (!otaState.initialized) {
    return;
  }
  
  ArduinoOTA.handle();
}

ota_status_t ota_get_status() {
  return otaState.status;
}

const char* ota_get_status_string() {
  switch (otaState.status) {
    case OTA_STATUS_IDLE: return "Idle";
    case OTA_STATUS_STARTING: return "Starting";
    case OTA_STATUS_IN_PROGRESS: return "In Progress";
    case OTA_STATUS_COMPLETED: return "Completed";
    case OTA_STATUS_FAILED: return "Failed";
    case OTA_STATUS_VALIDATING: return "Validating";
    default: return "Unknown";
  }
}

uint8_t ota_get_progress() {
  return otaState.progress;
}

void ota_get_status_info(char* buffer, size_t bufferSize) {
  if (!buffer || bufferSize < 300) {
    return;
  }
  
  unsigned long duration = 0;
  if (otaState.start_time > 0) {
    if (otaState.end_time > 0) {
      duration = otaState.end_time - otaState.start_time;
    } else {
      duration = millis() - otaState.start_time;
    }
  }
  
  snprintf(buffer, bufferSize,
    "OTA Status: %s\n"
    "  Progress: %u%%\n"
    "  Size: %u / %u bytes\n"
    "  Duration: %lu ms\n"
    "  Boot count: %u\n"
    "  Validation pending: %s\n"
    "  Error: %s",
    ota_get_status_string(),
    otaState.progress,
    otaState.current_size,
    otaState.total_size,
    duration,
    otaState.boot_count,
    otaState.validation_pending ? "Yes" : "No",
    otaState.error_message[0] ? otaState.error_message : "None"
  );
}

bool ota_validate_firmware() {
  Serial.println("🔍 Validating firmware...");
  
  // Get current partition
  const esp_partition_t* partition = esp_ota_get_running_partition();
  if (!partition) {
    Serial.println("❌ Failed to get running partition");
    return false;
  }
  
  Serial.printf("   Running partition: %s (type=%d, subtype=%d)\n",
                partition->label, partition->type, partition->subtype);
  
  // Basic validation: check if partition is valid
  if (partition->type != ESP_PARTITION_TYPE_APP) {
    Serial.println("❌ Invalid partition type");
    return false;
  }
  
  // Additional validation could include:
  // - Signature verification
  // - Checksum validation
  // - Version checking
  
  Serial.println("✅ Firmware validation successful");
  return true;
}

void ota_mark_valid() {
  Serial.println("✅ Marking firmware as valid");
  
  // Reset boot count
  rtc_boot_count = 0;
  rtc_validation_pending = false;
  otaState.boot_count = 0;
  otaState.validation_pending = false;
  
  // Mark partition as valid in OTA data
  const esp_partition_t* partition = esp_ota_get_running_partition();
  if (partition) {
    esp_ota_mark_app_valid_cancel_rollback();
    Serial.println("✅ Rollback cancelled - firmware marked as valid");
  }
}

bool ota_can_rollback() {
  const esp_partition_t* partition = esp_ota_get_last_invalid_partition();
  return (partition != nullptr);
}

bool ota_rollback() {
  Serial.println("🔄 Initiating firmware rollback...");
  
  const esp_partition_t* partition = esp_ota_get_last_invalid_partition();
  if (!partition) {
    Serial.println("❌ No rollback partition available");
    return false;
  }
  
  Serial.printf("   Rollback partition: %s\n", partition->label);
  
  esp_err_t err = esp_ota_set_boot_partition(partition);
  if (err != ESP_OK) {
    Serial.printf("❌ Rollback failed: %s\n", esp_err_to_name(err));
    return false;
  }
  
  Serial.println("✅ Rollback configured - restart to apply");
  return true;
}

uint32_t ota_get_boot_count() {
  return otaState.boot_count;
}

void ota_reset_boot_count() {
  rtc_boot_count = 0;
  otaState.boot_count = 0;
}

bool ota_is_validation_pending() {
  return otaState.validation_pending;
}

void ota_get_partition_info(char* buffer, size_t bufferSize) {
  if (!buffer || bufferSize < 200) {
    return;
  }
  
  const esp_partition_t* running = esp_ota_get_running_partition();
  const esp_partition_t* boot = esp_ota_get_boot_partition();
  const esp_partition_t* next = esp_ota_get_next_update_partition(nullptr);
  
  snprintf(buffer, bufferSize,
    "Partition Information:\n"
    "  Running: %s (0x%x, size=%u KB)\n"
    "  Boot: %s\n"
    "  Next Update: %s (0x%x, size=%u KB)\n"
    "  Rollback available: %s",
    running ? running->label : "Unknown",
    running ? running->address : 0,
    running ? running->size / 1024 : 0,
    boot ? boot->label : "Unknown",
    next ? next->label : "Unknown",
    next ? next->address : 0,
    next ? next->size / 1024 : 0,
    ota_can_rollback() ? "Yes" : "No"
  );
}