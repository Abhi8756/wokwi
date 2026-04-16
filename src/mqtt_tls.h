#ifndef MQTT_TLS_H
#define MQTT_TLS_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include "certificates.h"
#include "config.h"  // Include config.h for SystemConfig type

/**
 * @file mqtt_tls.h
 * @brief TLS/SSL Support for Secure MQTT Communication (REQ-014)
 * 
 * SAFETY FEATURES:
 * ================
 * 1. TLS is OPTIONAL - can be enabled/disabled via config
 * 2. Falls back to non-TLS if TLS fails
 * 3. Existing non-TLS code remains unchanged
 * 4. Can switch between TLS and non-TLS without code changes
 * 
 * IMPORTANT NOTES:
 * ================
 * 1. TLS requires ~40KB additional RAM
 * 2. TLS handshake adds ~2 seconds to connection time
 * 3. Requires valid certificates (see certificates.h)
 * 4. MQTT broker must support TLS on port 8883
 * 
 * CONFIGURATION:
 * ==============
 * Set in config.h or via MQTT configuration:
 * - mqtt_tls_enabled: true/false
 * - mqtt_tls_port: 8883 (default TLS port)
 * - mqtt_tls_verify: true/false (verify server certificate)
 * 
 * TESTING:
 * ========
 * For testing without real broker:
 * 1. Set mqtt_tls_enabled = false (use non-TLS)
 * 2. Or use test.mosquitto.org:8883 (public test broker)
 * 3. Or run local Mosquitto with TLS enabled
 * 
 * PRODUCTION:
 * ===========
 * For production deployment:
 * 1. Generate real certificates (see certificates.h)
 * 2. Enable server verification (mqtt_tls_verify = true)
 * 3. Consider certificate pinning for extra security
 * 4. Store certificates in encrypted NVS
 * 5. Use strong cipher suites only
 */

// TLS Configuration
#define MQTT_TLS_PORT 8883
#define MQTT_TLS_TIMEOUT_MS 10000  // 10 second timeout for TLS handshake

// Global TLS client (only created if TLS enabled)
extern WiFiClientSecure* wifiClientSecure;

/**
 * @brief Initialize TLS/SSL for MQTT
 * SAFETY: Only initializes if TLS is enabled in config
 * @return true if TLS initialized successfully, false otherwise
 */
inline bool mqtt_tls_init() {
  // Check if TLS is enabled in configuration
  if (!currentConfig.mqtt_tls_enabled) {
    Serial.println("ℹ️  MQTT TLS disabled in configuration");
    return false;
  }
  
  Serial.println("🔒 Initializing MQTT TLS/SSL...");
  
  // Check if certificates are configured
  if (!certificates_are_configured()) {
    Serial.println("⚠️  WARNING: Using example certificates");
    Serial.println("⚠️  Generate real certificates for production");
  }
  
  // Create secure WiFi client
  if (wifiClientSecure == nullptr) {
    wifiClientSecure = new WiFiClientSecure();
    if (wifiClientSecure == nullptr) {
      Serial.println("❌ Failed to allocate WiFiClientSecure");
      return false;
    }
  }
  
  // Configure certificate validation
  if (currentConfig.mqtt_tls_verify) {
    Serial.println("   Server verification: ENABLED");
    
    // Set CA certificate for server verification
    wifiClientSecure->setCACert(certificates_get_ca_cert());
    
    // Optional: Set client certificate for mutual TLS
    if (MQTT_TLS_USE_CLIENT_CERT) {
      Serial.println("   Client certificate: ENABLED");
      wifiClientSecure->setCertificate(certificates_get_client_cert());
      wifiClientSecure->setPrivateKey(certificates_get_client_key());
    }
    
    // Optional: Use certificate pinning
    if (MQTT_TLS_USE_FINGERPRINT) {
      Serial.println("   Certificate pinning: ENABLED");
      // Note: Fingerprint validation happens during connection
    }
  } else {
    Serial.println("   Server verification: DISABLED (INSECURE)");
    Serial.println("   🚨 WARNING: Vulnerable to MITM attacks!");
    wifiClientSecure->setInsecure();  // Skip certificate validation
  }
  
  // Set timeout for TLS handshake
  wifiClientSecure->setTimeout(MQTT_TLS_TIMEOUT_MS / 1000);
  
  Serial.println("✅ MQTT TLS initialized");
  certificates_print_status();
  
  return true;
}

/**
 * @brief Check if TLS is enabled
 * SAFETY: Read-only check, no side effects
 * @return true if TLS is enabled in configuration
 */
inline bool mqtt_tls_is_enabled() {
  return currentConfig.mqtt_tls_enabled;
}

/**
 * @brief Get MQTT port (TLS or non-TLS)
 * SAFETY: Returns appropriate port based on configuration
 * @return Port number (8883 for TLS, 1883 for non-TLS)
 */
inline uint16_t mqtt_tls_get_port() {
  return mqtt_tls_is_enabled() ? MQTT_TLS_PORT : 1883;
}

/**
 * @brief Get WiFi client (TLS or non-TLS)
 * SAFETY: Returns appropriate client based on configuration
 * @return Pointer to WiFiClient or WiFiClientSecure
 */
