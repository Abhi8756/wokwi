# Configuration Guide

Complete guide for configuring your IoT Manhole Monitoring System.

---

## 📋 Overview

This guide explains what to put in each configuration field in the `.env` file.

---

## 🔑 API Key Configuration

### API_KEY

**What it is**: A secret key to secure your bridge server API endpoints.

**Current Setup (Development)**:
```bash
API_KEY=your-secure-api-key-here
REQUIRE_API_KEY=false  # Set to false for development
```

**For Development (Easy)**:
```bash
# Option 1: Use a simple key
API_KEY=dev-secret-key-123

# Option 2: Leave authentication disabled
REQUIRE_API_KEY=false
```

**For Production (Secure)**:
```bash
# Generate a strong random key
API_KEY=$(openssl rand -hex 32)
REQUIRE_API_KEY=true
```

**How to generate**:
```bash
# Linux/Mac
openssl rand -hex 32

# Output example:
# a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1v2w3x4y5z6a7b8c9d0e1f2
```

**What to use**:
- **Development**: Any string (e.g., `dev-key-123`) or disable with `REQUIRE_API_KEY=false`
- **Production**: Strong random key generated with `openssl rand -hex 32`

---

## 📡 MQTT Configuration

### MQTT_BROKER

**What it is**: The address of your MQTT broker.

**For Local Development (Current Setup)**:
```bash
MQTT_BROKER=mqtt://localhost:1883
```

**For Production**:
```bash
# Local network
MQTT_BROKER=mqtt://192.168.1.100:1883

# Cloud broker
MQTT_BROKER=mqtt://mqtt.example.com:1883

# With TLS (secure)
MQTT_BROKER=mqtts://mqtt.example.com:8883
```

**What to use**:
- **Local development**: `mqtt://localhost:1883` (default)
- **Same network**: `mqtt://[broker-ip]:1883`
- **Cloud/Production**: `mqtts://[broker-domain]:8883` (with TLS)

---

### MQTT_USERNAME and MQTT_PASSWORD

**What they are**: Credentials for authenticating with the MQTT broker.

**Current Setup (Development - No Authentication)**:
```bash
MQTT_USERNAME=
MQTT_PASSWORD=
```

**This is CORRECT for development!** The Mosquitto broker we set up allows anonymous connections.

**When to use authentication**:
- ✅ Production deployments
- ✅ Public-facing brokers
- ✅ Cloud MQTT services
- ✅ Multi-tenant systems

**When NOT needed**:
- ✅ Local development (current setup)
- ✅ Private network with firewall
- ✅ Testing and prototyping

---

### Setting Up MQTT Authentication (Optional - Production)

If you want to enable authentication:

#### Step 1: Create Password File in Mosquitto

```bash
# Create password file
docker exec -it manhole-mqtt-broker mosquitto_passwd -c /mosquitto/config/passwd mqtt_user

# Enter password when prompted
```

#### Step 2: Update mosquitto.conf

Edit `wokwi/mosquitto.conf`:

```conf
# Disable anonymous access
allow_anonymous false

# Enable password file
password_file /mosquitto/config/passwd
```

#### Step 3: Restart Broker

```bash
docker-compose restart mosquitto
```

#### Step 4: Update .env

```bash
MQTT_USERNAME=mqtt_user
MQTT_PASSWORD=your_secure_password
```

#### Step 5: Update ESP32 Configuration

Via serial command:
```
config set mqtt_username mqtt_user
config set mqtt_password your_secure_password
config save
```

---

### MQTT_TOPIC_PREFIX

**What it is**: The root topic for all MQTT messages.

**Current Setup**:
```bash
MQTT_TOPIC_PREFIX=manhole
```

**Topic structure**:
```
manhole/
  └── location_001/
      ├── sensors
      ├── alerts
      ├── diagnostics
      └── config
```

**What to use**:
- **Default**: `manhole` (recommended)
- **Custom**: Any valid MQTT topic (e.g., `iot`, `sensors`, `monitoring`)

**When to change**:
- Multiple systems on same broker
- Organizational naming conventions
- Integration with existing systems

---

### MQTT_LOCATION_ID

**What it is**: Identifier for this specific monitoring location.

**Current Setup**:
```bash
MQTT_LOCATION_ID=location_001
```

**What to use**:
- **Single location**: `location_001` (default)
- **Multiple locations**: `location_001`, `location_002`, etc.
- **Descriptive names**: `main_street`, `industrial_zone`, etc.

**Examples**:
```bash
# Geographic
MQTT_LOCATION_ID=main_street_manhole
MQTT_LOCATION_ID=industrial_zone_01

# Numeric
MQTT_LOCATION_ID=MH-1023
MQTT_LOCATION_ID=SITE-001

# Hierarchical
MQTT_LOCATION_ID=city/district/manhole_01
```

