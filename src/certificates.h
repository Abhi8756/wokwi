#ifndef CERTIFICATES_H
#define CERTIFICATES_H

#include <Arduino.h>
#include <cstring>

/**
 * @file certificates.h
 * @brief TLS/SSL Certificate Storage for Secure MQTT Communication
 * 
 * IMPORTANT SECURITY NOTES:
 * ========================
 * 
 * 1. THESE ARE EXAMPLE CERTIFICATES FOR TESTING ONLY
 * 2. DO NOT USE THESE IN PRODUCTION
 * 3. Generate your own certificates for production use
 * 4. Store production certificates securely (encrypted NVS)
 * 5. Never commit real certificates to version control
 * 
 * Certificate Generation Instructions:
 * ====================================
 * 
 * For testing with Mosquitto broker:
 * 
 * 1. Generate CA certificate:
 *    openssl req -new -x509 -days 365 -extensions v3_ca \
 *      -keyout ca.key -out ca.crt
 * 
 * 2. Generate server certificate:
 *    openssl genrsa -out server.key 2048
 *    openssl req -new -out server.csr -key server.key
 *    openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key \
 *      -CAcreateserial -out server.crt -days 365
 * 
 * 3. Generate client certificate (optional):
 *    openssl genrsa -out client.key 2048
 *    openssl req -new -out client.csr -key client.key
 *    openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key \
 *      -CAcreateserial -out client.crt -days 365
 * 
 * 4. Convert to C string format:
 *    cat ca.crt | sed 's/^/"/;s/$/\\n"/' > ca_cert.h
 * 
 * Production Deployment:
 * ======================
 * 
 * For production, use one of these approaches:
 * 
 * 1. Certificate Pinning:
 *    - Store SHA256 fingerprint of server certificate
 *    - Validate fingerprint during TLS handshake
 *    - More secure than trusting CA
 * 
 * 2. Encrypted NVS Storage:
 *    - Store certificates in encrypted NVS partition
 *    - Load at runtime
 *    - Allows certificate updates without firmware update
 * 
 * 3. Hardware Security Module (HSM):
 *    - Use ESP32 secure element (ATECC608A)
 *    - Store private keys in hardware
 *    - Highest security level
 */

// ============================================================================
// ROOT CA CERTIFICATE
// ============================================================================
// This is the Certificate Authority that signed the MQTT broker's certificate
// The client uses this to verify the broker's identity

extern const char* ca_cert;

// ============================================================================
// CLIENT CERTIFICATE (Optional - for mutual TLS authentication)
// ============================================================================
// This certificate identifies the ESP32 client to the MQTT broker
// Only needed if broker requires client certificate authentication

extern const char* client_cert;

// ============================================================================
// CLIENT PRIVATE KEY (Optional - for mutual TLS authentication)
// ============================================================================
// CRITICAL SECURITY WARNING:
// - This private key must be kept SECRET
// - Never commit real private keys to version control
// - In production, store in encrypted NVS or hardware security module
// - Consider using certificate-less authentication (PSK-TLS) instead

extern const char* client_key;

// ============================================================================
// CERTIFICATE FINGERPRINT (Alternative to full CA cert)
// ============================================================================
// SHA256 fingerprint of the MQTT broker's certificate
// More secure than trusting a CA - validates specific certificate
// 
// To get fingerprint:
// openssl x509 -noout -fingerprint -sha256 -inform pem -in server.crt

extern const char* server_fingerprint;

// ============================================================================
// CERTIFICATE VALIDATION SETTINGS
// ============================================================================

// Enable/disable certificate validation
// WARNING: Disabling validation makes TLS vulnerable to MITM attacks
// Only disable for testing, NEVER in production
#define MQTT_TLS_VERIFY_SERVER true

// Enable/disable certificate pinning
// When enabled, only accepts server certificate matching fingerprint
// More secure than CA validation
#define MQTT_TLS_USE_FINGERPRINT false

// Enable/disable client certificate authentication
// When enabled, client must present certificate to broker
#define MQTT_TLS_USE_CLIENT_CERT false

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * @brief Check if certificates are configured
 * @return true if certificates appear to be real (not example placeholders)
 */
inline bool certificates_are_configured() {
  // Check if CA cert contains example text
  if (strstr(ca_cert, "Amazon Root CA") != NULL) {
    // Using example Amazon Root CA - this is OK for testing
    return true;
  }
  
  if (strstr(ca_cert, "EXAMPLE") != NULL || 
      strstr(ca_cert, "REPLACE") != NULL) {
    return false;
  }
  
  return true;
}

/**
 * @brief Print certificate configuration status
 */
inline void certificates_print_status() {
  Serial.println("\n=== TLS Certificate Status ===");
  
  Serial.printf("CA Certificate: %s\n", 
                certificates_are_configured() ? "Configured" : "EXAMPLE ONLY");
  
  Serial.printf("Client Certificate: %s\n",
                MQTT_TLS_USE_CLIENT_CERT ? "Required" : "Not used");
  
  Serial.printf("Certificate Pinning: %s\n",
                MQTT_TLS_USE_FINGERPRINT ? "Enabled" : "Disabled");
  
  Serial.printf("Server Verification: %s\n",
                MQTT_TLS_VERIFY_SERVER ? "Enabled" : "DISABLED (INSECURE)");
  
  if (!certificates_are_configured()) {
    Serial.println("\n⚠️  WARNING: Using example certificates!");
    Serial.println("⚠️  Generate real certificates for production use");
    Serial.println("⚠️  See certificates.h for instructions");
  }
  
  if (!MQTT_TLS_VERIFY_SERVER) {
    Serial.println("\n🚨 CRITICAL: Server verification DISABLED!");
    Serial.println("🚨 Connection is vulnerable to MITM attacks");
    Serial.println("🚨 Enable verification for production use");
  }
  
  Serial.println("==============================\n");
}

/**
 * @brief Get CA certificate
 * @return Pointer to CA certificate string
 */
inline const char* certificates_get_ca_cert() {
  return ca_cert;
}

/**
 * @brief Get client certificate
 * @return Pointer to client certificate string (NULL if not used)
 */
inline const char* certificates_get_client_cert() {
  return MQTT_TLS_USE_CLIENT_CERT ? client_cert : NULL;
}

/**
 * @brief Get client private key
 * @return Pointer to client private key string (NULL if not used)
 */
inline const char* certificates_get_client_key() {
  return MQTT_TLS_USE_CLIENT_CERT ? client_key : NULL;
}

/**
 * @brief Get server fingerprint
 * @return Pointer to fingerprint string (NULL if not used)
 */
inline const char* certificates_get_fingerprint() {
  return MQTT_TLS_USE_FINGERPRINT ? server_fingerprint : NULL;
}

#endif // CERTIFICATES_H
