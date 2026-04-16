# Telegram Bot Setup Guide

## Overview
The Manhole monitoring system now includes a Telegram bot that sends real-time notifications when gas thresholds (CH4 or H2S) are exceeded.

## Prerequisites
- A Telegram Bot Token (get from BotFather)
- Your Telegram Chat ID(s)

## Step 1: Create a Telegram Bot

1. Open Telegram and search for **@BotFather**
2. Start a chat and send `/newbot`
3. Follow the prompts:
   - Name: e.g., "Manhole Monitor Bot"
   - Username: e.g., "manhole_monitor_bot" (must be unique)
4. BotFather will provide you with a **Bot Token** (looks like: `123456:ABC-DEF1234ghIkl-zyx57W2v1u123ew11`)

## Step 2: Get Your Chat ID

### Option A: Using BotFather
1. Send `/start` to your new bot
2. Send any message to the bot
3. Go to: `https://api.telegram.org/bot<BOT_TOKEN>/getUpdates`
   - Replace `<BOT_TOKEN>` with your actual token
4. Look for `"chat"` → `"id"` in the JSON response

### Option B: Using a Chat ID Bot
1. Search for **@userinfobot**
2. Start the chat and send `/start`
3. It will show your Chat ID

## Step 3: Configure Environment Variables

Create or update your `.env` file in the Manhole directory:

```env
# Telegram Bot Configuration
TELEGRAM_BOT_TOKEN=your_bot_token_here
TELEGRAM_CHAT_IDS=your_chat_id_here,other_chat_id_if_multiple
```

### Example with Multiple Chat IDs:
```env
TELEGRAM_BOT_TOKEN=123456:ABC-DEF1234ghIkl-zyx57W2v1u123ew11
TELEGRAM_CHAT_IDS=987654321,123456789
```

## Step 4: Test the Bot

Once configured, test if the bot is working by running:

```bash
curl http://localhost:3001/api/telegram/test
```

You should receive a test message on Telegram.

## Alerting Behavior

### Threshold Levels:
- **CH4 (Methane)**
  - Warning: 700 ppm
  - Danger: 1000 ppm

- **H2S (Hydrogen Sulfide)**
  - Warning: 7 ppm
  - Danger: 10 ppm

- **Water Level**
  - Warning: 35 cm
  - Danger: 50 cm

### Alert Types:
1. **Status Change Alert** - Sent when overall status changes (Safe → Warning → Danger)
2. **Metric-Specific Alert** - Sent during Danger state for each metric that's critical
3. **Cooldown Protection** - Alerts from the same location/type are throttled for 5 minutes to prevent spam

### Alert Examples:

**⚠️ WARNING Alert:**
```
⚠️ WARNING

🔴 CH4: 850 ppm (warning)

📍 Location: MH-1023
🕐 Time: 2:45:30 PM
```

**🚨 DANGER Alert:**
```
🚨 DANGER ALERT

🔴 CH4: 1200 ppm (danger)
🟠 H2S: 12 ppm (danger)

📍 Location: MH-1023
🕐 Time: 2:50:15 PM
```

## Commands & Endpoints

### Test Telegram Bot
```bash
GET http://localhost:3001/api/telegram/test
```

Response:
```json
{
  "status": "ok",
  "message": "Test message sent successfully",
  "chatCount": 2
}
```

## Troubleshooting

### Bot doesn't send messages
1. **Invalid Token** - Verify token is correct in `.env`
2. **Invalid Chat ID** - Verify Chat ID is correct
3. **Bot not started** - Make sure you've sent `/start` to the bot in Telegram
4. **No TELEGRAM_BOT_TOKEN** - Check `.env` file is loaded correctly

### Check logs:
```bash
npm run bridge  # or npm run dev
```

Look for:
- `✅ Telegram bot initialized` - Bot is ready
- `⚠️ Telegram bot disabled` - Missing TELEGRAM_BOT_TOKEN
- `📲 Telegram alert sent` - Alert was successfully sent
- `❌ Failed to send Telegram message` - Error details

### Test API response
```bash
curl http://localhost:3001/api/telegram/test
```

Should show:
```json
{
  "status": "ok",
  "message": "Test message sent successfully",
  "chatCount": 1
}
```

## Features

✅ **Real-time Notifications** - Instant alerts on threshold breach
✅ **Smart Throttling** - Prevents alert spam (5-minute cooldown per alert type)
✅ **Multiple Recipients** - Send to multiple chat IDs
✅ **Status Tracking** - Only alerts on actual status changes
✅ **Metric Details** - Shows current value and threshold
✅ **Location Info** - Identifies which manhole triggered the alert
✅ **Timestamp** - Knows exactly when the alert occurred

## Environment Variables Reference

| Variable | Required | Example | Description |
|----------|----------|---------|-------------|
| `TELEGRAM_BOT_TOKEN` | Yes | `123456:ABC...` | Your Telegram bot token from BotFather |
| `TELEGRAM_CHAT_IDS` | Yes | `987654321` | Comma-separated chat IDs to receive alerts |

## Support

If you encounter issues:
1. Verify bot token and chat IDs are correct
2. Check that the bot has permission to send messages
3. Review logs for error messages
4. Test with `/api/telegram/test` endpoint
