# 🚀 VS Code Tasks Quick Reference

## One Command to Start Everything

```
Ctrl+Shift+P → Tasks: Run Task → System: Start Complete Stack (with MQTT)
```

This starts:
- ✅ MQTT Broker (port 1883)
- ✅ Bridge Server (port 3101)
- ✅ GUI Serial Bridge
- ✅ Dashboard (port 3000)

Then just **Start Wokwi Simulator** and you're ready! 🎉

---

## All Available Tasks

### 🔥 Most Used

| Task | Shortcut |
|------|----------|
| **Start Complete Stack (with MQTT)** | `Ctrl+Shift+P` → Tasks → `System: Start Complete Stack (with MQTT)` |
| **Stop All Services** | `Ctrl+Shift+P` → Tasks → `System: Stop All Services` |
| **Monitor MQTT Traffic** | `Ctrl+Shift+P` → Tasks → `MQTT: Monitor Traffic` |

---

### 📡 MQTT Tasks

| Task | What It Does |
|------|-------------|
| **MQTT: Start Broker** | Start Mosquitto (port 1883) |
| **MQTT: Stop Broker** | Stop Mosquitto |
| **MQTT: Monitor Traffic** | Watch all MQTT messages |
| **MQTT: View Broker Logs** | View broker logs |

---

### 🔧 Backend Tasks

| Task | What It Does |
|------|-------------|
| **Wokwi: Bridge** | Start bridge server (port 3101) |
| **Wokwi: GUI Serial Bridge** | Connect to Wokwi GUI serial |

---

### 🎨 Frontend Tasks

| Task | What It Does |
|------|-------------|
| **Dashboard: Dev Server** | Start React dashboard (port 3000) |

---

### 🎯 System Tasks

| Task | What It Does |
|------|-------------|
| **Start GUI Slider Stack** | Bridge + Serial + Dashboard (no MQTT) |
| **Start Complete Stack (with MQTT)** | Everything including MQTT ⭐ |
| **Stop All Services** | Stop MQTT and Docker |

---

## 📋 Typical Workflow

### Morning (Start Development)
1. `Ctrl+Shift+P` → Tasks → `System: Start Complete Stack (with MQTT)`
2. Start Wokwi Simulator
3. Open browser to `http://localhost:3000`

### During Development
- Monitor MQTT: `Ctrl+Shift+P` → Tasks → `MQTT: Monitor Traffic`
- Check logs: `Ctrl+Shift+P` → Tasks → `MQTT: View Broker Logs`

### Evening (Stop Development)
1. Stop Wokwi Simulator
2. `Ctrl+Shift+P` → Tasks → `System: Stop All Services`
3. Press `Ctrl+C` in Bridge/Dashboard terminals

---

## 🎯 What You'll See

### MQTT Broker Terminal
```
✅ MQTT Broker started on port 1883
```

### Bridge Server Terminal
```
✅ MQTT client connected to mqtt://localhost:1883
📡 Subscribed to MQTT topics: manhole/location_001/+
🌐 Bridge server listening on port 3101
```

### MQTT Monitor Terminal
```
manhole/location_001/sensors {"device_id":"MANHOLE_001","ch4":220.92,...}
manhole/location_001/alerts {"device_id":"MANHOLE_001","alert_type":"CH4_WARNING",...}
```

---

## 💡 Pro Tips

1. **Leave services running**: Start the stack once, then just start/stop Wokwi simulator as needed
2. **Use MQTT Monitor**: Great for debugging - see exactly what's being published
3. **Check broker logs**: If MQTT isn't working, check the broker logs first
4. **Terminal panels**: Use the dropdown to switch between different service terminals

---

## 🐛 Quick Troubleshooting

| Problem | Solution |
|---------|----------|
| MQTT won't start | Check Docker is running: `docker info` |
| Port 1883 in use | Stop other MQTT broker or change port |
| Tasks don't appear | Reload VS Code: `Ctrl+Shift+P` → `Developer: Reload Window` |
| Bridge can't connect | Make sure MQTT broker started successfully |

---

## 📚 More Info

- **Complete Guide**: `VSCODE_TASKS_GUIDE.md`
- **MQTT Setup**: `MQTT_SETUP_GUIDE.md`
- **Quick Start**: `MQTT_QUICK_START.md`

---

**Remember**: `System: Start Complete Stack (with MQTT)` is your friend! 🚀
