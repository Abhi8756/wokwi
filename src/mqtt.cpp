#include "mqtt.h"
#include "mqtt_tls.h"

// Global MQTT objects
MQTTState mqttState;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

bool mqtt_init(const char* deviceId, const char* locationId) {
  // Initialize state
  mqtt_init_state();
  
  // Set device ID
  if (deviceId) {
    strncpy(mqttState.deviceId, deviceId, sizeof(mqttState.deviceId) - 1);
  } else {
    // Generate device ID from MAC address
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(mqttState.deviceId, sizeof(mqttState.deviceId), "DEV%02X%02X", mac[4], mac[5]);
  }
  
  // Generate unique client ID
  mqtt_generate_client_id();
  
  // Initialize TLS if enabled (REQ-014)
  if (mqtt_tls_is_enabled()) {
    if (mqtt_tls_init()) {
      Serial.println("🔒 MQTT TLS initialized successfully");
    } else {
      Serial.println("⚠️  MQTT TLS initialization failed - falling back to non-TLS");
    }
  }
  
  // Configure MQTT client with appropriate WiFi client (TLS or non-TLS)
  Client* client = mqtt_tls_get_client();
  mqttClient.setClient(*client);
  
  // Use appropriate server and port based on TLS configuration
  uint16_t port = mqtt_tls_get_port();
  mqttClient.setServer(MQTT_SERVER, port);
  mqttClient.setCallback(mqtt_callback);
  mqttClient.setKeepAlive(MQTT_KEEPALIVE_INTERVAL);
  
  mqttState.initialized = true;
  
  Serial.println("📡 MQTT client initialized");
  Serial.printf("   Device ID: %s\n", mqttState.deviceId);
  Serial.printf("   Client ID: %s\n", mqttState.clientId);
  Serial.printf("   Server: %s:%d\n", MQTT_SERVER, port);
  Serial.printf("   TLS: %s\n", mqtt_tls_is_enabled() ? "Enabled" : "Disabled");
  
  return true;
}

bool mqtt_connect() {
  if (!mqttState.initialized || WiFi.status() != WL_CONNECTED) {
    return false;
  }
  
  if (mqttClient.connected()) {
    mqttState.state = MQTT_STATE_CONNECTED;
    return true;
  }
  
  mqttState.state = MQTT_STATE_CONNECTING;
  
  // Build Last Will and Testament topic and message
  char lwt_topic[128];
  mqtt_build_topic(lwt_topic, sizeof(lwt_topic), MQTT_TOPIC_STATUS);
  
  char lwt_message[256];
  snprintf(lwt_message, sizeof(lwt_message), 
    "{\"device_id\":\"%s\",\"status\":\"offline\",\"timestamp\":%lu,\"reason\":\"unexpected_disconnect\"}",
    mqttState.deviceId, millis());
  
  // Get appropriate port and connection type
  uint16_t port = mqtt_tls_get_port();
  const char* connType = mqtt_tls_is_enabled() ? "TLS" : "non-TLS";
  
  Serial.printf("📡 Connecting to MQTT broker %s:%d (%s)...\n", MQTT_SERVER, port, connType);
  
  // Attempt connection with Last Will Testament
  bool connected = false;
  if (strlen(MQTT_USERNAME) > 0) {
    connected = mqttClient.connect(mqttState.clientId, MQTT_USERNAME, MQTT_PASSWORD, 
                                  lwt_topic, MQTT_QOS_SENSORS, true, lwt_message);
  } else {
    connected = mqttClient.connect(mqttState.clientId, lwt_topic, MQTT_QOS_SENSORS, true, lwt_message);
  }
  
  if (connected) {
    mqttState.state = MQTT_STATE_CONNECTED;
    mqttState.reconnectAttempts = 0;
    mqttState.lastHeartbeat = millis();
    
    Serial.printf("✅ MQTT connected successfully (%s)\n", connType);
    
    // Verify TLS connection if enabled (REQ-014)
    if (mqtt_tls_is_enabled()) {
      if (mqtt_tls_verify_connection()) {
        Serial.println("🔒 TLS connection verified and secure");
      } else {
        Serial.println("⚠️  TLS connection not verified (check configuration)");
      }
    }
    
    // Subscribe to configuration topic
    char config_topic[128];
    mqtt_build_topic(config_topic, sizeof(config_topic), MQTT_TOPIC_CONFIG);
    mqttClient.subscribe(config_topic, MQTT_QOS_CONFIG);
    
    // Publish online status
    mqtt_publish_status("online");
    
    // Flush any buffered messages
    mqtt_flush_buffer();
    
    return true;
  } else {
    mqttState.state = MQTT_STATE_ERROR;
    mqttState.reconnectAttempts++;
    
    Serial.printf("❌ MQTT connection failed, rc=%d (%s)\n", mqttClient.state(), connType);
    
    // If TLS connection failed, provide helpful error message
    if (mqtt_tls_is_enabled()) {
      Serial.println("   TLS troubleshooting:");
      Serial.println("   - Check broker supports TLS on port 8883");
      Serial.println("   - Verify certificates are correct");
      Serial.println("   - Check system time (required for certificate validation)");
      Serial.println("   - Try disabling TLS verification for testing");
    }
    
    return false;
  }
}

