#!/bin/bash
# Stop all services for IoT Manhole Monitoring System

echo "🛑 Stopping IoT Manhole Monitoring System..."
echo ""

# Stop Docker containers
echo "Stopping MQTT Broker..."
docker-compose down

echo ""
echo "✅ All services stopped"
echo ""
echo "Note: Bridge server must be stopped manually (Ctrl+C)"