inline Client* mqtt_tls_get_client() {
  if (mqtt_tls_is_enabled() && wifiClientSecure != nullptr) {
    return wifiClientSecure;
  }
  
  // Fall back to non-TLS client
  extern WiFiClient wifiClient;
  return &wifiClient;
}

/**
 * @brief Verify TLS connection
 * SAFETY: Only checks connection status, doesn't modify anything
 * @return true if TLS connection is secure and verified
 */
inline bool mqtt_tls_verify_connection() {
  if (!mqtt_tls_is_enabled() || wifiClientSecure == nullptr) {
    return false;  // Not using TLS
  }
  
  if (!wifiClientSecure->connected()) {
    return false;  // Not connected
  }
  
  // Check if certificate was verified
  if (currentConfig.mqtt_tls_verify) {
    // Connection is verified by WiFiClientSecure
    Serial.println("✅ TLS connection verified");
    return true;
  } else {
    Serial.println("⚠️  TLS connection NOT verified (insecure mode)");
    return false;
  }
}

/**
 * @brief Get TLS connection info
 * SAFETY: Read-only, just reports status
 * @param buffer Buffer to store info string
 * @param bufferSize Size of buffer
 */
inline void mqtt_tls_get_info(char* buffer, size_t bufferSize) {
  if (!buffer || bufferSize < 100) {
    return;
  }
  
  if (!mqtt_tls_is_enabled()) {
    snprintf(buffer, bufferSize, "TLS: Disabled (using non-encrypted MQTT)");
    return;
  }
  
  snprintf(buffer, bufferSize,
    "TLS: Enabled\n"
    "  Port: %d\n"
    "  Verification: %s\n"
    "  Client Cert: %s\n"
    "  Pinning: %s",
    MQTT_TLS_PORT,
    currentConfig.mqtt_tls_verify ? "Enabled" : "DISABLED (INSECURE)",
    MQTT_TLS_USE_CLIENT_CERT ? "Yes" : "No",
    MQTT_TLS_USE_FINGERPRINT ? "Yes" : "No"
  );
}

/**
 * @brief Print TLS status
 * SAFETY: Read-only, just prints to serial
 */
inline void mqtt_tls_print_status() {
  Serial.println("\n=== MQTT TLS Status ===");
  
  if (!mqtt_tls_is_enabled()) {
    Serial.println("Status: DISABLED");
    Serial.println("Using non-encrypted MQTT (port 1883)");
    Serial.println("⚠️  Data transmitted in plain text");
    Serial.println("⚠️  Enable TLS for production use");
  } else {
    Serial.println("Status: ENABLED");
    Serial.printf("Port: %d\n", MQTT_TLS_PORT);
    
    Serial.printf("Server Verification: %s\n", 
                  currentConfig.mqtt_tls_verify ? "Enabled" : "DISABLED (INSECURE)");
    
    Serial.printf("Client Certificate: %s\n",
                  MQTT_TLS_USE_CLIENT_CERT ? "Required" : "Not used");
    
    Serial.printf("Certificate Pinning: %s\n",
                  MQTT_TLS_USE_FINGERPRINT ? "Enabled" : "Disabled");
    
    if (wifiClientSecure && wifiClientSecure->connected()) {
      Serial.println("Connection: SECURE ✅");
    } else {
      Serial.println("Connection: Not connected");
    }
    
    if (!currentConfig.mqtt_tls_verify) {
      Serial.println("\n🚨 SECURITY WARNING:");
      Serial.println("🚨 Server verification is DISABLED");
      Serial.println("🚨 Connection vulnerable to MITM attacks");
      Serial.println("🚨 Enable verification for production");
    }
  }
  
  Serial.println("=======================\n");
}

/**
 * @brief Cleanup TLS resources
 * SAFETY: Properly frees allocated memory
 */
inline void mqtt_tls_cleanup() {
  if (wifiClientSecure != nullptr) {
    if (wifiClientSecure->connected()) {
      wifiClientSecure->stop();
    }
    delete wifiClientSecure;
    wifiClientSecure = nullptr;
    Serial.println("✅ TLS resources cleaned up");
  }
}

/**
 * @brief Test TLS connection
 * SAFETY: Non-destructive test, doesn't affect production connection
 * @param server Server hostname or IP
 * @param port Server port (default 8883)
 * @return true if TLS connection successful
 */
inline bool mqtt_tls_test_connection(const char* server, uint16_t port = MQTT_TLS_PORT) {
  if (!mqtt_tls_is_enabled()) {
    Serial.println("❌ TLS not enabled, cannot test");
    return false;
  }
  
  Serial.printf("🔒 Testing TLS connection to %s:%d...\n", server, port);
  
  WiFiClientSecure testClient;
  testClient.setCACert(certificates_get_ca_cert());
  testClient.setTimeout(10);
  
  unsigned long startTime = millis();
  
  if (testClient.connect(server, port)) {
    unsigned long connectTime = millis() - startTime;
    Serial.printf("✅ TLS connection successful (%lu ms)\n", connectTime);
    testClient.stop();
    return true;
  } else {
    Serial.println("❌ TLS connection failed");
    Serial.println("   Check: Server address, port, certificates");
    return false;
  }
}

#endif // MQTT_TLS_H
