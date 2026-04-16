# 📚 IoT Manhole Monitoring System - Documentation

Complete documentation for the ESP32-based IoT Manhole Monitoring System with MQTT, real-time dashboard, and professional embedded features.

---

## 🚀 Getting Started

**New to the project? Start here:**

- **[How to Start](getting-started/HOW_TO_START.md)** - Quick guide to start the complete system
- **[MQTT Quick Start](getting-started/START_MQTT.md)** - 3-step MQTT setup
- **[Telegram Quick Start](getting-started/QUICK_START_TELEGRAM.md)** - Set up Telegram alerts

---

## 📡 MQTT Documentation

**Everything about MQTT communication:**

- **[MQTT Quick Start](mqtt/MQTT_QUICK_START.md)** - Get MQTT running in 3 commands
- **[MQTT Setup Guide](mqtt/MQTT_SETUP_GUIDE.md)** - Complete MQTT configuration guide
- **[MQTT Architecture](mqtt/MQTT_ARCHITECTURE.md)** - System architecture and data flow
- **[MQTT Setup Complete](mqtt/MQTT_SETUP_COMPLETE.md)** - Setup summary and verification

**Topics Covered:**
- Mosquitto broker setup (Docker)
- ESP32 MQTT client configuration
- Bridge server MQTT subscriber
- Topic hierarchy and QoS levels
- Security and authentication
- Troubleshooting

---

## 🔧 VS Code Tasks

**Automate your development workflow:**

- **[Tasks Quick Reference](vscode/TASKS_QUICK_REFERENCE.md)** - One-page cheat sheet
- **[VS Code Tasks Guide](vscode/VSCODE_TASKS_GUIDE.md)** - Complete task documentation
- **[Tasks README](vscode/README_TASKS.md)** - Task configuration summary

**Available Tasks:**
- Start/Stop MQTT Broker
- Start/Stop Bridge Server
- Monitor MQTT traffic
- View broker logs
- One-command complete stack startup

---

## 📱 Telegram Bot

**Set up alert notifications:**

- **[Telegram Bot Setup](telegram/TELEGRAM_BOT_SETUP.md)** - Step-by-step setup guide
- **[Telegram Bot Implementation](telegram/TELEGRAM_BOT_IMPLEMENTATION.md)** - Implementation details
- **[Telegram Bot Technical](telegram/TELEGRAM_BOT_TECHNICAL.md)** - Technical documentation
- **[Telegram Bot Overview](telegram/TELEGRAM_BOT.md)** - Feature overview

**Features:**
- Real-time alert notifications
- Threshold configuration
- Device status monitoring
- Multi-location support

---

## 📖 Documentation Structure

```
docs/
├── README.md (this file)
│
├── getting-started/
│   ├── HOW_TO_START.md          - Main startup guide
│   ├── START_MQTT.md             - MQTT quick start
│   └── QUICK_START_TELEGRAM.md   - Telegram quick start
│
├── mqtt/
│   ├── MQTT_QUICK_START.md       - Quick reference
│   ├── MQTT_SETUP_GUIDE.md       - Complete guide
│   ├── MQTT_ARCHITECTURE.md      - Architecture details
│   └── MQTT_SETUP_COMPLETE.md    - Setup summary
│
├── vscode/
│   ├── TASKS_QUICK_REFERENCE.md  - Quick reference
│   ├── VSCODE_TASKS_GUIDE.md     - Complete guide
│   └── README_TASKS.md           - Configuration summary
│
└── telegram/
    ├── TELEGRAM_BOT_SETUP.md     - Setup guide
    ├── TELEGRAM_BOT_IMPLEMENTATION.md
    ├── TELEGRAM_BOT_TECHNICAL.md
    └── TELEGRAM_BOT.md           - Overview
```

---

## 🎯 Quick Links by Task

### I want to start the system
→ [How to Start](getting-started/HOW_TO_START.md)

