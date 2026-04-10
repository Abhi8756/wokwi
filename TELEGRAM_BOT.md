# 🤖 Telegram Bot - Wokwi Integration Guide

## Overview

Your Manhole monitoring system now sends **real-time Telegram alerts** when gas thresholds are crossed. This guide explains how to connect your **Wokwi ESP32 simulator** directly to Telegram notifications.

---

## 🔥 WOKWI DATA → TELEGRAM ALERTS

### The Complete Flow

```
Wokwi ESP32 Simulator 
    ↓ (sends JSON via serial)
watch-rfc2217.js (npm run watch:gui)
    ↓ (reads serial data from port 4000)
POST to http://localhost:3001/api/sensor
    ↓
bridge.js receives real sensor data
    ↓
Checks against thresholds (CH4, H2S, Water Level)
    ↓
checkAndAlert() triggers
    ↓
🚨 Telegram Alert to your phone!
```

---

## ✨ Setup: 3 Terminals Running in Parallel

### **Terminal 1: Bridge Server** (API & Telegram Bot)
```powershell
npm run bridge
```

**What it does:**
- Starts the sensor API at `http://localhost:3001`
- Initializes Telegram bot with your credentials
- Receives sensor data and checks thresholds
- Sends alerts to Telegram

**Expected output:**
```
✅ Telegram bot initialized
🚀 Wokwi bridge running on http://localhost:3001
📡 POST sensor data to: http://localhost:3001/api/sensor
```

---

### **Terminal 2: Wokwi Watcher** (Serial Reader)
```powershell
npm run watch:gui
```

**What it does:**
- Connects to Wokwi simulator's serial port (RFC2217 on port 4000)
- Reads JSON sensor data from your ESP32 code
- **Forwards data to the bridge** (`/api/sensor`)
- Automatically reconnects if Wokwi disconnects

**Expected output:**
```
Connecting to Wokwi RFC2217 at 127.0.0.1:4000
Connected to simulator!
Received sensor data: {"ch4": 850, "h2s": 12, ...}
```

---

### **Terminal 3: Wokwi Simulator** (ESP32 Code)
In VS Code where your ESP32 project is:

1. Open the Wokwi project folder
2. Press `F1` → Search for **"Wokwi: Start Simulator"**
3. Or click the **▶️ Play button** in the Wokwi toolbar
4. Simulator connects to `127.0.0.1:4000` (port 4000)
5. Your ESP32 code sends sensor JSON data via serial

**The simulator should output sensor readings like:**
```json
{"ch4": 703.81, "h2s": 45.11, "waterLevel": 21.51}
```

---

## 🚀 Quick Start Guide

### Step 1: Verify Telegram Configuration
Make sure your `.env` file has:
```env
TELEGRAM_BOT_TOKEN=8741837188:AAHrhu4Ft8UBpIG94gwBsU7CIqYFAM8Srxw
TELEGRAM_CHAT_IDS=836634920
```

### Step 2: Start Services in Order

**In Terminal 1:**
```powershell
npm run bridge
```
Wait for: `✅ Telegram bot initialized`

**In Terminal 2:**
```powershell
npm run watch:gui
```
Wait for: `Connected to simulator!`

**In Terminal 3:**
Start Wokwi simulator (F1 → "Wokwi: Start Simulator")

### Step 3: Watch for Alerts

Monitor **Terminal 1 (bridge)** output. When sensor data arrives, you'll see:

```
[INGESTED] wokwi-gui | CH4 850.00 ppm | H2S 12.50 ppm | Water Level 45.00 cm | Danger
📲 Telegram alert sent - Danger at MH-1023
```

📱 **Check your Telegram** - you should receive an alert message!

---

## 🚨 Alert Thresholds

Your Telegram bot sends alerts when these levels are exceeded:

| Metric | Warning | Danger | Status |
|--------|---------|--------|--------|
| **CH4 (Methane)** | 700 ppm | 1000 ppm | 🟢 Safe/<br/>🟡 Warning/<br/>🔴 Danger |
| **H2S (Hydrogen Sulfide)** | 7 ppm | 10 ppm | 🟢 Safe/<br/>🟡 Warning/<br/>🔴 Danger |
| **Water Level** | 35 cm | 50 cm | 🟢 Safe/<br/>🟡 Warning/<br/>🔴 Danger |

