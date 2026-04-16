# ⚡ Telegram Bot Quick Start

## Get Alerts in 5 Minutes

### Step 1: Get Your Bot Token
1. Open Telegram → Search for **@BotFather**
2. Send `/newbot`
3. Follow instructions (name your bot, choose a username)
4. **Copy the token** (looks like: `123456:ABC-DEF...`)

### Step 2: Get Your Chat ID
1. Send your new bot a message
2. Visit: `https://api.telegram.org/bot<YOUR_TOKEN>/getUpdates`
   - Replace `<YOUR_TOKEN>` with your actual token
3. Find the number in `"id":` under `"chat"`

### Step 3: Configure Your Project
Edit `.env` in the Manhole folder:

```env
TELEGRAM_BOT_TOKEN=your_token_here
TELEGRAM_CHAT_IDS=your_chat_id_here
```

### Step 4: Test It
```bash
npm run bridge
```

In another terminal:
```bash
curl http://localhost:3001/api/telegram/test
```

You should get a message on Telegram! 🎉

## Done! 

The bot will now send alerts when:
- **CH4** exceeds 1000 ppm 🔴
- **H2S** exceeds 10 ppm 🟠  
- **Water Level** exceeds 50 cm 🔵

## Troubleshooting

**No message received?**
- ✅ Make sure you sent `/start` to the bot first
- ✅ Check token is correct in `.env`
- ✅ Check Chat ID is correct (should be a number)

**Need more help?**
- See `TELEGRAM_BOT_SETUP.md` for detailed setup
- See `TELEGRAM_BOT_TECHNICAL.md` for architecture details

---

**Questions?** Check the logs:
```bash
npm run bridge
```
Look for: ✅ `Telegram bot initialized` or ⚠️ errors