void mqtt_disconnect() {
  if (mqttClient.connected()) {
    // Publish offline status before disconnecting
    mqtt_publish_status("offline");
    mqttClient.disconnect();
  }
  mqttState.state = MQTT_STATE_DISCONNECTED;
  Serial.println("📡 MQTT disconnected");
}

void mqtt_update() {
  if (!mqttState.initialized) {
    return;
  }
  
  unsigned long currentTime = millis();
  
  // Handle connection state
  if (mqttClient.connected()) {
    // Process incoming messages
    mqttClient.loop();
    
    // Send periodic heartbeat
    if (currentTime - mqttState.lastHeartbeat > (MQTT_KEEPALIVE_INTERVAL * 1000 / 2)) {
      char heartbeat[128];
      snprintf(heartbeat, sizeof(heartbeat), 
        "{\"device_id\":\"%s\",\"uptime\":%lu,\"free_heap\":%u}",
        mqttState.deviceId, currentTime, ESP.getFreeHeap());
      
      mqtt_publish_diagnostics(heartbeat);
      mqttState.lastHeartbeat = currentTime;
    }
  } else {
    // Handle reconnection
    if (mqttState.state == MQTT_STATE_CONNECTED) {
      mqttState.state = MQTT_STATE_RECONNECTING;
      Serial.println("⚠️  MQTT connection lost, attempting reconnection...");
    }
    
    if (currentTime - mqttState.lastConnectAttempt > MQTT_RECONNECT_INTERVAL) {
      mqttState.lastConnectAttempt = currentTime;
      
      if (mqtt_connect()) {
        mqttState.reconnections++;
        Serial.printf("✅ MQTT reconnected (attempt %d)\n", mqttState.reconnectAttempts);
      } else {
        Serial.printf("⚠️  MQTT reconnection failed (attempt %d)\n", mqttState.reconnectAttempts);
      }
    }
  }
}

bool mqtt_publish_sensors(const char* sensorData) {
  char topic[128];
  mqtt_build_topic(topic, sizeof(topic), MQTT_TOPIC_SENSORS);
  
  if (mqtt_is_connected()) {
    bool success = mqttClient.publish(topic, (const uint8_t*)sensorData, strlen(sensorData));
    if (success) {
      mqttState.messagesSent++;
      mqttState.lastMessageTime = millis();
      return true;
    }
  }
  
  // Buffer message if not connected
  return mqtt_buffer_message(topic, sensorData, MQTT_QOS_SENSORS);
}

bool mqtt_publish_diagnostics(const char* diagnosticData) {
  char topic[128];
  mqtt_build_topic(topic, sizeof(topic), MQTT_TOPIC_DIAGNOSTICS);
  
  if (mqtt_is_connected()) {
    bool success = mqttClient.publish(topic, (const uint8_t*)diagnosticData, strlen(diagnosticData));
    if (success) {
      mqttState.messagesSent++;
      return true;
    }
  }
  
  // Don't buffer diagnostics (QoS 0 - fire and forget)
  return false;
}

bool mqtt_publish_alert(const char* alertData) {
  char topic[128];
  mqtt_build_topic(topic, sizeof(topic), MQTT_TOPIC_ALERTS);
  
  if (mqtt_is_connected()) {
    bool success = mqttClient.publish(topic, (const uint8_t*)alertData, strlen(alertData), true); // Retained
    if (success) {
      mqttState.messagesSent++;
      return true;
    }
  }
  
  // Buffer alerts with high priority
  return mqtt_buffer_message(topic, alertData, MQTT_QOS_SENSORS, true);
}

bool mqtt_publish_status(const char* status) {
  char topic[128];
  mqtt_build_topic(topic, sizeof(topic), MQTT_TOPIC_STATUS);
  
  char statusMessage[256];
  snprintf(statusMessage, sizeof(statusMessage), 
    "{\"device_id\":\"%s\",\"status\":\"%s\",\"timestamp\":%lu}",
    mqttState.deviceId, status, millis());
  
  if (mqtt_is_connected()) {
    return mqttClient.publish(topic, (const uint8_t*)statusMessage, strlen(statusMessage), true); // Retained
  }
  
  return false;
}

mqtt_state_t mqtt_get_state() {
  return mqttState.state;
}

const char* mqtt_get_state_string() {
  switch (mqttState.state) {
    case MQTT_STATE_DISCONNECTED: return "Disconnected";
    case MQTT_STATE_CONNECTING: return "Connecting";
    case MQTT_STATE_CONNECTED: return "Connected";
    case MQTT_STATE_RECONNECTING: return "Reconnecting";
    case MQTT_STATE_ERROR: return "Error";
    default: return "Unknown";
  }
}

