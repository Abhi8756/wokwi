#!/bin/bash
# Complete startup script for IoT Manhole Monitoring System with MQTT

set -e  # Exit on error

echo "🚀 Starting IoT Manhole Monitoring System with MQTT"
echo "=================================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Step 1: Check Docker
echo "1️⃣  Checking Docker..."
if ! docker info > /dev/null 2>&1; then
    echo -e "${RED}❌ Docker is not running. Please start Docker first.${NC}"
    exit 1
fi
echo -e "${GREEN}✅ Docker is running${NC}"
echo ""

# Step 2: Start MQTT Broker
echo "2️⃣  Starting MQTT Broker (Mosquitto)..."
if docker ps | grep -q manhole-mqtt-broker; then
    echo -e "${YELLOW}⚠️  MQTT broker already running${NC}"
else
    docker-compose up -d mosquitto
    echo "⏳ Waiting for MQTT broker to initialize..."
    sleep 3
    
    if docker ps | grep -q manhole-mqtt-broker; then
        echo -e "${GREEN}✅ MQTT Broker started successfully${NC}"
    else
        echo -e "${RED}❌ Failed to start MQTT broker${NC}"
        echo "Check logs: docker logs manhole-mqtt-broker"
        exit 1
    fi
fi
echo ""

# Step 3: Check .env file
echo "3️⃣  Checking configuration..."
if [ ! -f .env ]; then
    echo -e "${YELLOW}⚠️  No .env file found, copying from .env.example${NC}"
    cp .env.example .env
    echo -e "${GREEN}✅ Created .env file${NC}"
    echo -e "${YELLOW}📝 Please edit .env file with your configuration${NC}"
else
    echo -e "${GREEN}✅ Configuration file exists${NC}"
fi
echo ""

# Step 4: Check Node modules
echo "4️⃣  Checking Node.js dependencies..."
if [ ! -d node_modules ]; then
    echo "📦 Installing dependencies..."
    npm install
fi
echo -e "${GREEN}✅ Dependencies ready${NC}"
echo ""

# Step 5: Display connection info
echo "=================================================="
echo -e "${GREEN}🎉 System Ready!${NC}"
echo "=================================================="
echo ""
echo "📡 MQTT Broker:"
echo "   Host: localhost"
echo "   Port: 1883"
echo "   Status: Running"
echo ""
echo "🔧 Next Steps:"
echo ""
echo "   1. Start the bridge server:"
echo "      ${YELLOW}npm run bridge${NC}"
echo ""
echo "   2. Start Wokwi simulator:"
echo "      - Open Wokwi VS Code extension"
echo "      - Click 'Start Simulation'"
echo ""
echo "   3. Monitor MQTT traffic (optional):"
echo "      ${YELLOW}docker exec -it manhole-mqtt-broker mosquitto_sub -t 'manhole/#' -v${NC}"
echo ""
echo "   4. View broker logs:"
echo "      ${YELLOW}docker logs -f manhole-mqtt-broker${NC}"
echo ""
echo "🛑 To stop everything:"
echo "   ${YELLOW}./stop-all.sh${NC}"
echo ""
echo "📚 For detailed setup guide, see:"
echo "   ${YELLOW}MQTT_SETUP_GUIDE.md${NC}"
echo ""
