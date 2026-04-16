#!/bin/bash
# Test MQTT broker connectivity

echo "🧪 Testing MQTT Broker..."
echo ""

# Check if broker is running
if ! docker ps | grep -q manhole-mqtt-broker; then
    echo "❌ MQTT broker is not running"
    echo "Start it with: ./start-mqtt.sh"
    exit 1
fi

echo "✅ MQTT broker is running"
echo ""

# Test publish and subscribe
echo "📡 Testing MQTT publish/subscribe..."
echo ""

# Subscribe in background
docker exec manhole-mqtt-broker mosquitto_sub -t "test/topic" -C 1 &
SUB_PID=$!

# Wait a moment
sleep 1

# Publish test message
docker exec manhole-mqtt-broker mosquitto_pub -t "test/topic" -m "Hello MQTT!"

# Wait for subscriber
wait $SUB_PID

echo ""
echo "✅ MQTT broker is working correctly!"
echo ""
echo "📊 Broker Information:"
docker exec manhole-mqtt-broker mosquitto_sub -t '$SYS/broker/version' -C 1

echo ""
echo "🔍 To monitor all manhole topics:"
echo "   docker exec -it manhole-mqtt-broker mosquitto_sub -t 'manhole/#' -v"
