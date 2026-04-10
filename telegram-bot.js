const { Telegraf } = require('telegraf');

class TelegramAlertBot {
  constructor(botToken, chatIds = []) {
    this.botToken = botToken;
    this.chatIds = Array.isArray(chatIds) ? chatIds : [chatIds];
    this.bot = botToken ? new Telegraf(botToken) : null;
    this.previousStatus = {}; // Track previous status to avoid duplicate alerts
    this.alertCooldown = new Map(); // Cooldown per chat to avoid spam
    this.COOLDOWN_DURATION = 300000; // 5 minutes cooldown between same alert type
    this.enabled = !!botToken;

    if (this.enabled) {
      console.log('✅ Telegram bot initialized');
    } else {
      console.warn('⚠️  Telegram bot disabled (no TELEGRAM_BOT_TOKEN found)');
    }
  }

  /**
   * Check if alert should be throttled
   */
  shouldThrottle(chatId, alertType) {
    const key = `${chatId}-${alertType}`;
    const lastAlert = this.alertCooldown.get(key);
    const now = Date.now();

    if (lastAlert && (now - lastAlert) < this.COOLDOWN_DURATION) {
      return true; // Throttle this alert
    }

    this.alertCooldown.set(key, now);
    return false;
  }

  /**
   * Format a measurement alert message
   */
  formatMetricAlert(metricName, value, status, threshold, location) {
    const emoji = status === 'danger' ? '🚨' : '⚠️';
    const statusText = status === 'danger' ? 'DANGER' : 'WARNING';
    
    return `${emoji} <b>${statusText}</b>\n\n<b>${metricName}</b>\n📊 Current: ${value}\n⚠️ Threshold: ${threshold}\n📍 Location: ${location.id}`;
  }

  /**
   * Format overall alert message
   */
  formatOverallAlert(reading, location) {
    const emoji = reading.status === 'Danger' ? '🚨' : '⚠️';
    const alerts = [];

    if (reading.metricStatus.ch4 !== 'safe') {
      alerts.push(`🔴 CH4: ${reading.ch4} ppm (${reading.metricStatus.ch4})`);
    }
    if (reading.metricStatus.h2s !== 'safe') {
      alerts.push(`🟠 H2S: ${reading.h2s} ppm (${reading.metricStatus.h2s})`);
    }
    if (reading.metricStatus.waterLevel !== 'safe') {
      alerts.push(`🔵 Water Level: ${reading.waterLevel}cm (${reading.metricStatus.waterLevel})`);
    }

    const timestamp = new Date(reading.lastUpdated).toLocaleTimeString();

    return `${emoji} <b>${reading.status.toUpperCase()} ALERT</b>\n\n${alerts.join('\n')}\n\n📍 <b>Location:</b> ${location.id}\n🕐 <b>Time:</b> ${timestamp}`;
  }

  /**
   * Send a message to all configured chat IDs
   */
  async sendMessage(message, options = {}) {
    if (!this.enabled || !this.bot || this.chatIds.length === 0) {
      return false;
    }

    const sendPromises = this.chatIds.map((chatId) => {
      return this.bot
        .telegram.sendMessage(chatId, message, {
          parse_mode: 'HTML',
          disable_web_page_preview: true,
          ...options,
        })
        .catch((error) => {
          console.error(
            `❌ Failed to send Telegram message to ${chatId}:`,
            error.message
          );
          return null;
        });
    });

    try {
      const results = await Promise.all(sendPromises);
      const successCount = results.filter((r) => r !== null).length;
      return successCount > 0;
    } catch (error) {
      console.error('❌ Telegram sendMessage error:', error.message);
      return false;
    }
  }

  /**
   * Check reading against thresholds and send alerts
   */
  async checkAndAlert(reading) {
    if (!this.enabled || !this.bot) {
      return;
    }

    const location = reading.location || { id: 'Unknown' };
    const statusKey = `${location.id}-status`;
    const previousStatus = this.previousStatus[statusKey];

    // Send alert if status changed or became critical
    if (reading.status !== previousStatus || reading.status === 'Danger') {
      // Check individual metrics for detailed alerts
      const ch4Changed = reading.metricStatus.ch4 !== 'safe';
      const h2sChanged = reading.metricStatus.h2s !== 'safe';
      const waterChanged = reading.metricStatus.waterLevel !== 'safe';

      // Main alert for overall status change
      if (reading.status !== previousStatus) {
        const shouldSend = !this.shouldThrottle(
          this.chatIds[0],
          `${location.id}-${reading.status}`
        );

        if (shouldSend) {
          const message = this.formatOverallAlert(reading, location);
          await this.sendMessage(message);
          console.log(
            `📲 Telegram alert sent - ${reading.status} at ${location.id}`
          );
        }

        this.previousStatus[statusKey] = reading.status;
      }

      // Additional metric-specific alerts for danger state
      if (reading.status === 'Danger') {
        const dangersKey = `${location.id}-dangers`;
        const previousDangers = this.previousStatus[dangersKey] || {};

        if (
          ch4Changed &&
          previousDangers.ch4 !== reading.metricStatus.ch4
        ) {
          const message = this.formatMetricAlert(
            'CH4 (Methane)',
            reading.ch4 + ' ppm',
            reading.metricStatus.ch4,
            reading.thresholds.ch4Danger,
            location
          );
          await this.sendMessage(message);
        }

        if (
          h2sChanged &&
          previousDangers.h2s !== reading.metricStatus.h2s
        ) {
          const message = this.formatMetricAlert(
            'H2S (Hydrogen Sulfide)',
            reading.h2s + ' ppm',
            reading.metricStatus.h2s,
            reading.thresholds.h2sDanger,
            location
          );
          await this.sendMessage(message);
        }

        if (
          waterChanged &&
          previousDangers.waterLevel !== reading.metricStatus.waterLevel
        ) {
          const message = this.formatMetricAlert(
            'Water Level',
            reading.waterLevel + ' cm',
            reading.metricStatus.waterLevel,
            reading.thresholds.waterLevelDanger,
            location
          );
          await this.sendMessage(message);
        }

        this.previousStatus[dangersKey] = {
          ch4: reading.metricStatus.ch4,
          h2s: reading.metricStatus.h2s,
          waterLevel: reading.metricStatus.waterLevel,
        };
      }
    }
  }

  /**
   * Send a test message
   */
  async sendTest() {
    const testMessage = `✅ <b>Telegram Bot Test</b>\n\nBot is connected and ready to send alerts!\n\n📊 This is a test notification.`;
    return await this.sendMessage(testMessage);
  }
}

module.exports = TelegramAlertBot;
