# Telegram Bot Integration - Technical Documentation

## Overview
The Manhole monitoring system includes a real-time Telegram notification system that alerts users when gas (CH4, H2S) or water level thresholds are crossed.

## Architecture

### System Flow
```
Sensor Data (ESP32)
        ↓
continuous-data.js (Simulator)
        ↓
bridge.js (/api/sensor endpoint) → normalizeSensorData()
        ↓
Status Evaluation (compares against thresholds)
        ↓
telegram-bot.js (TelegramAlertBot class)
        ↓
checkAndAlert() method
        ↓
Telegram API → User's Chat
```

## Components

### 1. **telegram-bot.js** - Alert Bot Module
Core class that handles:
- Telegram bot initialization with token and chat IDs
- Threshold monitoring and status tracking
- Smart alert throttling (5-minute cooldown to prevent spam)
- Message formatting with emoji and HTML formatting
- Metric-specific and overall status alerts

**Key Methods:**
- `checkAndAlert(reading)` - Main method called when new sensor data arrives
- `sendMessage(message, options)` - Sends message to all configured chat IDs
- `shouldThrottle(chatId, alertType)` - Prevents duplicate alerts
- `formatOverallAlert()` - Creates formatted status alert
- `formatMetricAlert()` - Creates metric-specific alert
- `sendTest()` - Test connectivity

### 2. **bridge.js** - Main Server
Modified to:
- Import and initialize `TelegramAlertBot` with environment variables
- Call `telegramBot.checkAndAlert()` for every new sensor reading
- Provide test endpoint at `/api/telegram/test`

**Integration Point:**
```javascript
// In POST /api/sensor endpoint
telegramBot.checkAndAlert(reading).catch((error) => {
  console.error("❌ Telegram alert error:", error.message);
});
```

## Configuration

### Environment Variables
Add to `.env` file:
```env
TELEGRAM_BOT_TOKEN=123456:ABC-DEF1234ghIkl-zyx57W2v1u123ew11
TELEGRAM_CHAT_IDS=987654321,123456789
```

### Key Configuration Details
- **TELEGRAM_BOT_TOKEN**: Required. Bot token from @BotFather
- **TELEGRAM_CHAT_IDS**: Comma-separated list of chat IDs
- If not configured, bot silently disables (logs warning)

## Alert Thresholds

### Current Thresholds
```javascript
{
  h2sWarning: 7,        // ppm
  h2sDanger: 10,        // ppm
  ch4Warning: 700,      // ppm
  ch4Danger: 1000,      // ppm
  waterLevelWarning: 35, // cm
  waterLevelDanger: 50,  // cm
}
```

### Customizing Thresholds
To change thresholds, edit the `thresholds` object in `bridge.js` (lines 21-27).

## Alert Types

### 1. **Status Change Alert**
Sent when overall status changes:
- Safe → Warning
- Safe → Danger
- Warning → Danger
- Danger → Warning or Safe

**Format:**
```
⚠️ WARNING

🔴 CH4: 850 ppm (warning)

📍 Location: MH-1023
🕐 Time: 2:45:30 PM
```

### 2. **Metric-Specific Alert**
Sent when in Danger state and a metric enters danger:
- Only during Danger status
- Sent for each metric crossing its danger threshold

**Format:**
```
🚨 DANGER

CH4 (Methane)
📊 Current: 1200 ppm
⚠️ Threshold: 1000 ppm
📍 Location: MH-1023
```

### 3. **Test Alert**
Manual test via `/api/telegram/test` endpoint:
```
✅ Telegram Bot Test

Bot is connected and ready to send alerts!

📊 This is a test notification.
```

## Smart Throttling

### Purpose
Prevents alert spam when sensors fluctuate around thresholds.

### Implementation
- **Cooldown Type**: Per chat ID and alert type (e.g., "MH-1023-warning")
- **Duration**: 5 minutes (300,000 ms)
- **Logic**:
  ```javascript
  if (timeSinceLastAlert < 5minutes) {
    // Skip sending duplicate alert
  } else {
    // Send alert and update cooldown
  }
  ```

### State Tracking
```javascript
previousStatus = {
  "MH-1023-status": "Safe",
  "MH-1023-dangers": {
    ch4: "safe",
    h2s: "safe",
    waterLevel: "safe"
  }
}

alertCooldown = {
  "987654321-MH-1023-warning": timestamp,
  "987654321-MH-1023-danger": timestamp
}
```