void mqtt_get_status_info(char* buffer, size_t bufferSize) {
  if (!buffer || bufferSize < 300) {
    return;
  }
  
  uint16_t port = mqtt_tls_get_port();
  const char* tlsStatus = mqtt_tls_is_enabled() ? "Enabled" : "Disabled";
  
  snprintf(buffer, bufferSize,
    "MQTT Status: %s\n"
    "  Device ID: %s\n"
    "  Client ID: %s\n"
    "  Server: %s:%d\n"
    "  TLS: %s\n"
    "  Messages sent: %lu\n"
    "  Messages buffered: %lu\n"
    "  Buffered messages: %d/%d\n"
    "  Reconnections: %lu\n"
    "  Last message: %lu ms ago",
    mqtt_get_state_string(),
    mqttState.deviceId,
    mqttState.clientId,
    MQTT_SERVER,
    port,
    tlsStatus,
    mqttState.messagesSent,
    mqttState.messagesBuffered,
    mqttState.bufferedMessages,
    MQTT_BUFFER_SIZE,
    mqttState.reconnections,
    mqttState.lastMessageTime > 0 ? millis() - mqttState.lastMessageTime : 0
  );
}

bool mqtt_force_reconnect() {
  if (mqttClient.connected()) {
    mqtt_disconnect();
  }
  
  mqttState.lastConnectAttempt = 0; // Force immediate reconnection attempt
  return mqtt_connect();
}

// Internal helper functions
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  // Convert payload to string
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';
  
  Serial.printf("📡 MQTT message received: %s -> %s\n", topic, message);
  
  // Handle configuration updates
  char config_topic[128];
  mqtt_build_topic(config_topic, sizeof(config_topic), MQTT_TOPIC_CONFIG);
  
  if (strcmp(topic, config_topic) == 0) {
    // Parse and apply configuration (REQ-009)
    Serial.println("📡 Processing configuration update...");
    
    // Forward to config module
    extern bool config_update_from_json(const char* json);
    if (config_update_from_json(message)) {
      Serial.println("✅ Configuration updated successfully");
      
      // Send acknowledgment
      char ack[128];
      snprintf(ack, sizeof(ack), "{\"status\":\"ok\",\"message\":\"Configuration updated\"}");
      mqtt_publish_status(ack);
    } else {
      Serial.println("❌ Configuration update failed");
      
      // Send error
      char error[128];
      snprintf(error, sizeof(error), "{\"status\":\"error\",\"message\":\"Configuration update failed\"}");
      mqtt_publish_status(error);
    }
  }
}

bool mqtt_buffer_message(const char* topic, const char* payload, uint8_t qos, bool retain) {
  if (mqttState.bufferedMessages >= MQTT_BUFFER_SIZE) {
    // Buffer full, drop oldest message
    mqttState.bufferTail = (mqttState.bufferTail + 1) % MQTT_BUFFER_SIZE;
    mqttState.bufferedMessages--;
  }
  
  MQTTMessage* msg = &mqttState.messageBuffer[mqttState.bufferHead];
  strncpy(msg->topic, topic, sizeof(msg->topic) - 1);
  strncpy(msg->payload, payload, sizeof(msg->payload) - 1);
  msg->qos = qos;
  msg->retain = retain;
  msg->timestamp = millis();
  
  mqttState.bufferHead = (mqttState.bufferHead + 1) % MQTT_BUFFER_SIZE;
  mqttState.bufferedMessages++;
  mqttState.messagesBuffered++;
  
  Serial.printf("📡 Message buffered (%d/%d): %s\n", 
                mqttState.bufferedMessages, MQTT_BUFFER_SIZE, topic);
  
  return true;
}

void mqtt_flush_buffer() {
  if (!mqtt_is_connected() || mqttState.bufferedMessages == 0) {
    return;
  }
  
  Serial.printf("📡 Flushing %d buffered messages...\n", mqttState.bufferedMessages);
  
  int flushed = 0;
  while (mqttState.bufferedMessages > 0 && flushed < 5) { // Limit to 5 per cycle
    MQTTMessage* msg = &mqttState.messageBuffer[mqttState.bufferTail];
    
    if (mqttClient.publish(msg->topic, (const uint8_t*)msg->payload, strlen(msg->payload), msg->retain)) {
      mqttState.bufferTail = (mqttState.bufferTail + 1) % MQTT_BUFFER_SIZE;
      mqttState.bufferedMessages--;
      mqttState.messagesSent++;
      flushed++;
    } else {
      break; // Stop if publish fails
    }
  }
  
  if (flushed > 0) {
    Serial.printf("📡 Flushed %d messages, %d remaining\n", flushed, mqttState.bufferedMessages);
  }
}

void mqtt_build_topic(char* buffer, size_t bufferSize, const char* subtopic) {
  snprintf(buffer, bufferSize, "%s/%s/%s", 
           MQTT_TOPIC_PREFIX, MQTT_LOCATION_ID, subtopic);
}

void mqtt_generate_client_id() {
  snprintf(mqttState.clientId, sizeof(mqttState.clientId), 
           "%s%s_%lu", MQTT_CLIENT_ID_PREFIX, mqttState.deviceId, millis());
}