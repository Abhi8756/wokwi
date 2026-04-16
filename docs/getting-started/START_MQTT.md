# 🚀 Start MQTT in 3 Steps

## Step 1: Start MQTT Broker
```bash
cd wokwi
./start-all.sh
```

## Step 2: Start Bridge Server
```bash
npm run bridge
```

## Step 3: Start Wokwi Simulator
- Open Wokwi VS Code extension
- Click "Start Simulation"

---

## ✅ Verify It's Working

### ESP32 Serial Output Should Show:
```
✅ MQTT connected to localhost:1883
📡 Published to manhole/location_001/sensors
```

### Bridge Server Should Show:
```
✅ MQTT client connected to mqtt://localhost:1883
📨 MQTT message received
[INGESTED] MANHOLE_001 | CH4 220.92 ppm | ...
```

---

## 🔍 Monitor MQTT (Optional)
```bash
docker exec -it manhole-mqtt-broker mosquitto_sub -t 'manhole/#' -v
```

---

## 🛑 Stop Everything
```bash
./stop-all.sh
```

---

## 📚 More Info
- Quick Start: `MQTT_QUICK_START.md`
- Complete Guide: `MQTT_SETUP_GUIDE.md`
- Architecture: `MQTT_ARCHITECTURE.md`

---

**That's it! You're using MQTT! 🎉**
