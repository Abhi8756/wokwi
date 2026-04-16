#include "mqtt_tls.h"

// Global TLS client instance (only created if TLS enabled)
WiFiClientSecure* wifiClientSecure = nullptr;