## API Endpoints

### Test Endpoint
**GET** `/api/telegram/test`

**Response (Success):**
```json
{
  "status": "ok",
  "message": "Test message sent successfully",
  "chatCount": 1
}
```

**Response (Disabled):**
```json
{
  "error": "Telegram bot is not enabled. Set TELEGRAM_BOT_TOKEN and TELEGRAM_CHAT_IDS environment variables."
}
```

### Sensor Endpoint (Modified)
**POST** `/api/sensor`

Now includes Telegram alert checking:
```javascript
{
  "status": "ok",
  "storageMode": "firestore+local-cache",
  "firebaseDocId": "ABC123...",
  "reading": {
    "ch4": 850,
    "h2s": 5.5,
    "waterLevel": 40,
    "status": "Warning",
    "alert": false,
    "location": { "id": "MH-1023", "lat": 12.9692, "lng": 79.1559 },
    "lastUpdated": "2024-04-09T14:45:30.000Z"
  }
}
```

## Monitoring & Debugging

### Console Logs

**Initialization:**
```
✅ Telegram bot initialized
```

**Alert Sent:**
```
📲 Telegram alert sent - Warning at MH-1023
```

**Error:**
```
❌ Telegram alert error: Invalid bot token
❌ Failed to send Telegram message to 987654321: Chat not found
```

### Checking Bot Status
```bash
# In another terminal while bridge is running
curl http://localhost:3001/api/telegram/test
```

### Viewing Logs
```bash
npm run bridge
# Look for messages like:
# ✅ Telegram bot initialized
# 📲 Telegram alert sent - Warning at MH-1023
# ❌ Telegram alert error: ...
```

## Error Handling

### Common Issues

**Invalid Token:**
- Log: `⚠️ Telegram bot disabled (no TELEGRAM_BOT_TOKEN found)`
- Fix: Set `TELEGRAM_BOT_TOKEN` in `.env`

**Invalid Chat ID:**
- Log: `❌ Failed to send Telegram message to 987654321: Chat not found`
- Fix: Get correct Chat ID from getUpdates API

**Bot Not Started:**
- Message doesn't send
- Fix: Send `/start` to bot in Telegram first

**Rate Limited:**
- Telegram API rate limiting
- Bot includes automatic retry logic

## Performance Considerations

- **Async Operation**: Alert checking doesn't block sensor data processing
- **Parallel Sends**: Messages sent to multiple chat IDs in parallel
- **Timeout**: Set to prevent hanging requests
- **Memory**: Alert state stored in memory (cleared on restart)

## Future Enhancements

Possible improvements:
1. Persistent alert history to database
2. User-configurable thresholds via Telegram commands
3. Daily summary reports
4. Alert history queries
5. Webhook-based notifications
6. Multiple bot instances per location
7. Notification priority levels
8. Custom alert messages per location

## Dependencies

- **telegraf** (^4.15.0): Telegram bot framework
- Part of existing: express, firebase-admin

## Testing

### Manual Test
```bash
# Terminal 1
npm run bridge

# Terminal 2
# Test endpoint
curl http://localhost:3001/api/telegram/test

# Simulate sensor data
curl -X POST http://localhost:3001/api/sensor \
  -H "Content-Type: application/json" \
  -d '{"ch4": 1100, "h2s": 12, "waterLevel": 55}'
```

### Expected Results
1. Bot should send test message immediately
2. Sensor POST should trigger danger alert
3. Check Telegram app for messages

## Integration with Other Systems

### Firebase/Firestore
- Bot can read from Firestore for historical analysis
- Currently only sends real-time alerts

### Dashboard (Sewerly)
- Dashboard receives same sensor data
- Bot operates in parallel
- No interference between systems

### Continuous Data Simulator
- Works with both Wokwi simulator and real ESP32
- Detects and processes threshold breaches automatically

## Security Considerations

- **Bot Token**: Keep `.env` file secret, don't commit to git
- **Chat IDs**: Determine who receives alerts
- **Rate Limiting**: Built-in protection from Telegram API
- **Data**: Only sends status and location info, no full readings stored in memory
