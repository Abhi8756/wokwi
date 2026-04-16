# VS Code Tasks Guide

## 🚀 Quick Start - Run Everything with One Command

### Start Complete Stack (MQTT + Bridge + Dashboard)

1. Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on Mac)
2. Type: `Tasks: Run Task`
3. Select: **`System: Start Complete Stack (with MQTT)`**

This will automatically:
- ✅ Start MQTT Broker (Mosquitto)
- ✅ Start Bridge Server (port 3101)
- ✅ Start GUI Serial Bridge
- ✅ Start Dashboard Dev Server (port 3000)

---

## 📋 Available Tasks

### MQTT Tasks

#### 1. MQTT: Start Broker
- Starts Mosquitto MQTT broker in Docker
- Port: 1883
- **Shortcut**: `Ctrl+Shift+P` → `Tasks: Run Task` → `MQTT: Start Broker`

#### 2. MQTT: Stop Broker
- Stops Mosquitto MQTT broker
- **Shortcut**: `Ctrl+Shift+P` → `Tasks: Run Task` → `MQTT: Stop Broker`

#### 3. MQTT: Monitor Traffic
- Watch all MQTT messages in real-time
- Shows all topics: `manhole/#`
- **Shortcut**: `Ctrl+Shift+P` → `Tasks: Run Task` → `MQTT: Monitor Traffic`

#### 4. MQTT: View Broker Logs
- View Mosquitto broker logs
- **Shortcut**: `Ctrl+Shift+P` → `Tasks: Run Task` → `MQTT: View Broker Logs`

---

### Backend Tasks

#### 5. Wokwi: Bridge
- Starts the bridge server (port 3101)
- Connects to MQTT broker
- Ingests sensor data
- **Shortcut**: `Ctrl+Shift+P` → `Tasks: Run Task` → `Wokwi: Bridge`

#### 6. Wokwi: GUI Serial Bridge
- Connects to Wokwi GUI serial port
- Forwards data to bridge server
- **Shortcut**: `Ctrl+Shift+P` → `Tasks: Run Task` → `Wokwi: GUI Serial Bridge`

---

### Frontend Tasks

#### 7. Dashboard: Dev Server
- Starts React dashboard (port 3000)
- Hot reload enabled
- **Shortcut**: `Ctrl+Shift+P` → `Tasks: Run Task` → `Dashboard: Dev Server`

---

### System Tasks

#### 8. System: Start GUI Slider Stack
- Starts Bridge + GUI Serial + Dashboard (without MQTT)
- **Shortcut**: `Ctrl+Shift+P` → `Tasks: Run Task` → `System: Start GUI Slider Stack`

#### 9. System: Start Complete Stack (with MQTT) ⭐
- **RECOMMENDED**: Starts everything including MQTT
- **Shortcut**: `Ctrl+Shift+P` → `Tasks: Run Task` → `System: Start Complete Stack (with MQTT)`

#### 10. System: Stop All Services
- Stops MQTT broker and Docker containers
- **Note**: You still need to manually stop Bridge and Dashboard (Ctrl+C in terminals)
- **Shortcut**: `Ctrl+Shift+P` → `Tasks: Run Task` → `System: Stop All Services`

---

## 🎯 Recommended Workflow

### Starting Development

1. **Start Complete Stack**:
   - `Ctrl+Shift+P` → `Tasks: Run Task` → `System: Start Complete Stack (with MQTT)`

2. **Start Wokwi Simulator**:
   - Open Wokwi extension
   - Click "Start Simulation"

3. **Monitor MQTT (Optional)**:
   - `Ctrl+Shift+P` → `Tasks: Run Task` → `MQTT: Monitor Traffic`

### During Development

- **View MQTT Messages**: Use `MQTT: Monitor Traffic` task
- **Check Broker Logs**: Use `MQTT: View Broker Logs` task
- **Restart Bridge**: Stop with `Ctrl+C`, then run `Wokwi: Bridge` task again

### Stopping Development

1. **Stop Wokwi Simulator**: Click "Stop Simulation"
2. **Stop Bridge/Dashboard**: Press `Ctrl+C` in their terminals
3. **Stop MQTT**: Run `System: Stop All Services` task

---

## 🔧 Task Configuration

