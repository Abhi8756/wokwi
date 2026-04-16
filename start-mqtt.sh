#!/bin/bash
# Start MQTT Broker for IoT Manhole Monitoring System

echo "🚀 Starting Mosquitto MQTT Broker..."
echo ""

# Check if Docker is running
if ! docker info > /dev/null 2>&1; then
    echo "❌ Docker is not running. Please start Docker first."
    exit 1
fi

# Start Mosquitto using Docker Compose
docker-compose up -d mosquitto

# Wait for broker to start
echo ""
echo "⏳ Waiting for MQTT broker to start..."
sleep 3

# Check if broker is running
if docker ps | grep -q manhole-mqtt-broker; then
    echo "✅ MQTT Broker is running!"
    echo ""
    echo "📡 MQTT Broker Details:"
    echo "   Host: localhost"
    echo "   Port: 1883"
    echo "   Protocol: MQTT (non-TLS)"
    echo ""
    echo "📊 To view broker logs:"
    echo "   docker logs -f manhole-mqtt-broker"
    echo ""
    echo "🛑 To stop the broker:"
    echo "   docker-compose down"
    echo ""
else
    echo "❌ Failed to start MQTT broker"
    echo "Check logs with: docker-compose logs mosquitto"
    exit 1
fi