---

## 📱 Example Alert Messages

### ⚠️ WARNING Alert
```
⚠️ WARNING

🔴 CH4: 850 ppm (warning)

📍 Location: MH-1023
🕐 Time: 2:45:30 PM
```

### 🚨 DANGER Alert
```
🚨 DANGER ALERT

🔴 CH4: 1200 ppm (danger)
🟠 H2S: 12 ppm (danger)
🔵 Water Level: 55 cm (danger)

📍 Location: MH-1023
🕐 Time: 2:50:15 PM
```

### ✅ Test Alert
```
✅ Telegram Bot Test

Bot is connected and ready to send alerts!

📊 This is a test notification.
```

---

## 🧪 Testing

### Test 1: Verify Bot Connectivity
In any terminal:
```powershell
curl http://localhost:3001/api/telegram/test
```

**Expected response:**
```json
{
  "status": "ok",
  "message": "Test message sent successfully",
  "chatCount": 1
}
```

**Expected on Telegram:** ✅ Test message received

---

### Test 2: Send Fake Danger Data
```powershell
curl -X POST http://localhost:3001/api/sensor `
  -H "Content-Type: application/json" `
  -d '{"ch4": 1100, "h2s": 12, "water": 40}'
```

**Expected on Telegram:** 🚨 DANGER alert with CH4 and H2S values

---

### Test 3: Real Wokwi Simulation
1. All 3 terminals running
2. Wokwi simulator sends data naturally
3. Watch bridge logs for `[INGESTED]` messages
4. Receive Telegram alerts automatically

---

## 🔍 Debugging

### Bridge not showing "Telegram bot initialized"
**Issue:** `.env` file not loaded or bot token missing

**Fix:**
```powershell
Get-Process node | Stop-Process -Force  # Kill all Node processes
npm run bridge                           # Restart
```

---

### No "[INGESTED]" messages in bridge terminal
**Issue:** Wokwi data not reaching the bridge

**Fix:**
1. Make sure `npm run watch:gui` is running (Terminal 2)
2. Check Wokwi simulator is actually running
3. Verify port 4000 connection in watch:gui terminal
4. Look for errors in Terminal 2 output

---

### Telegram alerts not received
**Issue:** Bot not sending messages

**Check:**
```powershell
curl http://localhost:3001/api/telegram/test
```

**If error:** Bot token or chat ID is wrong
- Verify both values in `.env`
- Restart bridge: `npm run bridge`
- Test again

---

### "Chat not found" error
**Issue:** Invalid Telegram chat ID

**Fix:**
1. Open Telegram, send `/start` to your bot
2. Visit: `https://api.telegram.org/bot8741837188:AAHrhu4Ft8UBpIG94gwBsU7CIqYFAM8Srxw/getUpdates`
3. Find `"id"` in JSON response
4. Update `TELEGRAM_CHAT_IDS` in `.env`
5. Restart bridge

---

## 📊 Architecture Details

### watch-rfc2217.js
- **Purpose:** Reads Wokwi ESP32 serial output
- **Input:** RFC2217 serial data on port 4000
- **Output:** POST to `/api/sensor` endpoint
- **Source field:** Sets `source: "wokwi-gui"` for identification

### bridge.js
- **Purpose:** Central API server
- **Endpoints:**
  - `POST /api/sensor` - Receives sensor data & triggers alerts
  - `GET /api/telegram/test` - Test bot connectivity
  - `GET /api/sensor/latest` - Get latest reading
  - `GET /api/sensor/history` - Get historical data

### telegram-bot.js
- **Purpose:** Telegram notification engine
- **Features:**
  - Smart throttling (5-minute cooldown per alert type)
  - Status tracking (only alerts on changes)
  - Formatted messages with emojis
  - Multi-chat support

---

## ⚙️ Configuration

### Environment Variables (.env)
```env
# Telegram
TELEGRAM_BOT_TOKEN=8741837188:AAHrhu4Ft8UBpIG94gwBsU7CIqYFAM8Srxw
TELEGRAM_CHAT_IDS=836634920

# Wokwi Serial
WOKWI_RFC2217_HOST=127.0.0.1
WOKWI_RFC2217_PORT=4000

# Bridge Server
BRIDGE_PORT=3001
BRIDGE_HOST=127.0.0.1

# Storage
SENSOR_STORAGE_BACKEND=local-cache
LOCAL_CACHE_ENABLED=true
```

