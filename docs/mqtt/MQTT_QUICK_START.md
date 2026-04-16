# MQTT Quick Start Guide

## 🚀 Get MQTT Running in 3 Commands

### 1. Start Everything
```bash
cd wokwi
./start-all.sh
```

### 2. Start Bridge Server
```bash
npm run bridge
```

### 3. Start Wokwi Simulator
- Open Wokwi VS Code extension
- Click "Start Simulation"

---

## ✅ Verify It's Working

You should see in the **ESP32 serial output**:
```
✅ MQTT connected to localhost:1883
📡 Published to manhole/location_001/sensors
```

You should see in the **Bridge server logs**:
```
✅ MQTT client connected to mqtt://localhost:1883
📡 Subscribed to MQTT topics: manhole/location_001/+
📨 MQTT message received on manhole/location_001/sensors
[INGESTED] MANHOLE_001 | CH4 220.92 ppm | ...
```

---

## 🔍 Monitor MQTT Traffic

Watch all messages in real-time:
```bash
docker exec -it manhole-mqtt-broker mosquitto_sub -t 'manhole/#' -v
```

---

## 🧪 Test MQTT Broker

```bash
./test-mqtt.sh
```

---

## 🛑 Stop Everything

```bash
./stop-all.sh
```

---

## 📚 Need More Details?

See the complete guide: **MQTT_SETUP_GUIDE.md**

---

## 🐛 Troubleshooting

### Problem: "MQTT connection failed"

**Solution:**
```bash
# Check if broker is running
docker ps | grep mosquitto

# If not running
./start-mqtt.sh

# Check logs
docker logs manhole-mqtt-broker
```

### Problem: Bridge not receiving messages

**Solution:**
1. Check `.env` file has correct MQTT settings
2. Verify ESP32 and bridge use same `location_id`
3. Monitor MQTT traffic to see if messages are published

---

## 📊 What's Different from HTTP?

| Feature | HTTP (Old) | MQTT (New) |
|---------|-----------|-----------|
| Protocol | Request/Response | Publish/Subscribe |
| Power Usage | High (polling) | Low (push) |
| Latency | ~1 second | <50ms |
| Offline Support | None | 10 message buffer |
| Reliability | Best effort | QoS guarantees |

---

## 🎯 MQTT Topics Used

```
manhole/location_001/sensors      - Sensor readings (QoS 1)
manhole/location_001/alerts       - Alert notifications (QoS 1)
manhole/location_001/diagnostics  - System diagnostics (QoS 0)
manhole/location_001/config       - Configuration updates (QoS 2)
manhole/location_001/status       - Device status (Last Will)
```

---

**That's it! You're now using MQTT for real-time IoT communication! 🎉**
