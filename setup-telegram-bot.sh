#!/bin/bash
# Quick Telegram Bot Setup Script
# This script helps you configure the Telegram bot with minimal steps

echo "🤖 Manhole Telegram Bot Setup"
echo "=============================="
echo ""

# Check if .env file exists
if [ ! -f ".env" ]; then
    echo "⚠️  .env file not found. Creating from .env.example..."
    cp .env.example .env
    echo "✅ Created .env file"
else
    echo "✅ .env file exists"
fi

echo ""
echo "📋 Setup Steps:"
echo "1. Create a Telegram bot:"
echo "   - Open Telegram and find @BotFather"
echo "   - Send /newbot and follow the prompts"
echo "   - Copy your bot token (123456:ABC-DEF...)"
echo ""
echo "2. Get your Chat ID:"
echo "   - Send /start to your new bot"
echo "   - Send any message"
echo "   - Visit: https://api.telegram.org/botYOUR_TOKEN/getUpdates"
echo "   - Find your 'id' in the JSON response"
echo ""
echo "3. Update .env with your credentials:"
echo "   - TELEGRAM_BOT_TOKEN=your_bot_token"
echo "   - TELEGRAM_CHAT_IDS=your_chat_id"
echo ""
echo "📝 Edit your .env file with a text editor and add:"
echo "   TELEGRAM_BOT_TOKEN=<your_token>"
echo "   TELEGRAM_CHAT_IDS=<your_chat_id>"
echo ""
echo "4. Test the bot:"
echo "   npm run bridge"
echo "   # In another terminal:"
echo "   curl http://localhost:3001/api/telegram/test"
echo ""
echo "✨ Done! Your bot is ready to send alerts 🚀"
