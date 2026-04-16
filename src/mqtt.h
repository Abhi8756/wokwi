#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

/**
 * @file mqtt.h
 * @brief MQTT communication module for IoT sensor data transmission
 * 
 * This module provides MQTT connectivity for reliable, low-power sensor data
 * transmission. Implements professional IoT communication patterns including
 * QoS levels, Last Will Testament, offline buffering, and topic hierarchy.
 * 
 * Key Features:
 * - QoS-based message delivery (sensors=1, diagnostics=0, config=2)
 * - Last Will and Testament for device status monitoring
 * - Offline message buffering (10 readings in memory)
 * - Hierarchical topic structure for scalability
 * - Automatic reconnection with exponential backoff
 * - Message compression and batching for efficiency
 */

// MQTT Configuration Constants
#define MQTT_SERVER "localhost"
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID_PREFIX "manhole_sensor_"
#define MQTT_USERNAME ""  // Set via environment or config
#define MQTT_PASSWORD ""  // Set via environment or config

// Topic Configuration
#define MQTT_TOPIC_PREFIX "manhole"
#define MQTT_LOCATION_ID "location_001"  // Configurable per device
#define MQTT_TOPIC_SENSORS "sensors"
#define MQTT_TOPIC_DIAGNOSTICS "diagnostics"
#define MQTT_TOPIC_CONFIG "config"
#define MQTT_TOPIC_STATUS "status"
#define MQTT_TOPIC_ALERTS "alerts"

// QoS Levels (REQ-007)
#define MQTT_QOS_DIAGNOSTICS 0  // Fire and forget
#define MQTT_QOS_SENSORS 1      // At least once delivery
#define MQTT_QOS_CONFIG 2       // Exactly once delivery

// Buffer Configuration
#define MQTT_BUFFER_SIZE 10     // Offline message buffer
#define MQTT_MAX_MESSAGE_SIZE 512
#define MQTT_KEEPALIVE_INTERVAL 60
#define MQTT_RECONNECT_INTERVAL 5000

// Connection State
typedef enum {
  MQTT_STATE_DISCONNECTED,
  MQTT_STATE_CONNECTING,
  MQTT_STATE_CONNECTED,
  MQTT_STATE_RECONNECTING,
  MQTT_STATE_ERROR
} mqtt_state_t;

// Message Buffer Structure
struct MQTTMessage {
  char topic[64];
  char payload[MQTT_MAX_MESSAGE_SIZE];
  uint8_t qos;
  bool retain;
  unsigned long timestamp;
};

// MQTT State Management
struct MQTTState {
  mqtt_state_t state;
  unsigned long lastConnectAttempt;
  unsigned long lastHeartbeat;
  int reconnectAttempts;
  bool initialized;
  char clientId[32];
  char deviceId[16];
  
  // Message buffering
  MQTTMessage messageBuffer[MQTT_BUFFER_SIZE];
  int bufferHead;
  int bufferTail;
  int bufferedMessages;
  
  // Statistics
  unsigned long messagesSent;
  unsigned long messagesBuffered;
  unsigned long reconnections;
  unsigned long lastMessageTime;
};

// Global MQTT state and client
extern MQTTState mqttState;
extern WiFiClient wifiClient;
extern PubSubClient mqttClient;

/**
 * @brief Initialize MQTT client and connection
 * @param deviceId Unique device identifier (e.g., "DEV001")
 * @param locationId Location identifier for topic hierarchy
 * @return true if initialization successful, false otherwise
 */
bool mqtt_init(const char* deviceId = nullptr, const char* locationId = MQTT_LOCATION_ID);

/**
 * @brief Connect to MQTT broker with Last Will Testament
 * @return true if connection successful, false otherwise
 */
bool mqtt_connect();

/**
 * @brief Disconnect from MQTT broker gracefully
 */
void mqtt_disconnect();

/**
 * @brief Check connection status and handle reconnection
 * Should be called regularly in main loop
 */
void mqtt_update();

/**
 * @brief Publish sensor data with appropriate QoS
 * @param sensorData JSON string containing sensor readings
 * @return true if published successfully, false if buffered
 */
bool mqtt_publish_sensors(const char* sensorData);

/**
 * @brief Publish diagnostic information
 * @param diagnosticData JSON string containing system diagnostics
 * @return true if published successfully, false if buffered
 */
bool mqtt_publish_diagnostics(const char* diagnosticData);

/**
 * @brief Publish alert message with high priority
 * @param alertData JSON string containing alert information
 * @return true if published successfully, false if buffered
 */
bool mqtt_publish_alert(const char* alertData);

/**
 * @brief Publish device status (online/offline)
 * @param status Status string ("online", "offline", "maintenance")
 * @return true if published successfully, false otherwise
 */
bool mqtt_publish_status(const char* status);

/**
 * @brief Get MQTT connection state
 * @return Current connection state
 */
mqtt_state_t mqtt_get_state();

/**
 * @brief Get MQTT connection state as string
 * @return Human-readable state string
 */
const char* mqtt_get_state_string();

/**
 * @brief Get MQTT statistics and status information
 * @param buffer Buffer to store status information
 * @param bufferSize Size of the buffer
 */
void mqtt_get_status_info(char* buffer, size_t bufferSize);

/**
 * @brief Force MQTT reconnection
 * @return true if reconnection initiated, false otherwise
 */
bool mqtt_force_reconnect();

/**
 * @brief Check if MQTT is connected and ready
 * @return true if connected, false otherwise
 */
inline bool mqtt_is_connected() {
  return mqttState.state == MQTT_STATE_CONNECTED && mqttClient.connected();
}

// Internal helper functions
void mqtt_callback(char* topic, byte* payload, unsigned int length);
bool mqtt_buffer_message(const char* topic, const char* payload, uint8_t qos, bool retain = false);
void mqtt_flush_buffer();
void mqtt_build_topic(char* buffer, size_t bufferSize, const char* subtopic);
void mqtt_generate_client_id();

// MQTT State Initialization
inline void mqtt_init_state() {
  mqttState.state = MQTT_STATE_DISCONNECTED;
  mqttState.lastConnectAttempt = 0;
  mqttState.lastHeartbeat = 0;
  mqttState.reconnectAttempts = 0;
  mqttState.initialized = false;
  mqttState.bufferHead = 0;
  mqttState.bufferTail = 0;
  mqttState.bufferedMessages = 0;
  mqttState.messagesSent = 0;
  mqttState.messagesBuffered = 0;
  mqttState.reconnections = 0;
  mqttState.lastMessageTime = 0;
  memset(mqttState.clientId, 0, sizeof(mqttState.clientId));
  memset(mqttState.deviceId, 0, sizeof(mqttState.deviceId));
}

#endif // MQTT_H