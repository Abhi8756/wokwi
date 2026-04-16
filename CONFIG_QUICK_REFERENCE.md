# ⚙️ Configuration Quick Reference

Quick answers for common configuration questions.

---

## 🔑 API Key

### What to put:
```bash
# Development (easy)
API_KEY=dev-key-123
REQUIRE_API_KEY=false

# Production (secure)
API_KEY=$(openssl rand -hex 32)
REQUIRE_API_KEY=true
```

### How to generate:
```bash
openssl rand -hex 32
```

---

## 📡 MQTT Username & Password

### For Development (Current Setup):
```bash
MQTT_USERNAME=
MQTT_PASSWORD=
```

**Leave them EMPTY!** ✅

Your Mosquitto broker is configured for anonymous access (no authentication needed).

---

### When to Use Authentication:

**Only for production deployments!**

To enable:
```bash
# 1. Create password in Mosquitto
docker exec -it manhole-mqtt-broker mosquitto_passwd -c /mosquitto/config/passwd mqtt_user

# 2. Update .env
MQTT_USERNAME=mqtt_user
MQTT_PASSWORD=your_password_here

# 3. Update mosquitto.conf
allow_anonymous false
password_file /mosquitto/config/passwd

# 4. Restart broker
docker-compose restart mosquitto
```

---

## 📱 Telegram Bot Token

### How to get:
1. Open Telegram
2. Search for [@BotFather](https://t.me/BotFather)
3. Send `/newbot`
4. Follow prompts
5. Copy token

### Format:
```bash
TELEGRAM_BOT_TOKEN=1234567890:ABCdefGHIjklMNOpqrsTUVwxyz
```

---

## 💬 Telegram Chat ID

### How to get:
1. Open Telegram
2. Search for [@userinfobot](https://t.me/userinfobot)
3. Send `/start`
4. Copy your ID

### Format:
```bash
# Single user
TELEGRAM_CHAT_IDS=836634920

# Multiple users
TELEGRAM_CHAT_IDS=836634920,123456789,987654321
```

---

## 🔥 Firebase Configuration

### Option 1: Use JSON File (Easier)
1. Download service account JSON from Firebase Console
2. Save as `wokwi/serviceAccountKey.json`
3. Leave Firebase env vars empty

### Option 2: Use Environment Variables
```bash
FIREBASE_PROJECT_ID=your-project-id
FIREBASE_CLIENT_EMAIL=firebase-adminsdk-xxxxx@your-project.iam.gserviceaccount.com
FIREBASE_PRIVATE_KEY="-----BEGIN PRIVATE KEY-----\n...\n-----END PRIVATE KEY-----\n"
```

---

## ✅ Minimum Required Configuration

### For Development (What You Need Now):
```bash
# MQTT (no authentication)
MQTT_BROKER=mqtt://localhost:1883
MQTT_USERNAME=                    # Leave empty ✅
MQTT_PASSWORD=                    # Leave empty ✅
MQTT_LOCATION_ID=location_001

# API Key (disabled for dev)
REQUIRE_API_KEY=false

# Storage (simple)
SENSOR_STORAGE_BACKEND=json
```

**That's it!** Everything else is optional.

---

## 🎯 Your Current Setup

Based on your `.env` file, you have:

✅ **MQTT Broker**: `mqtt://localhost:1883` (correct)
✅ **MQTT Username**: Empty (correct for development)
✅ **MQTT Password**: Empty (correct for development)
✅ **Location ID**: `location_001` (good)
✅ **Telegram**: Configured (optional but working)

**Your configuration is correct!** 🎉

---

## 🚀 What to Do Now

1. **Start MQTT broker**:
   ```bash
   ./start-mqtt.sh
   ```

2. **Start bridge server**:
   ```bash
   npm run bridge
   ```

3. **Start Wokwi simulator**

**No configuration changes needed!**

---

## 🔒 For Production Later

When deploying to production, you'll want to:

1. **Enable API key authentication**:
   ```bash
   API_KEY=$(openssl rand -hex 32)
   REQUIRE_API_KEY=true
   ```

2. **Enable MQTT authentication**:
   ```bash
   MQTT_USERNAME=mqtt_user
   MQTT_PASSWORD=secure_password
   ```

3. **Use TLS for MQTT**:
   ```bash
   MQTT_BROKER=mqtts://mqtt.example.com:8883
   ```

4. **Use Firestore**:
   ```bash
   SENSOR_STORAGE_BACKEND=firestore
   ```

---

## 📚 Complete Guide

For detailed explanations, see:
**[Configuration Guide](docs/getting-started/CONFIGURATION_GUIDE.md)**

---

## 💡 Quick Answers

### Q: Do I need MQTT username/password?
**A**: No, not for development. Leave them empty.

### Q: How do I get an API key?
**A**: For dev, use any string or disable with `REQUIRE_API_KEY=false`

### Q: Do I need Firebase?
**A**: No, use `SENSOR_STORAGE_BACKEND=json` for local storage.

### Q: Do I need Telegram?
**A**: No, it's optional. Leave `TELEGRAM_BOT_TOKEN` empty to disable.

---

**Your current configuration is ready to use! Just start the services.** 🚀