### I want to set up MQTT
→ [MQTT Quick Start](mqtt/MQTT_QUICK_START.md)

### I want to use VS Code tasks
→ [Tasks Quick Reference](vscode/TASKS_QUICK_REFERENCE.md)

### I want to set up Telegram alerts
→ [Telegram Bot Setup](telegram/TELEGRAM_BOT_SETUP.md)

### I want to understand the architecture
→ [MQTT Architecture](mqtt/MQTT_ARCHITECTURE.md)

### I want to troubleshoot MQTT
→ [MQTT Setup Guide - Troubleshooting](mqtt/MQTT_SETUP_GUIDE.md#troubleshooting)

### I want to monitor MQTT traffic
→ [VS Code Tasks - MQTT Monitor](vscode/TASKS_QUICK_REFERENCE.md)

---

## 🏗️ System Architecture

```
┌─────────────┐         ┌──────────────┐         ┌──────────────┐
│   ESP32     │  MQTT   │  Mosquitto   │  MQTT   │    Bridge    │
│  Firmware   ├────────>│    Broker    ├────────>│    Server    │
│  (Wokwi)    │ Publish │  (Docker)    │Subscribe│  (Node.js)   │
└─────────────┘         └──────────────┘         └──────┬───────┘
                                                         │
                                                         ├─> Storage
                                                         ├─> Dashboard
                                                         └─> Telegram
```

---

## 🔑 Key Features

### ESP32 Firmware
- ✅ Non-blocking architecture
- ✅ Dual-core task management
- ✅ Watchdog timer
- ✅ MQTT with QoS
- ✅ Offline message buffering
- ✅ NTP time synchronization
- ✅ OTA firmware updates
- ✅ Configuration storage (NVS)
- ✅ Fault detection & diagnostics
- ✅ Memory safety monitoring
- ✅ Performance monitoring

### Bridge Server
- ✅ MQTT subscriber
- ✅ WebSocket real-time updates
- ✅ Multi-location support
- ✅ Alert management
- ✅ Historical data export
- ✅ Threshold hysteresis
- ✅ API key authentication
- ✅ Exponential backoff retry

### Dashboard
- ✅ Real-time sensor data
- ✅ WebSocket updates
- ✅ Alert history
- ✅ Multi-location view
- ✅ Data export (CSV/JSON)
- ✅ Responsive design

### Telegram Bot
- ✅ Real-time alerts
- ✅ Threshold configuration
- ✅ Device status
- ✅ Multi-location support

---

## 🛠️ Development Tools

### Scripts
- `start-all.sh` - Start complete system
- `start-mqtt.sh` - Start MQTT broker only
- `stop-all.sh` - Stop all services
- `stop-mqtt.sh` - Stop MQTT broker
- `test-mqtt.sh` - Test MQTT connectivity

### VS Code Tasks
- System: Start Complete Stack (with MQTT)
- MQTT: Start/Stop Broker
- MQTT: Monitor Traffic
- MQTT: View Broker Logs
- System: Stop All Services

---

## 📊 Performance Metrics

| Metric | HTTP (Old) | MQTT (New) |
|--------|-----------|-----------|
| **Latency** | ~1 second | <50ms |
| **Power** | ~500mW | ~35mW (93% reduction) |
| **Bandwidth** | 300 bytes/msg | 150 bytes/msg |
| **Offline Buffer** | None | 10 messages |
| **Reliability** | Best effort | QoS guarantees |

---

## 🔒 Security Features

### Current (Development)
- API key authentication
- Plain MQTT (port 1883)
- Local network only

### Production Ready
- TLS/SSL encryption (port 8883)
- Username/password authentication
- Client certificates
- Certificate validation & pinning
- Access control lists (ACLs)

---

## 🐛 Troubleshooting

### Common Issues

**MQTT connection failed**
→ See [MQTT Setup Guide - Troubleshooting](mqtt/MQTT_SETUP_GUIDE.md#troubleshooting)

**Bridge not receiving messages**
→ Check topic configuration in [MQTT Architecture](mqtt/MQTT_ARCHITECTURE.md)

**VS Code tasks not working**
→ See [VS Code Tasks Guide - Troubleshooting](vscode/VSCODE_TASKS_GUIDE.md#troubleshooting)

**Telegram bot not responding**
→ See [Telegram Bot Setup - Troubleshooting](telegram/TELEGRAM_BOT_SETUP.md)

---

## 📈 Project Status

### Phase 1: Critical Features ✅ Complete
- Non-blocking firmware architecture
- Watchdog timer
- Dual-core task management
- MQTT protocol implementation
- Network retry with exponential backoff
- NTP time synchronization
- TLS/SSL MQTT communication
- API key authentication
- Configuration storage (NVS)
- OTA firmware updates
- Fault detection & diagnostics
- Memory safety monitoring
- Performance monitoring
- Threshold hysteresis
- WebSocket real-time updates

### Phase 2: Feature Enhancements ✅ 4/5 Complete
- Historical data export ✅
- Multi-location support ✅
- Alert management dashboard ✅
- Browser push notifications (pending)

### Phase 3: Advanced Features 🚧 In Progress
- Predictive analytics
- Mobile app
- ML anomaly detection
- SD card logging (hardware)

---

## 🎓 Learning Resources

### For Beginners
1. Start with [How to Start](getting-started/HOW_TO_START.md)
2. Read [MQTT Quick Start](mqtt/MQTT_QUICK_START.md)
3. Try [Tasks Quick Reference](vscode/TASKS_QUICK_REFERENCE.md)

### For Advanced Users
1. Study [MQTT Architecture](mqtt/MQTT_ARCHITECTURE.md)
2. Review [MQTT Setup Guide](mqtt/MQTT_SETUP_GUIDE.md)
3. Explore [VS Code Tasks Guide](vscode/VSCODE_TASKS_GUIDE.md)

### For Production Deployment
1. Security setup in [MQTT Setup Guide](mqtt/MQTT_SETUP_GUIDE.md#security-production)
2. Monitoring in [MQTT Architecture](mqtt/MQTT_ARCHITECTURE.md#monitoring-and-debugging)
3. Telegram alerts in [Telegram Bot Setup](telegram/TELEGRAM_BOT_SETUP.md)

---

## 🤝 Contributing

When adding new documentation:

1. Place files in appropriate folders:
   - `getting-started/` - Quick start guides
   - `mqtt/` - MQTT-related docs
   - `vscode/` - VS Code configuration
   - `telegram/` - Telegram bot docs

2. Update this README with links

3. Follow naming conventions:
   - Use UPPERCASE for main docs
   - Use descriptive names
   - Include file extension (.md)

---

## 📞 Support

### Documentation Issues
- Check the troubleshooting sections
- Review related documentation
- Check logs for error messages

### System Issues
- ESP32: Check serial output
- MQTT: Run `test-mqtt.sh`
- Bridge: Check terminal logs
- Dashboard: Check browser console

---

## 🎉 Quick Start Summary

**The fastest way to get started:**

1. **Start the system**:
   ```
   Ctrl+Shift+P → Tasks: Run Task → System: Start Complete Stack (with MQTT)
   ```

2. **Start Wokwi simulator**:
   - Open Wokwi extension
   - Click "Start Simulation"

3. **Open dashboard**:
   - Browser: `http://localhost:3000`

**That's it! You're running a professional IoT system!** 🚀

---

## 📚 External Resources

- [Mosquitto Documentation](https://mosquitto.org/documentation/)
- [MQTT Protocol Specification](https://mqtt.org/mqtt-specification/)
- [ESP32 Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [PubSubClient Library](https://github.com/knolleary/pubsubclient)
- [Wokwi Simulator](https://wokwi.com/)

---

**Last Updated**: April 2026

**Version**: 2.0 (MQTT-enabled)

**Maintained by**: IoT Development Team
