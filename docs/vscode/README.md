# VS Code Tasks Documentation

Automate your development workflow with VS Code tasks.

---

## 📚 Available Documentation

### [Tasks Quick Reference](TASKS_QUICK_REFERENCE.md) ⭐
**One-page cheat sheet for all tasks.**

Perfect for:
- Quick lookup
- Daily reference
- Learning keyboard shortcuts

**Keep this open while developing!**

---

### [VS Code Tasks Guide](VSCODE_TASKS_GUIDE.md)
**Complete task documentation.**

Covers:
- All available tasks
- Task configuration
- Customization options
- Keyboard shortcuts
- Troubleshooting

**Read this for complete understanding!**

---

### [Tasks README](README_TASKS.md)
**Task configuration summary.**

Contains:
- What was added
- How to use tasks
- Task organization
- Benefits overview

**Read this to understand the setup!**

---

## 🎯 Available Tasks

### MQTT Tasks
- **MQTT: Start Broker** - Start Mosquitto
- **MQTT: Stop Broker** - Stop Mosquitto
- **MQTT: Monitor Traffic** - Watch MQTT messages
- **MQTT: View Broker Logs** - View broker logs

### Backend Tasks
- **Wokwi: Bridge** - Start bridge server
- **Wokwi: GUI Serial Bridge** - Connect to Wokwi serial

### Frontend Tasks
- **Dashboard: Dev Server** - Start React dashboard

### System Tasks
- **System: Start GUI Slider Stack** - Start without MQTT
- **System: Start Complete Stack (with MQTT)** ⭐ - Start everything
- **System: Stop All Services** - Stop all services

---

## 🚀 Quick Start

### Start Everything
```
Ctrl+Shift+P → Tasks: Run Task → System: Start Complete Stack (with MQTT)
```

### Stop Everything
```
Ctrl+Shift+P → Tasks: Run Task → System: Stop All Services
```

### Monitor MQTT
```
Ctrl+Shift+P → Tasks: Run Task → MQTT: Monitor Traffic
```

---

## 💡 Why Use Tasks?

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

## 🎨 Task Organization

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

## 📊 Terminal Panels

VS Code creates separate terminal panels:

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

Switch between panels using the dropdown menu.

---

## ⌨️ Keyboard Shortcuts

### Access Tasks
- `Ctrl+Shift+P` → Type `Tasks: Run Task`
- Or: Terminal menu → Run Task...

### Custom Shortcuts (Optional)
Add to Keyboard Shortcuts (JSON):

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

---

## 🔧 Customization

### Change Ports

Edit `.vscode/tasks.json`:

```json
{
  "label": "Wokwi: Bridge",
  "command": "BRIDGE_PORT=3101 npm run bridge",  // Change port here
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

---

## 🐛 Troubleshooting

### Tasks don't appear
**Solution**: Reload VS Code
```
Ctrl+Shift+P → Developer: Reload Window
```

---

### MQTT task fails
**Solution**: Check Docker is running
```bash
docker info
```

---

### Multiple terminals clutter view
**Solution**: 
- Use terminal dropdown to switch
- Close unused terminals
- Toggle terminal: `Ctrl+` ` (backtick)

---

## 🎯 Typical Workflow

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

## ✅ Benefits

- ✅ **One-command startup** - No manual terminal management
- ✅ **Organized panels** - Each service in its own terminal
- ✅ **Integrated monitoring** - MQTT traffic in VS Code
- ✅ **Consistent workflow** - Same process every time
- ✅ **Easy debugging** - All logs in one place
- ✅ **Professional setup** - Industry-standard automation

---

## 🔗 Related Documentation

- **[Getting Started](../getting-started/)** - Quick start guides
- **[MQTT Documentation](../mqtt/)** - MQTT setup and architecture
- **[Telegram Bot](../telegram/)** - Alert notifications
- **[Main Documentation](../README.md)** - Complete index

---

## 💡 Pro Tips

1. **Leave services running**: Start once, then just start/stop Wokwi as needed
2. **Use MQTT Monitor**: Great for debugging communication
3. **Check logs first**: Most issues show up in logs
4. **Create shortcuts**: Add keyboard shortcuts for frequent tasks
5. **Use task groups**: Tasks are organized by function

---

**Need help?** Check the complete guide or troubleshooting sections.
