#include "ntp.h"

// Initialize global NTP state
NTPState ntpState = {
  NTP_STATUS_NOT_INITIALIZED,  // status
  0,                           // lastSyncTime
  0,                           // lastSyncAttempt
  0,                           // syncAttempts
  false,                       // initialized
  0,                           // bootTime
  0,                           // bootMillis
  NTP_TIMEZONE_OFFSET,         // timezoneOffset
  NTP_DAYLIGHT_OFFSET          // daylightOffset
};