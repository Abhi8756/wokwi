# MQTT Documentation

Complete documentation for MQTT setup, configuration, and architecture.

---

## 📚 Available Documentation

### [MQTT Quick Start](MQTT_QUICK_START.md) ⭐
**Get MQTT running in 3 commands.**

Perfect for:
- Quick setup
- First-time users
- Testing MQTT

**Start here for the fastest setup!**

---

### [MQTT Setup Guide](MQTT_SETUP_GUIDE.md)
**Complete MQTT configuration guide.**

Covers:
- Mosquitto broker installation
- ESP32 MQTT client configuration
- Bridge server MQTT subscriber
- Topic hierarchy and QoS levels
- Security and authentication
- Production deployment
- Troubleshooting

**Read this for complete understanding!**

---

### [MQTT Architecture](MQTT_ARCHITECTURE.md)
**System architecture and data flow.**

Includes:
- System architecture diagrams
- Topic hierarchy
- Message flow
- QoS levels explained
- Connection states
- Performance characteristics
- Security layers
- Deployment scenarios
- Monitoring and debugging

**Read this to understand how it all works!**

---

### [MQTT Setup Complete](MQTT_SETUP_COMPLETE.md)
**Setup summary and verification.**

Contains:
- What was created
- How to start using MQTT
- Verification steps
- Benefits overview
- Success checklist

**Read this after setup to verify everything!**

---

## 🎯 Which Document Should I Read?

### I want to get started quickly
→ **[MQTT Quick Start](MQTT_QUICK_START.md)**

### I want complete setup instructions
→ **[MQTT Setup Guide](MQTT_SETUP_GUIDE.md)**

### I want to understand the architecture
→ **[MQTT Architecture](MQTT_ARCHITECTURE.md)**

### I want to verify my setup
→ **[MQTT Setup Complete](MQTT_SETUP_COMPLETE.md)**

### I'm having issues
→ **[MQTT Setup Guide - Troubleshooting](MQTT_SETUP_GUIDE.md#troubleshooting)**

---

## 🔑 Key Concepts

### MQTT Broker
- **What**: Message broker (Mosquitto)
- **Where**: Docker container
- **Port**: 1883 (non-TLS) or 8883 (TLS)
- **Purpose**: Routes messages between publishers and subscribers

### MQTT Topics
```
manhole/
  └── {location_id}/
      ├── sensors      - Sensor readings
      ├── alerts       - Alert notifications
      ├── diagnostics  - System health
      ├── config       - Configuration
      └── status       - Device status
```

### Quality of Service (QoS)
- **QoS 0**: At most once (fire and forget)
- **QoS 1**: At least once (acknowledged)
- **QoS 2**: Exactly once (not supported by PubSubClient)

### Message Flow
```
ESP32 → Publish → Broker → Subscribe → Bridge → Storage/Dashboard
```

---

## 📊 MQTT Benefits

| Feature | HTTP (Old) | MQTT (New) |
|---------|-----------|-----------|
| **Latency** | ~1 second | <50ms ⚡ |
| **Power** | ~500mW | ~35mW (93% reduction) 🔋 |
| **Bandwidth** | 300 bytes/msg | 150 bytes/msg 📉 |
| **Offline** | No buffer | 10 messages 💾 |
| **Reliability** | Best effort | QoS guarantees ✅ |

---

## 🚀 Quick Start Commands

### Start MQTT Broker
```bash
./start-mqtt.sh
```

### Test MQTT
```bash
./test-mqtt.sh
```

### Monitor Traffic
```bash
docker exec -it manhole-mqtt-broker mosquitto_sub -t 'manhole/#' -v
```

### Stop MQTT
```bash
./stop-mqtt.sh
```

---

## 🔒 Security

### Development (Current)
- Anonymous connections allowed
- Plain MQTT (port 1883)
- Local network only

### Production (Recommended)
- TLS/SSL encryption (port 8883)
- Username/password authentication
- Client certificates
- Certificate validation
- Access control lists (ACLs)

**See [MQTT Setup Guide - Security](MQTT_SETUP_GUIDE.md#security-production) for setup.**

---

## 🐛 Common Issues

### "MQTT connection failed, rc=-2"
**Cause**: Broker not running

**Solution**: Start broker with `./start-mqtt.sh`

---

### Bridge not receiving messages
**Cause**: Topic mismatch

**Solution**: Check `.env` file has correct `MQTT_LOCATION_ID`

---

### Port 1883 already in use
**Cause**: Another MQTT broker running

**Solution**: Stop other broker or change port in `docker-compose.yml`

---

## 🔗 Related Documentation

- **[Getting Started](../getting-started/)** - Quick start guides
- **[VS Code Tasks](../vscode/)** - Task automation
- **[Telegram Bot](../telegram/)** - Alert notifications
- **[Main Documentation](../README.md)** - Complete index

---

## 📖 External Resources

- [Mosquitto Documentation](https://mosquitto.org/documentation/)
- [MQTT Protocol Specification](https://mqtt.org/mqtt-specification/)
- [PubSubClient Library](https://github.com/knolleary/pubsubclient)
- [ESP32 MQTT Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mqtt.html)

---

## 💡 Pro Tips

1. **Use VS Code tasks** for easy MQTT management
2. **Monitor traffic** to debug communication issues
3. **Check broker logs** when things go wrong
4. **Test with mosquitto_pub/sub** before using ESP32
5. **Enable TLS** for production deployments

---

**Need help?** Check the troubleshooting sections or review the complete setup guide.
