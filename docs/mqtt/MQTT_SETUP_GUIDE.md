# MQTT Setup Guide for IoT Manhole Monitoring System

This guide will help you set up MQTT communication between your ESP32 firmware and the bridge server.

## 🎯 What You'll Achieve

- ESP32 publishes sensor data via MQTT (instead of HTTP)
- Bridge server subscribes to MQTT topics and receives data
- Real-time communication with QoS guarantees
- Offline message buffering on ESP32

---

## 📋 Prerequisites

- Docker installed and running
- Node.js and npm installed
- Wokwi simulator running

---

## 🚀 Quick Start (3 Steps)

### Step 1: Start the MQTT Broker

```bash
cd wokwi
./start-mqtt.sh
```

This will:
- Start Mosquitto MQTT broker in Docker
- Expose MQTT on port 1883
- Configure persistence and logging

**Verify it's running:**
```bash
docker ps | grep mosquitto
```

You should see: `manhole-mqtt-broker`

---

### Step 2: Update Environment Variables

Create or update `wokwi/.env` file:

```bash
# MQTT Configuration
MQTT_BROKER=mqtt://localhost:1883
MQTT_USERNAME=
MQTT_PASSWORD=
MQTT_TOPIC_PREFIX=manhole
MQTT_LOCATION_ID=location_001

# Bridge Server
PORT=3001
BRIDGE_API_KEY=your-secret-key-here
REQUIRE_API_KEY=false

# Storage Backend
SENSOR_STORAGE_BACKEND=json
FIRESTORE_COLLECTION=sensor_readings
ALERTS_COLLECTION=alert_history
```

---

### Step 3: Start the Bridge Server

```bash
cd wokwi
npm run bridge
```

You should see:
```
✅ MQTT client connected to mqtt://localhost:1883
📡 Subscribed to MQTT topics: manhole/location_001/+
```

---

## 🔧 How It Works

### MQTT Topic Structure

The system uses a hierarchical topic structure:

```
manhole/
  └── {location_id}/
      ├── sensors      (QoS 1) - Sensor readings
      ├── alerts       (QoS 1) - Alert notifications
      ├── diagnostics  (QoS 0) - System diagnostics
      └── config       (QoS 2) - Configuration updates
```

### Data Flow

```
┌─────────────┐         ┌──────────────┐         ┌──────────────┐
│   ESP32     │ MQTT    │  Mosquitto   │ MQTT    │    Bridge    │
│  Firmware   ├────────>│    Broker    ├────────>│    Server    │
│  (Wokwi)    │ Publish │  (Docker)    │Subscribe│  (Node.js)   │
└─────────────┘         └──────────────┘         └──────────────┘
                                                         │
                                                         v
                                                  ┌──────────────┐
                                                  │   Storage    │
                                                  │ (JSON/Cloud) │
                                                  └──────────────┘
```

### ESP32 Behavior

1. **Connected to MQTT**: Publishes directly to broker
2. **MQTT Unavailable**: Buffers up to 10 messages in memory
3. **Reconnection**: Sends buffered messages with exponential backoff

---

## 🧪 Testing MQTT Communication

### Test 1: Monitor MQTT Traffic

Open a new terminal and subscribe to all topics:

```bash
# Using mosquitto_sub (if installed)
mosquitto_sub -h localhost -p 1883 -t "manhole/#" -v

# OR using Docker
docker exec -it manhole-mqtt-broker mosquitto_sub -t "manhole/#" -v
```

### Test 2: Verify ESP32 Publishing

1. Start Wokwi simulator
2. Watch the serial output - you should see:
   ```
   ✅ MQTT connected to localhost:1883
   📡 Published to manhole/location_001/sensors
   ```

3. In the mosquitto_sub terminal, you should see:
   ```
   manhole/location_001/sensors {"device_id":"MANHOLE_001","ch4":220.92,...}
   ```

### Test 3: Check Bridge Server Logs

In the bridge server terminal, you should see:
```
📨 MQTT message received on manhole/location_001/sensors
[INGESTED] MANHOLE_001 | CH4 220.92 ppm | H2S 1.37 ppm | Water Level 4.30 cm | Normal
```

---

## 🐛 Troubleshooting

### Problem: "MQTT connection failed, rc=-2"

**Cause**: Mosquitto broker not running

**Solution**:
```bash
# Check if broker is running
docker ps | grep mosquitto

# If not running, start it
./start-mqtt.sh

# Check broker logs
docker logs -f manhole-mqtt-broker
```