**Important**: ESP32 and bridge must use the same location ID!

---

## 🔥 Firebase/Firestore Configuration

### FIREBASE_PROJECT_ID

**What it is**: Your Firebase project ID.

**How to get it**:
1. Go to [Firebase Console](https://console.firebase.google.com/)
2. Select your project
3. Click gear icon → Project settings
4. Copy "Project ID"

**Example**:
```bash
FIREBASE_PROJECT_ID=manhole-monitoring-e5d2b
```

---

### FIREBASE_CLIENT_EMAIL

**What it is**: Service account email for Firebase Admin SDK.

**How to get it**:
1. Firebase Console → Project settings
2. Service accounts tab
3. Click "Generate new private key"
4. Download JSON file
5. Copy `client_email` from JSON

**Example**:
```bash
FIREBASE_CLIENT_EMAIL=firebase-adminsdk-xxxxx@manhole-monitoring-e5d2b.iam.gserviceaccount.com
```

---

### FIREBASE_PRIVATE_KEY

**What it is**: Private key for Firebase service account.

**How to get it**:
1. Same JSON file from above
2. Copy `private_key` value
3. Keep the `\n` characters

**Example**:
```bash
FIREBASE_PRIVATE_KEY="-----BEGIN PRIVATE KEY-----\nMIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQC...\n-----END PRIVATE KEY-----\n"
```

**Important**: Keep quotes and `\n` characters!

---

### Alternative: Use serviceAccountKey.json

Instead of environment variables, you can use a JSON file:

1. Download service account JSON from Firebase
2. Save as `wokwi/serviceAccountKey.json`
3. Leave Firebase env vars empty

**The bridge will automatically use the JSON file if present.**

---

## 📱 Telegram Bot Configuration

### TELEGRAM_BOT_TOKEN

**What it is**: Token for your Telegram bot.

**How to get it**:

1. Open Telegram and search for [@BotFather](https://t.me/BotFather)
2. Send `/newbot`
3. Follow prompts to create bot
4. BotFather will give you a token

**Example**:
```bash
TELEGRAM_BOT_TOKEN=1234567890:ABCdefGHIjklMNOpqrsTUVwxyz
```

**Format**: `[numbers]:[letters and numbers]`

**To disable Telegram**:
```bash
TELEGRAM_BOT_TOKEN=
```

---

### TELEGRAM_CHAT_IDS

**What it is**: Chat IDs of users/groups to receive alerts.

**How to get your chat ID**:

1. Open Telegram and search for [@userinfobot](https://t.me/userinfobot)
2. Send `/start`
3. Bot will reply with your chat ID

**Example**:
```bash
# Single user
TELEGRAM_CHAT_IDS=836634920

# Multiple users (comma-separated)
TELEGRAM_CHAT_IDS=836634920,123456789,987654321
```

**For groups**:
1. Add bot to group
2. Send a message in group
3. Visit: `https://api.telegram.org/bot[YOUR_BOT_TOKEN]/getUpdates`
4. Look for `"chat":{"id":-123456789}` (negative number for groups)

---

## 🔧 Other Configuration

### BRIDGE_PORT

**What it is**: Port for the bridge server.

**Default**:
```bash
BRIDGE_PORT=3001
```

**When to change**:
- Port conflict with another service
- Multiple bridge instances
- Firewall requirements

---

### SENSOR_STORAGE_BACKEND

**What it is**: Where to store sensor data.

**Options**:
```bash
# Use Firestore (cloud)
SENSOR_STORAGE_BACKEND=firestore

# Use local JSON file
SENSOR_STORAGE_BACKEND=json

# Auto-detect (Firestore if configured, else JSON)
SENSOR_STORAGE_BACKEND=auto
```

**Recommendation**:
- **Development**: `json` (simple, no setup)
- **Production**: `firestore` (scalable, reliable)

---

### Threshold Hysteresis

**What they are**: Prevent alert spam when values oscillate near thresholds.

**Current Setup**:
```bash
H2S_HYSTERESIS=1.0      # ±1 ppm
CH4_HYSTERESIS=50.0     # ±50 ppm
WATER_HYSTERESIS=2.0    # ±2 cm
```

**How it works**:
- Alert triggers at threshold
- Alert clears at threshold - hysteresis
- Prevents rapid on/off alerts

**Example**:
```
CH4 threshold: 1000 ppm
CH4 hysteresis: 50 ppm

Alert ON:  CH4 > 1000 ppm
Alert OFF: CH4 < 950 ppm (1000 - 50)
```

---

## 📝 Complete Example Configurations

### Development (Simple)

```bash
# Bridge
BRIDGE_PORT=3001
SENSOR_STORAGE_BACKEND=json
API_KEY=dev-key-123
REQUIRE_API_KEY=false

# MQTT (no authentication)
MQTT_BROKER=mqtt://localhost:1883
MQTT_USERNAME=
MQTT_PASSWORD=
MQTT_TOPIC_PREFIX=manhole
MQTT_LOCATION_ID=location_001

# Telegram (optional)
TELEGRAM_BOT_TOKEN=
TELEGRAM_CHAT_IDS=

# Firebase (optional)
FIREBASE_PROJECT_ID=
FIREBASE_CLIENT_EMAIL=
FIREBASE_PRIVATE_KEY=
```

---

### Production (Secure)

```bash
# Bridge
BRIDGE_PORT=3001
SENSOR_STORAGE_BACKEND=firestore
API_KEY=a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6q7r8s9t0u1v2w3x4y5z6a7b8c9d0e1f2
REQUIRE_API_KEY=true

# MQTT (with authentication and TLS)
MQTT_BROKER=mqtts://mqtt.example.com:8883
MQTT_USERNAME=mqtt_user
MQTT_PASSWORD=secure_mqtt_password_here
MQTT_TOPIC_PREFIX=manhole
MQTT_LOCATION_ID=site_001

# Telegram
TELEGRAM_BOT_TOKEN=1234567890:ABCdefGHIjklMNOpqrsTUVwxyz
TELEGRAM_CHAT_IDS=836634920,123456789

# Firebase
FIREBASE_PROJECT_ID=manhole-monitoring-prod
FIREBASE_CLIENT_EMAIL=firebase-adminsdk-xxxxx@manhole-monitoring-prod.iam.gserviceaccount.com
FIREBASE_PRIVATE_KEY="-----BEGIN PRIVATE KEY-----\n...\n-----END PRIVATE KEY-----\n"
FIRESTORE_COLLECTION=sensor_readings
```

---

## ✅ Quick Setup Checklist

### Minimum Required (Development)
- [x] `MQTT_BROKER=mqtt://localhost:1883`
- [x] `MQTT_LOCATION_ID=location_001`
- [x] `REQUIRE_API_KEY=false`

### Recommended (Development)
- [x] Minimum required
- [x] `SENSOR_STORAGE_BACKEND=json`
- [x] `TELEGRAM_BOT_TOKEN` (for alerts)
- [x] `TELEGRAM_CHAT_IDS` (your chat ID)

### Production Ready
- [x] Strong `API_KEY` with `REQUIRE_API_KEY=true`
- [x] MQTT with TLS (`mqtts://`)
- [x] MQTT authentication (`MQTT_USERNAME`, `MQTT_PASSWORD`)
- [x] Firestore configuration
- [x] Telegram bot configured
- [x] Proper threshold values

---

## 🔒 Security Best Practices

### DO
- ✅ Use strong random API keys
- ✅ Enable MQTT authentication in production
- ✅ Use TLS/SSL for MQTT (mqtts://)
- ✅ Keep tokens and keys in `.env` (not in code)
- ✅ Add `.env` to `.gitignore`
- ✅ Rotate keys periodically

### DON'T
- ❌ Commit `.env` to git
- ❌ Share API keys publicly
- ❌ Use weak passwords
- ❌ Disable authentication in production
- ❌ Use plain MQTT (mqtt://) in production

---

## 🐛 Troubleshooting

### "MQTT connection failed"
**Check**:
- `MQTT_BROKER` is correct
- Broker is running: `docker ps | grep mosquitto`
- If using auth, credentials are correct

---

### "API key invalid"
**Check**:
- `API_KEY` matches in ESP32 and bridge
- Or set `REQUIRE_API_KEY=false` for development

---

### "Telegram bot not responding"
**Check**:
- `TELEGRAM_BOT_TOKEN` is correct
- `TELEGRAM_CHAT_IDS` is correct
- Bot is not blocked by user

---

### "Firebase error"
**Check**:
- All three Firebase variables are set
- Or use `serviceAccountKey.json` file
- Or set `SENSOR_STORAGE_BACKEND=json`

---

## 📚 Related Documentation

- **[How to Start](HOW_TO_START.md)** - System startup guide
- **[MQTT Setup Guide](../mqtt/MQTT_SETUP_GUIDE.md)** - MQTT configuration
- **[Telegram Bot Setup](../telegram/TELEGRAM_BOT_SETUP.md)** - Telegram configuration

---

## 💡 Pro Tips

1. **Start simple**: Use minimal config for development
2. **Test incrementally**: Add features one at a time
3. **Keep backups**: Save working `.env` configurations
4. **Use comments**: Document custom settings
5. **Check logs**: Most config issues show in logs

---

**Need help?** Check the troubleshooting section or review related documentation.
