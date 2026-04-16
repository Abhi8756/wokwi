# Manhole Monitoring System

This folder contains the simulator-side pieces that feed the React dashboard in `../sewerly`.

## 📚 Documentation

**Complete documentation is available in the [`docs/`](docs/) folder.**

### Quick Links
- **[Getting Started Guide](docs/getting-started/HOW_TO_START.md)** - Start here!
- **[MQTT Quick Start](docs/mqtt/MQTT_QUICK_START.md)** - Set up MQTT in 3 commands
- **[VS Code Tasks](docs/vscode/TASKS_QUICK_REFERENCE.md)** - One-command startup
- **[Documentation Index](docs/README.md)** - Complete documentation index

---

## 🚀 Quick Start

### Option 1: Use VS Code Tasks (Recommended)

1. Press `Ctrl+Shift+P`
2. Type: `Tasks: Run Task`
3. Select: `System: Start Complete Stack (with MQTT)`
4. Start Wokwi Simulator

**See [How to Start Guide](docs/getting-started/HOW_TO_START.md) for details.**

### Option 2: Manual Start

```bash
# Terminal 1: Start MQTT Broker
./start-mqtt.sh

# Terminal 2: Start Bridge Server
npm run bridge

# Terminal 3: Start GUI Serial Bridge
npm run watch:gui

# Terminal 4: Start Dashboard
cd ../sewerly && npm run dev
```

Then start Wokwi Simulator in VS Code.

---

## Supported workflow

The supported live path is:

`Wokwi VS Code sliders -> RFC2217 serial bridge -> local bridge API -> dashboard`

**With MQTT enabled:**

`ESP32 (Wokwi) -> MQTT Broker (Mosquitto) -> Bridge Server -> Dashboard`

---

## Main files

- `src/main.cpp`
  Reads the three Wokwi sliders, smooths the values, and prints JSON sensor data once per second.
- `diagram.json`
  Defines the ESP32 and the slider controls in the Wokwi circuit.
- `wokwi.toml`
  Enables RFC2217 serial forwarding from the VS Code Wokwi simulator.
- `watch-rfc2217.js`
  Connects to the simulator serial stream and forwards sensor JSON to the local bridge.
- `bridge.js`
  Normalizes readings, stores them in Firestore when configured, and serves the API used by the dashboard. Includes MQTT subscriber for real-time data ingestion.
- `mosquitto.conf`
  MQTT broker configuration for Mosquitto.
- `docker-compose.yml`
  Docker setup for Mosquitto MQTT broker.

---

## Scripts

### Startup Scripts
- `start-all.sh` - Start complete system (MQTT + Bridge + checks)
- `start-mqtt.sh` - Start MQTT broker only
- `stop-all.sh` - Stop all services
- `stop-mqtt.sh` - Stop MQTT broker
- `test-mqtt.sh` - Test MQTT connectivity

### Telegram Bot Scripts
- `setup-telegram-bot.sh` / `.bat` - Set up Telegram bot

---

## How to run it

### Using VS Code Tasks (Recommended)

1. Open the `wokwi` folder in VS Code.
2. Press `Ctrl+Shift+P` → `Tasks: Run Task` → `System: Start Complete Stack (with MQTT)`.
3. Start the Wokwi simulator from the VS Code GUI.
4. Move the sliders and watch the dashboard update.

**See [VS Code Tasks Guide](docs/vscode/TASKS_QUICK_REFERENCE.md) for all available tasks.**

### Manual Method

1. Start MQTT broker: `./start-mqtt.sh`
2. Run the VS Code task `System: Start GUI Slider Stack`.
3. Start the Wokwi simulator from the VS Code GUI.
4. Move the sliders and watch the dashboard update.

---

## Important limitation

The Wokwi VS Code simulator tab must stay visible while using the GUI sliders. If you hide the tab, Wokwi may pause and stop sending serial output.

---

## Data storage

- Firestore mode:
  Set `SENSOR_STORAGE_BACKEND=firestore` and add Firebase admin credentials in `.env` or `serviceAccountKey.json`. In this mode, the bridge reads and writes from Firestore instead of using `sensor-readings.json` as the main store.
- Local fallback mode:
  If Firestore is not configured, the bridge falls back to `sensor-readings.json`.

---

## MQTT Communication

The system uses MQTT for real-time IoT communication:

- **Broker**: Mosquitto (Docker, port 1883)
- **Topics**: `manhole/{location_id}/{sensors|alerts|diagnostics|config|status}`
- **QoS**: Level 1 for critical data (sensors, alerts)
- **Features**: Offline buffering, Last Will and Testament, exponential backoff retry

**See [MQTT Documentation](docs/mqtt/) for complete setup and configuration.**

---

## Utility scripts kept on purpose

- `continuous-data.js`
  Sends a continuous sample stream to the bridge when you want to test without Wokwi.
- `test-data.js`
  Sends a single sample payload for quick verification.

---

## 📖 Documentation Structure

```
docs/
├── README.md                    - Documentation index
├── getting-started/             - Quick start guides
├── mqtt/                        - MQTT setup and architecture
├── vscode/                      - VS Code tasks documentation
└── telegram/                    - Telegram bot setup
```

**Browse all documentation: [docs/README.md](docs/README.md)**

---

## 🎯 Key Features

### ESP32 Firmware
- ✅ Non-blocking architecture
- ✅ Dual-core task management (FreeRTOS)
- ✅ Watchdog timer
- ✅ MQTT with QoS and offline buffering
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
- ✅ API key authentication

### Dashboard
- ✅ Real-time sensor data
- ✅ WebSocket updates
- ✅ Alert history
- ✅ Multi-location view
- ✅ Data export (CSV/JSON)

---

## 🔧 Development

### Prerequisites
- Docker (for MQTT broker)
- Node.js 16+
- PlatformIO (for ESP32 firmware)
- VS Code with Wokwi extension

### Environment Variables
Copy `.env.example` to `.env` and configure:
- MQTT broker settings
- API keys
- Firebase credentials (optional)
- Telegram bot token (optional)

---

## 📊 Performance

| Metric | HTTP (Old) | MQTT (New) |
|--------|-----------|-----------|
| Latency | ~1 second | <50ms |
| Power | ~500mW | ~35mW (93% reduction) |
| Bandwidth | 300 bytes/msg | 150 bytes/msg |

---

## 🐛 Troubleshooting

See documentation for detailed troubleshooting:
- [MQTT Troubleshooting](docs/mqtt/MQTT_SETUP_GUIDE.md#troubleshooting)
- [VS Code Tasks Troubleshooting](docs/vscode/VSCODE_TASKS_GUIDE.md#troubleshooting)
- [Getting Started Guide](docs/getting-started/HOW_TO_START.md#troubleshooting)

---

## 📚 Learn More

- [Complete Documentation](docs/README.md)
- [MQTT Architecture](docs/mqtt/MQTT_ARCHITECTURE.md)
- [System Requirements](docs/getting-started/HOW_TO_START.md)

---

**Version**: 2.0 (MQTT-enabled)

**Last Updated**: April 2026
