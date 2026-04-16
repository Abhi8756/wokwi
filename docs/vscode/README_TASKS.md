# ✅ VS Code Tasks Configured!

Your VS Code workspace now has integrated tasks for starting/stopping all services including MQTT.

---

## 🎯 Quick Start

### Start Everything with One Command

1. Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on Mac)
2. Type: `Tasks: Run Task`
3. Select: **`System: Start Complete Stack (with MQTT)`**

This automatically starts:
- ✅ MQTT Broker (Mosquitto on port 1883)
- ✅ Bridge Server (port 3101)
- ✅ GUI Serial Bridge
- ✅ Dashboard Dev Server (port 3000)

Then just **start Wokwi simulator** and you're ready!

---

## 📋 What Was Added

### New MQTT Tasks

1. **MQTT: Start Broker** - Start Mosquitto MQTT broker
2. **MQTT: Stop Broker** - Stop Mosquitto MQTT broker
3. **MQTT: Monitor Traffic** - Watch all MQTT messages in real-time
4. **MQTT: View Broker Logs** - View Mosquitto broker logs

### New System Tasks

5. **System: Start Complete Stack (with MQTT)** - Start everything including MQTT
6. **System: Stop All Services** - Stop MQTT and Docker containers

### Existing Tasks (Updated)

- Wokwi: Bridge
- Wokwi: GUI Serial Bridge
- Dashboard: Dev Server
- System: Start GUI Slider Stack

---

## 🚀 How to Use

### Access Tasks Menu

**Method 1**: Command Palette
- Press `Ctrl+Shift+P`
- Type: `Tasks: Run Task`
- Select your task

**Method 2**: Terminal Menu
- Click Terminal → Run Task...
- Select your task

**Method 3**: Keyboard Shortcut (Optional)
- Set up custom shortcuts in Keyboard Shortcuts settings

---

## 📊 Task Organization

Tasks are organized into groups:

```
📡 MQTT Tasks
   ├── Start Broker
   ├── Stop Broker
   ├── Monitor Traffic
   └── View Broker Logs

🔧 Backend Tasks
   ├── Wokwi: Bridge
   └── Wokwi: GUI Serial Bridge

🎨 Frontend Tasks
   └── Dashboard: Dev Server

🎯 System Tasks
   ├── Start GUI Slider Stack
   ├── Start Complete Stack (with MQTT) ⭐
   └── Stop All Services
```

---

## 🎬 Typical Development Session

### 1. Start Your Day
```
Ctrl+Shift+P → Tasks: Run Task → System: Start Complete Stack (with MQTT)
```

Wait for all services to start (you'll see 4 terminal panels).

### 2. Start Wokwi Simulator
- Open Wokwi VS Code extension
- Click "Start Simulation"

### 3. Open Dashboard
- Browser: `http://localhost:3000`

### 4. Monitor MQTT (Optional)
```
Ctrl+Shift+P → Tasks: Run Task → MQTT: Monitor Traffic
```

### 5. End Your Day
```
Ctrl+Shift+P → Tasks: Run Task → System: Stop All Services
```

Then press `Ctrl+C` in Bridge and Dashboard terminals.

---

## 🔍 What You'll See

### Terminal Panels

VS Code will create separate terminal panels:

1. **MQTT Broker** - Mosquitto startup messages
2. **Bridge Server** - MQTT connection, data ingestion logs
3. **GUI Serial Bridge** - Serial port connection
4. **Dashboard Dev Server** - Vite dev server
5. **MQTT Monitor** (if running) - Real-time MQTT messages

### Success Messages

**MQTT Broker:**
```
✅ MQTT Broker started on port 1883
```

**Bridge Server:**
```
✅ MQTT client connected to mqtt://localhost:1883
📡 Subscribed to MQTT topics: manhole/location_001/+
🌐 Bridge server listening on port 3101
```

**ESP32 (Wokwi):**
```
✅ MQTT connected to localhost:1883
📡 Published to manhole/location_001/sensors
```

---

## 💡 Pro Tips

### 1. Leave Services Running
Start the complete stack once in the morning, then just start/stop Wokwi simulator as needed. The backend services can stay running all day.

### 2. Use MQTT Monitor for Debugging
The MQTT Monitor task shows you exactly what messages are being published. Great for debugging communication issues.

### 3. Check Logs When Things Go Wrong
Use "MQTT: View Broker Logs" to see what's happening with the broker.

### 4. Switch Between Terminals
Use the terminal dropdown menu to switch between different service panels.

### 5. Create Keyboard Shortcuts
Add custom shortcuts for frequently used tasks (see `VSCODE_TASKS_GUIDE.md`).

---

## 🐛 Troubleshooting

### Problem: "MQTT: Start Broker" fails

**Cause**: Docker not running

**Solution**:
```bash
# Check Docker status
docker info

# Start Docker Desktop if needed
```

---

### Problem: Port 1883 already in use

**Cause**: Another MQTT broker running

**Solution**:
```bash
# Find what's using the port
sudo lsof -i :1883

# Stop the other service or change port in docker-compose.yml
```

---

### Problem: Tasks don't appear in menu

**Cause**: VS Code hasn't loaded tasks.json

**Solution**:
- Reload VS Code: `Ctrl+Shift+P` → `Developer: Reload Window`

---

### Problem: Bridge can't connect to MQTT

**Cause**: MQTT broker not started or crashed

**Solution**:
1. Check if broker is running: `docker ps | grep mosquitto`
2. View broker logs: Run "MQTT: View Broker Logs" task
3. Restart broker: Run "MQTT: Stop Broker" then "MQTT: Start Broker"

---

## 📚 Documentation

- **Quick Reference**: `TASKS_QUICK_REFERENCE.md` - One-page cheat sheet
- **Complete Guide**: `VSCODE_TASKS_GUIDE.md` - Detailed task documentation
- **MQTT Setup**: `MQTT_SETUP_GUIDE.md` - MQTT configuration guide
- **Architecture**: `MQTT_ARCHITECTURE.md` - System architecture details

---

## 🎯 Task Comparison

### Before (Manual)
```bash
# Terminal 1
./start-mqtt.sh

# Terminal 2
npm run bridge

# Terminal 3
npm run watch:gui

# Terminal 4
cd ../sewerly && npm run dev
```

### After (Automated)
```
Ctrl+Shift+P → Tasks: Run Task → System: Start Complete Stack (with MQTT)
```

**One command starts everything!** 🎉

---

## ✅ Benefits

- ✅ **One-command startup** - No more juggling multiple terminals
- ✅ **Organized panels** - Each service in its own terminal
- ✅ **Integrated monitoring** - MQTT traffic and logs accessible from VS Code
- ✅ **Consistent environment** - Same startup process for everyone
- ✅ **Easy debugging** - All logs in one place
- ✅ **Professional workflow** - Industry-standard task automation

---

## 🚀 You're Ready!

Your development environment is now fully configured with:

- ✅ MQTT broker (Mosquitto)
- ✅ Bridge server with MQTT support
- ✅ Dashboard with WebSocket
- ✅ VS Code tasks for automation
- ✅ Complete documentation

**Start developing with one command!**

```
Ctrl+Shift+P → Tasks: Run Task → System: Start Complete Stack (with MQTT)
```

---

**Happy coding! 🎉**
