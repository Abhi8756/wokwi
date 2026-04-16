# Telegram Bot Documentation

Complete documentation for setting up and using the Telegram alert bot.

---

## 📚 Available Documentation

### [Telegram Bot Setup](TELEGRAM_BOT_SETUP.md) ⭐
**Step-by-step setup guide.**

Learn how to:
- Create a Telegram bot with BotFather
- Get your bot token
- Find your chat ID
- Configure the bot
- Test notifications

**Start here to set up Telegram alerts!**

---

### [Telegram Bot Implementation](TELEGRAM_BOT_IMPLEMENTATION.md)
**Implementation details and code structure.**

Covers:
- Code architecture
- Alert logic
- Message formatting
- Error handling
- Integration with bridge server

**Read this to understand the implementation!**

---

### [Telegram Bot Technical](TELEGRAM_BOT_TECHNICAL.md)
**Technical documentation and API details.**

Includes:
- Telegram Bot API usage
- Message types
- Rate limiting
- Security considerations
- Advanced features

**Read this for technical deep dive!**

---

### [Telegram Bot Overview](TELEGRAM_BOT.md)
**Feature overview and capabilities.**

Contains:
- Feature list
- Alert types
- Configuration options
- Usage examples

**Read this for a quick overview!**

---

## 🎯 Which Document Should I Read?

### I want to set up the bot
→ **[Telegram Bot Setup](TELEGRAM_BOT_SETUP.md)**

### I want to understand the code
→ **[Telegram Bot Implementation](TELEGRAM_BOT_IMPLEMENTATION.md)**

### I want technical details
→ **[Telegram Bot Technical](TELEGRAM_BOT_TECHNICAL.md)**

### I want a quick overview
→ **[Telegram Bot Overview](TELEGRAM_BOT.md)**

---

## 🚀 Quick Setup

### 1. Create Bot
Talk to [@BotFather](https://t.me/BotFather) on Telegram:
```
/newbot
```

### 2. Get Token
BotFather will give you a token like:
```
1234567890:ABCdefGHIjklMNOpqrsTUVwxyz
```

### 3. Get Chat ID
Talk to [@userinfobot](https://t.me/userinfobot):
```
/start
```

### 4. Configure
Add to `.env`:
```bash
TELEGRAM_BOT_TOKEN=your_token_here
TELEGRAM_CHAT_IDS=your_chat_id_here
```

### 5. Test
```bash
npm run bridge
```

Trigger an alert by moving CH4 slider above threshold!

---

## 📱 Alert Types

### CH4 Alerts
- **Warning**: CH4 > 1000 ppm
- **Danger**: CH4 > 5000 ppm

### H2S Alerts
- **Warning**: H2S > 10 ppm
- **Danger**: H2S > 20 ppm

### Water Level Alerts
- **Warning**: Water > 50 cm
- **Danger**: Water > 80 cm

### System Alerts
- Device offline
- Sensor malfunction
- Configuration changes

---

## 🔔 Alert Format

```
🚨 ALERT: CH4 Warning

Location: Main Street Manhole
Device: MANHOLE_001
Metric: CH4
Value: 1104.59 ppm
Threshold: 1000.00 ppm
Status: Warning
Time: 2026-04-16 04:08:56

⚠️ Methane levels elevated. Check ventilation.
```

---

## ⚙️ Configuration

### Environment Variables

```bash
# Required
TELEGRAM_BOT_TOKEN=your_bot_token

# Required - comma-separated for multiple recipients
TELEGRAM_CHAT_IDS=123456789,987654321

# Optional - customize alert messages
ALERT_MESSAGE_PREFIX="🚨 ALERT:"
ALERT_MESSAGE_SUFFIX="Please investigate immediately."
```

### Multiple Recipients

Send alerts to multiple users/groups:
```bash
TELEGRAM_CHAT_IDS=123456789,987654321,555666777
```

---

## 🔒 Security

### Bot Token
- Keep token secret
- Don't commit to git
- Use environment variables
- Rotate if compromised

### Chat IDs
- Only authorized users
- Verify chat IDs
- Remove old users
- Monitor bot usage

---

## 🐛 Troubleshooting

### Bot not sending messages
**Check**:
1. Token is correct
2. Chat ID is correct
3. Bot is not blocked by user
4. Bridge server is running

---

### Wrong chat ID
**Solution**: Talk to [@userinfobot](https://t.me/userinfobot) to get correct ID

---

### Rate limiting
**Cause**: Too many messages

**Solution**: Telegram limits to 30 messages/second. The bot handles this automatically.

---

## 💡 Features

### Current Features
- ✅ Real-time alert notifications
- ✅ Multiple alert levels (Warning, Danger)
- ✅ Multiple recipients
- ✅ Rich message formatting
- ✅ Device status updates
- ✅ Multi-location support

### Planned Features
- 🔄 Interactive commands (/status, /config)
- 🔄 Alert acknowledgment
- 🔄 Threshold configuration via bot
- 🔄 Historical data queries
- 🔄 Alert silencing

---

## 📊 Alert Statistics

The bot tracks:
- Total alerts sent
- Alerts by type
- Alerts by location
- Response times
- Delivery status

View in bridge server logs.

---

## 🔗 Related Documentation

- **[Getting Started](../getting-started/)** - Quick start guides
- **[MQTT Documentation](../mqtt/)** - MQTT setup
- **[VS Code Tasks](../vscode/)** - Task automation
- **[Main Documentation](../README.md)** - Complete index

---

## 📖 External Resources

- [Telegram Bot API](https://core.telegram.org/bots/api)
- [BotFather Documentation](https://core.telegram.org/bots#6-botfather)
- [Telegram Bot Best Practices](https://core.telegram.org/bots/faq)

---

## 💡 Pro Tips

1. **Test with one user first** before adding multiple recipients
2. **Use groups** for team notifications
3. **Customize messages** for your use case
4. **Monitor rate limits** if sending many alerts
5. **Keep token secure** - never commit to git

---

**Need help?** Check the setup guide or troubleshooting sections.
