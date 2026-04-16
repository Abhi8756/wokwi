#!/bin/bash
# Stop MQTT Broker

echo "🛑 Stopping Mosquitto MQTT Broker..."
docker-compose down

echo "✅ MQTT Broker stopped"
