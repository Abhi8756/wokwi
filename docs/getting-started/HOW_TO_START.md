# 🚀 How to Start Your IoT System

## The Easy Way (Recommended)

### Step 1: Open Command Palette
Press `Ctrl+Shift+P` (Windows/Linux) or `Cmd+Shift+P` (Mac)

### Step 2: Run Task
Type: `Tasks: Run Task`

### Step 3: Select Task
Choose: **`System: Start Complete Stack (with MQTT)`**

### Step 4: Start Wokwi
- Open Wokwi extension
- Click "Start Simulation"

### Done! ✅

---

## What Just Happened?

VS Code automatically started:

```
┌─────────────────────────────────────┐
│ 1. MQTT Broker (Mosquitto)         │
│    Port: 1883                       │
│    Status: Running in Docker        │
└─────────────────────────────────────┘
              ↓
┌─────────────────────────────────────┐
│ 2. Bridge Server                    │
│    Port: 3101                       │
│    Status: Connected to MQTT        │
└─────────────────────────────────────┘
              ↓
┌─────────────────────────────────────┐
│ 3. GUI Serial Bridge                │
│    Status: Waiting for Wokwi        │
└─────────────────────────────────────┘
              ↓
┌─────────────────────────────────────┐
│ 4. Dashboard Dev Server             │
│    Port: 3000                       │
│    URL: http://localhost:3000       │
└─────────────────────────────────────┘
```

---

## Verify It's Working

### Check Terminal Panels

You should see 4 terminal panels at the bottom of VS Code:

**Panel 1: MQTT Broker**
```
✅ MQTT Broker started on port 1883
```

**Panel 2: Bridge Server**
```
✅ MQTT client connected to mqtt://localhost:1883
📡 Subscribed to MQTT topics: manhole/location_001/+
🌐 Bridge server listening on port 3101
```

**Panel 3: GUI Serial Bridge**
```
Connected to Wokwi RFC2217 serial server
```

**Panel 4: Dashboard**
```
VITE ready in XXX ms
Local: http://localhost:3000/
```

### Check ESP32 Serial Output

After starting Wokwi, you should see:
```
✅ MQTT connected to localhost:1883
📡 Published to manhole/location_001/sensors
{"device_id":"MANHOLE_001","ch4":220.92,...}
```

### Check Dashboard

Open browser to: `http://localhost:3000`

You should see real-time sensor data updating.

---

## Optional: Monitor MQTT Traffic

Want to see what's happening on MQTT?

1. `Ctrl+Shift+P`
2. `Tasks: Run Task`
3. `MQTT: Monitor Traffic`

You'll see all MQTT messages:
```
manhole/location_001/sensors {"device_id":"MANHOLE_001",...}
manhole/location_001/alerts {"alert_type":"CH4_WARNING",...}
```

---

## How to Stop

### Quick Stop
1. Stop Wokwi Simulator (click "Stop Simulation")
2. `Ctrl+Shift+P` → `Tasks: Run Task` → `System: Stop All Services`
3. Press `Ctrl+C` in Bridge and Dashboard terminals

### Or Just Close VS Code
All services will stop automatically when you close VS Code.

---

## The Old Way (Manual)

If you prefer manual control:

### Terminal 1: MQTT Broker
```bash
cd wokwi
./start-mqtt.sh
```

### Terminal 2: Bridge Server
```bash
cd wokwi
npm run bridge
```

### Terminal 3: GUI Serial Bridge
```bash
cd wokwi
npm run watch:gui
```

### Terminal 4: Dashboard
```bash
cd sewerly
npm run dev
```

### Terminal 5: Wokwi
Start Wokwi simulator in VS Code

---

## Troubleshooting

### Nothing happens when I run the task

**Check**: Is Docker running?
```bash
docker info
```

If not, start Docker Desktop.

---

### "Port 1883 already in use"

**Check**: Is another MQTT broker running?
```bash
sudo lsof -i :1883
```

Stop the other broker or change the port.

---

### Bridge can't connect to MQTT

**Check**: Did MQTT broker start successfully?

Look at the MQTT Broker terminal panel. You should see:
```
✅ MQTT Broker started on port 1883
```

If not, check Docker logs:
```bash
docker logs manhole-mqtt-broker
```

---

### ESP32 shows "MQTT connection failed"

**Check**: Is MQTT broker running?
```bash
docker ps | grep mosquitto
```

If not running:
```
Ctrl+Shift+P → Tasks: Run Task → MQTT: Start Broker
```

---

## Quick Reference

| Action | Command |
|--------|---------|
| **Start Everything** | `Ctrl+Shift+P` → Tasks → `System: Start Complete Stack (with MQTT)` |
| **Stop Everything** | `Ctrl+Shift+P` → Tasks → `System: Stop All Services` |
| **Monitor MQTT** | `Ctrl+Shift+P` → Tasks → `MQTT: Monitor Traffic` |
| **View Logs** | `Ctrl+Shift+P` → Tasks → `MQTT: View Broker Logs` |

---

## What's Next?

1. ✅ System is running
2. ✅ MQTT is working
3. ✅ Data is flowing

Now you can:
- Develop new features
- Test MQTT communication
- Monitor sensor data
- Debug issues

---

## Documentation

- **This Guide**: `HOW_TO_START.md` (you are here)
- **Quick Reference**: `TASKS_QUICK_REFERENCE.md`
- **Complete Guide**: `VSCODE_TASKS_GUIDE.md`
- **MQTT Setup**: `MQTT_SETUP_GUIDE.md`

---

**Remember**: One command starts everything!

```
Ctrl+Shift+P → Tasks: Run Task → System: Start Complete Stack (with MQTT)
```

**Happy developing! 🎉**
