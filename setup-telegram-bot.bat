@echo off
REM Quick Telegram Bot Setup Script for Windows
REM This script helps you configure the Telegram bot

echo.
echo ^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*
echo      Manhole Telegram Bot Setup
echo ^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*^*
echo.

REM Check if .env file exists
if not exist ".env" (
    echo [^[91m^^!^[0m] .env file not found. Creating from .env.example...
    copy .env.example .env
    echo [^[92m✓^[0m] Created .env file
) else (
    echo [^[92m✓^[0m] .env file exists
)

echo.
echo ^[Setup Steps:^]
echo 1. Create a Telegram bot:
echo    - Open Telegram and find @BotFather
echo    - Send /newbot and follow the prompts
echo    - Copy your bot token ^(123456:ABC-DEF...^)
echo.
echo 2. Get your Chat ID:
echo    - Send /start to your new bot
echo    - Send any message
echo    - Visit: https://api.telegram.org/botYOUR_TOKEN/getUpdates
echo    - Find your 'id' in the JSON response
echo.
echo 3. Update .env with your credentials:
echo    - TELEGRAM_BOT_TOKEN=your_bot_token
echo    - TELEGRAM_CHAT_IDS=your_chat_id
echo.
echo 4. Test the bridge:
echo    npm run bridge
echo.
echo    In another terminal, test Telegram:
echo    curl http://localhost:3001/api/telegram/test
echo.
echo [^[92m✨ Done! Your bot is ready to send alerts 🚀^[0m]
echo.
pause