Tasks are configured in `.vscode/tasks.json`

### Task Groups

Tasks are organized into groups for better panel management:

- **mqtt**: MQTT-related tasks
- **backend**: Bridge and serial tasks
- **frontend**: Dashboard tasks

### Task Dependencies

The `System: Start Complete Stack (with MQTT)` task runs in this order:

1. Start MQTT Broker (sequential)
2. Then start in parallel:
   - Bridge Server
   - GUI Serial Bridge
   - Dashboard Dev Server

---

## 📊 Task Panels

VS Code will create separate terminal panels for each task:

```
┌─────────────────────────────────────┐
│ Terminal Panels                     │
├─────────────────────────────────────┤
│ 1. MQTT Broker                      │
│ 2. Bridge Server                    │
│ 3. GUI Serial Bridge                │
│ 4. Dashboard Dev Server             │
│ 5. MQTT Monitor (if running)        │
└─────────────────────────────────────┘
```

You can switch between panels using the dropdown in the terminal area.

---

## 🎨 Customization

### Change Ports

Edit `.vscode/tasks.json`:

```json
{
  "label": "Wokwi: Bridge",
  "command": "BRIDGE_PORT=3101 npm run bridge",  // Change 3101 to your port
  ...
}
```

### Add Environment Variables

```json
{
  "label": "Wokwi: Bridge",
  "command": "BRIDGE_PORT=3101 MQTT_BROKER=mqtt://localhost:1883 npm run bridge",
  ...
}
```

### Change MQTT Port

Edit `docker-compose.yml`:

```yaml
ports:
  - "1883:1883"  # Change first 1883 to your desired port
```

---

## 🐛 Troubleshooting

### Problem: "MQTT: Start Broker" fails

**Cause**: Docker not running or port already in use

**Solution**:
```bash
# Check Docker
docker info

# Check port 1883
sudo lsof -i :1883

# Stop conflicting service or change port
```

---

### Problem: Tasks don't appear in menu

**Cause**: VS Code hasn't loaded tasks.json

**Solution**:
1. Reload VS Code: `Ctrl+Shift+P` → `Developer: Reload Window`
2. Or close and reopen VS Code

---

### Problem: "System: Start Complete Stack" doesn't start MQTT

**Cause**: Docker not running

**Solution**:
1. Start Docker Desktop
2. Wait for Docker to fully start
3. Run the task again

---

### Problem: Multiple terminals clutter the view

**Solution**:
- Use the terminal dropdown to switch between panels
- Close unused terminals with the trash icon
- Use `Ctrl+` ` (backtick) to toggle terminal visibility

---

## 💡 Tips

### Keyboard Shortcuts

Create custom keyboard shortcuts for frequently used tasks:

1. `Ctrl+Shift+P` → `Preferences: Open Keyboard Shortcuts (JSON)`
2. Add:

```json
[
  {
    "key": "ctrl+shift+m",
    "command": "workbench.action.tasks.runTask",
    "args": "System: Start Complete Stack (with MQTT)"
  },
  {
    "key": "ctrl+shift+q",
    "command": "workbench.action.tasks.runTask",
    "args": "System: Stop All Services"
  }
]
```

### Task Status Bar

Add tasks to status bar for quick access:

Install extension: **Task Runner** or **Task Buttons**

---

## 📚 Related Documentation

- **MQTT Setup**: `MQTT_SETUP_GUIDE.md`
- **Quick Start**: `MQTT_QUICK_START.md`
- **Architecture**: `MQTT_ARCHITECTURE.md`

---

## ✅ Quick Reference

| Task | What It Does | When to Use |
|------|-------------|-------------|
| **Start Complete Stack** | Starts MQTT + Bridge + Dashboard | Beginning of dev session |
| **MQTT: Start Broker** | Starts MQTT only | When you need just MQTT |
| **MQTT: Monitor Traffic** | Watch MQTT messages | Debugging MQTT communication |
| **MQTT: View Broker Logs** | View broker logs | Troubleshooting MQTT issues |
| **Stop All Services** | Stops MQTT and Docker | End of dev session |

---

**Pro Tip**: Use `System: Start Complete Stack (with MQTT)` at the beginning of your dev session, then just start/stop Wokwi simulator as needed. The backend services can stay running! 🚀