### Customizing Thresholds
Edit `bridge.js` (lines 21-27):
```javascript
const thresholds = {
  h2sWarning: 7,        // ppm
  h2sDanger: 10,        // ppm
  ch4Warning: 700,      // ppm
  ch4Danger: 1000,      // ppm
  waterLevelWarning: 35, // cm
  waterLevelDanger: 50,  // cm
};
```

---

## 🎯 Expected Flow Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    WOKWI ECOSYSTEM                          │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────┐                                       │
│  │  Wokwi ESP32     │                                       │
│  │  Simulator       │                                       │
│  │  (sends JSON     │                                       │
│  │   on serial)     │                                       │
│  └────────┬─────────┘                                       │
│           │                                                  │
│           │ RFC2217 Serial                                  │
│           │ Port 4000                                       │
│           ▼                                                  │
│  ┌──────────────────────────────────────┐                   │
│  │  watch-rfc2217.js                    │                   │
│  │  (npm run watch:gui)                 │                   │
│  │                                      │                   │
│  │  - Reads serial data                 │                   │
│  │  - Parses JSON                       │                   │
│  │  - Forwards to bridge                │                   │
│  └──────────────┬───────────────────────┘                   │
│                 │                                            │
│                 │ HTTP POST                                 │
│                 │ /api/sensor                               │
│                 ▼                                            │
│     ┌───────────────────────────────────┐                   │
│     │  bridge.js                        │                   │
│     │  (npm run bridge)                 │                   │
│     │                                   │                   │
│     │  ┌──────────────────────────────┐ │                   │
│     │  │  normalizeSensorData()       │ │                   │
│     │  └───────────┬──────────────────┘ │                   │
│     │              │                    │                   │
│     │  ┌───────────▼──────────────────┐ │                   │
│     │  │  Evaluate Thresholds        │ │                   │
│     │  │  - CH4, H2S, Water Level    │ │                   │
│     │  └───────────┬──────────────────┘ │                   │
│     │              │                    │                   │
│     │  ┌───────────▼──────────────────┐ │                   │
│     │  │  telegramBot.checkAndAlert() │ │                   │
│     │  └───────────┬──────────────────┘ │                   │
│     │              │                    │                   │
│     └──────────────┼────────────────────┘                   │
│                    │                                         │
│                    │ Telegram API                           │
│                    ▼                                         │
│           ┌─────────────────┐                               │
│           │  Telegram Bot   │                               │
│           └────────┬────────┘                               │
│                    │                                         │
│                    │ Push Notification                      │
│                    ▼                                         │
│           ┌─────────────────┐                               │
│           │ 📱 Your Phone   │                               │
│           │                 │                               │
│           │ 🚨 Danger Alert │                               │
│           └─────────────────┘                               │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## ✅ Checklist

- [ ] `.env` file has `TELEGRAM_BOT_TOKEN` and `TELEGRAM_CHAT_IDS`
- [ ] Terminal 1: `npm run bridge` is running
- [ ] Terminal 2: `npm run watch:gui` is running
- [ ] Terminal 3: Wokwi simulator is running
- [ ] Wokwi simulator is sending JSON sensor data
- [ ] Bridge terminal shows `[INGESTED]` messages
- [ ] Telegram messages received when thresholds crossed
- [ ] Test alert successful: `curl http://localhost:3001/api/telegram/test`

---

## 🆘 Need Help?

**Check terminal outputs for:**
- Bridge: `⚠️` or `❌` error messages
- Watch GUI: Connection issues to port 4000
- Wokwi: Simulator not starting or serial output errors

**Test connectivity:**
```powershell
# Is bridge running?
curl http://localhost:3001/health

# Is bot initialized?
curl http://localhost:3001/api/telegram/test

# Is data flowing?
curl http://localhost:3001/api/sensor/latest
```

---

## 🎉 You're All Set!

Your Manhole monitoring system is now:
✅ Reading real Wokwi sensor data
✅ Processing thresholds
✅ Sending instant Telegram alerts
✅ Monitoring 24/7

**Enjoy real-time manhole gas alerts!** 🚀