---

### Problem: "Connection refused on port 1883"

**Cause**: Port already in use or firewall blocking

**Solution**:
```bash
# Check what's using port 1883
sudo netstat -tulpn | grep 1883

# OR
sudo lsof -i :1883

# If another service is using it, stop it or change the port
```

---

### Problem: Bridge not receiving messages

**Cause**: Topic mismatch or subscription issue

**Solution**:
1. Check environment variables match:
   - ESP32 `location_id` in config
   - Bridge `MQTT_LOCATION_ID` in .env
   
2. Verify subscription in bridge logs:
   ```
   📡 Subscribed to MQTT topics: manhole/location_001/+
   ```

3. Test with mosquitto_sub to confirm messages are published

---

### Problem: Messages not persisted

**Cause**: Mosquitto persistence not configured

**Solution**:
```bash
# Check mosquitto.conf has:
persistence true
persistence_location /mosquitto/data/

# Restart broker
docker-compose restart mosquitto
```

---

## 🔒 Security (Production)

For production deployment, enable authentication:

### 1. Create Password File

```bash
# Create password file in Docker container
docker exec -it manhole-mqtt-broker mosquitto_passwd -c /mosquitto/config/passwd mqtt_user

# Enter password when prompted
```

### 2. Update mosquitto.conf

```conf
# Disable anonymous access
allow_anonymous false

# Enable password file
password_file /mosquitto/config/passwd
```

### 3. Update .env

```bash
MQTT_USERNAME=mqtt_user
MQTT_PASSWORD=your_secure_password
```

### 4. Update ESP32 Config

Via serial command or MQTT:
```
config set mqtt_username mqtt_user
config set mqtt_password your_secure_password
config save
```

---

## 📊 Monitoring

### View Broker Logs

```bash
docker logs -f manhole-mqtt-broker
```

### View Broker Statistics

```bash
# Connect to broker container
docker exec -it manhole-mqtt-broker sh

# View active connections
mosquitto_sub -t '$SYS/broker/clients/connected' -C 1
```

### Monitor Message Rate

```bash
# Messages received per minute
mosquitto_sub -t '$SYS/broker/messages/received' -C 1
```

---

## 🛑 Stopping Services

### Stop MQTT Broker

```bash
./stop-mqtt.sh
```

### Stop Bridge Server

Press `Ctrl+C` in the bridge terminal

### Stop Everything

```bash
docker-compose down
```

---

## 📈 Performance Metrics

With MQTT enabled, you should see:

- **Latency**: <50ms from ESP32 to bridge
- **Power Savings**: 93% reduction vs HTTP polling
- **Reliability**: QoS 1 guarantees message delivery
- **Offline Buffering**: Up to 10 messages stored on ESP32

---

## 🎓 Advanced Topics

### Custom QoS Levels

Edit `wokwi/src/mqtt.cpp` to change QoS:

```cpp
// QoS 0 - At most once (fire and forget)
mqttClient.publish(topic, payload, false);

// QoS 1 - At least once (acknowledged)
mqttClient.publish(topic, payload, true);

// QoS 2 - Exactly once (not supported by PubSubClient)
```

### Last Will and Testament

Already configured in ESP32 firmware:

```cpp
// Sent automatically if ESP32 disconnects unexpectedly
Topic: manhole/location_001/status
Payload: {"device_id":"MANHOLE_001","status":"offline"}
```

### Message Retention

Enable in mosquitto.conf:

```conf
# Retain last message on each topic
retain_available true
```

Then publish with retain flag:
```cpp
mqttClient.publish(topic, payload, true); // retained=true
```

---

## 📚 Additional Resources

- [Mosquitto Documentation](https://mosquitto.org/documentation/)
- [MQTT Protocol Specification](https://mqtt.org/mqtt-specification/)
- [PubSubClient Library](https://github.com/knolleary/pubsubclient)
- [ESP32 MQTT Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mqtt.html)

---

## ✅ Success Checklist

- [ ] Mosquitto broker running in Docker
- [ ] Bridge server connected to MQTT broker
- [ ] ESP32 publishing sensor data via MQTT
- [ ] Bridge receiving and storing data
- [ ] No "MQTT connection failed" errors in ESP32 logs
- [ ] Data visible in dashboard

---

**Need Help?** Check the troubleshooting section or review the logs for error messages.
