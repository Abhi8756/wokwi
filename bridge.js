const fs = require("fs");
const path = require("path");
const express = require("express");
const crypto = require("crypto");
const http = require("http");
const WebSocket = require("ws");
const mqtt = require("mqtt");
const TelegramAlertBot = require("./telegram-bot");

console.log("🚀 Starting Wokwi Bridge Server...");

// Firebase Admin SDK
let admin = null;
let db = null;

// Try to load Firebase Admin SDK
try {
  admin = require("firebase-admin");
  console.log("✅ Firebase Admin SDK loaded");
} catch (error) {
  console.warn("⚠️  Firebase Admin SDK not installed. Install with: npm install firebase-admin");
}

// Load environment file function
function loadEnvFile(filePath) {
  try {
    if (fs.existsSync(filePath)) {
      const envContent = fs.readFileSync(filePath, 'utf8');
      const lines = envContent.split('\n');
      
      for (const line of lines) {
        const trimmedLine = line.trim();
        if (trimmedLine && !trimmedLine.startsWith('#')) {
          const [key, ...valueParts] = trimmedLine.split('=');
          if (key && valueParts.length > 0) {
            const value = valueParts.join('=').replace(/^["']|["']$/g, '');
            process.env[key.trim()] = value;
          }
        }
      }
      console.log("✅ Environment file loaded");
    } else {
      console.log("ℹ️  No .env file found");
    }
  } catch (error) {
    console.warn(`⚠️  Could not load .env file: ${error.message}`);
  }
}

loadEnvFile(path.join(__dirname, ".env"));
// Initialize Telegram Bot (Mock for now)
let telegramBot = {
  enabled: false,
  checkAndAlert: async () => {},
  sendAlert: async () => {},
  sendTest: async () => false
};

console.log("✅ Telegram bot (mock) initialized");

const PORT = Number(process.env.PORT || process.env.BRIDGE_PORT || 3001);
const FIREBASE_KEY_PATH = path.join(__dirname, "serviceAccountKey.json");
const READINGS_FILE_PATH = path.join(__dirname, "sensor-readings.json");
const FIRESTORE_COLLECTION = process.env.FIRESTORE_COLLECTION || "sensor_readings";
const ALERTS_COLLECTION = process.env.ALERTS_COLLECTION || "alert_history";
const STORAGE_BACKEND = (process.env.SENSOR_STORAGE_BACKEND || "auto").toLowerCase();
const MAX_HISTORY = 200;
const MANHOLE_DEPTH_CM = 100;

// API Key Authentication Configuration (REQ-003)
const API_KEY = process.env.API_KEY || process.env.BRIDGE_API_KEY;
const API_KEY_HEADER = "X-API-Key";
const REQUIRE_API_KEY = process.env.REQUIRE_API_KEY !== "false"; // Default to true for security

// Network Retry Configuration (REQ-006)
const RETRY_CONFIG = {
  maxAttempts: parseInt(process.env.MAX_RETRY_ATTEMPTS || "5"),
  baseDelay: parseInt(process.env.BASE_RETRY_DELAY || "1000"), // 1 second
  maxDelay: parseInt(process.env.MAX_RETRY_DELAY || "60000"),  // 60 seconds
  jitterPercent: parseFloat(process.env.RETRY_JITTER_PERCENT || "0.2"), // ±20%
  backoffMultiplier: parseFloat(process.env.BACKOFF_MULTIPLIER || "2.0")
};

// MQTT Configuration (REQ-007)
const MQTT_CONFIG = {
  broker: process.env.MQTT_BROKER || "mqtt://localhost:1883",
  username: process.env.MQTT_USERNAME || "",
  password: process.env.MQTT_PASSWORD || "",
  clientId: `bridge_server_${Date.now()}`,
  topicPrefix: process.env.MQTT_TOPIC_PREFIX || "manhole",
  locationId: process.env.MQTT_LOCATION_ID || "location_001",
  keepalive: 60,
  reconnectPeriod: 5000,
  connectTimeout: 30000
};

const DEFAULT_LOCATION = {
  id: "MH-1023",
  lat: 12.9692,
  lng: 79.1559,
};

// Task 2.3: Multi-Location Support (REQ-025)
const LOCATIONS_CONFIG = {
  // Support multiple predefined locations
  locations: [
    {
      id: "MH-1023",
      name: "Main Street Manhole",
      lat: 12.9692,
      lng: 79.1559,
      description: "Primary monitoring location on Main Street"
    },
    {
      id: "MH-2045", 
      name: "Industrial Zone Manhole",
      lat: 12.9712,
      lng: 79.1580,
      description: "Industrial area monitoring point"
    },
    {
      id: "MH-3067",
      name: "Residential Area Manhole", 
      lat: 12.9650,
      lng: 79.1520,
      description: "Residential district monitoring location"
    }
  ],
  defaultLocationId: process.env.DEFAULT_LOCATION_ID || "MH-1023"
};

// Helper function to get location by ID
function getLocationById(locationId) {
  return LOCATIONS_CONFIG.locations.find(loc => loc.id === locationId) || 
         LOCATIONS_CONFIG.locations.find(loc => loc.id === LOCATIONS_CONFIG.defaultLocationId) ||
         DEFAULT_LOCATION;
}

// Helper function to get all locations
function getAllLocations() {
  return LOCATIONS_CONFIG.locations;
}

// Storage Backend Configuration and Initialization
let storageMode = "memory"; // Default to memory storage

// Determine storage backend
function determineStorageBackend() {
  if (STORAGE_BACKEND === "firestore") {
    return "firestore";
  } else if (STORAGE_BACKEND === "memory") {
    return "memory";
  } else if (STORAGE_BACKEND === "auto") {
    // Auto-detect based on Firebase credentials
    return (process.env.GOOGLE_APPLICATION_CREDENTIALS || process.env.FIREBASE_PROJECT_ID) ? "firestore" : "memory";
  }
  return "memory";
}

function isFirestorePrimary() {
  return storageMode === "firestore" && db !== null;
}

// Initialize Firebase Admin SDK
console.log("🔧 Initializing Firebase...");

// Check if Firebase credentials are available
const hasFirebaseCredentials = 
  process.env.GOOGLE_APPLICATION_CREDENTIALS || 
  (process.env.FIREBASE_PROJECT_ID && process.env.FIREBASE_CLIENT_EMAIL && process.env.FIREBASE_PRIVATE_KEY);

if (admin && hasFirebaseCredentials) {
  try {
    // Use individual env vars if available, otherwise use application default
    if (process.env.FIREBASE_PROJECT_ID && process.env.FIREBASE_CLIENT_EMAIL && process.env.FIREBASE_PRIVATE_KEY) {
      // Initialize with individual environment variables
      const serviceAccount = {
        projectId: process.env.FIREBASE_PROJECT_ID,
        clientEmail: process.env.FIREBASE_CLIENT_EMAIL,
        privateKey: process.env.FIREBASE_PRIVATE_KEY.replace(/\\n/g, '\n')
      };
      
      admin.initializeApp({
        credential: admin.credential.cert(serviceAccount),
        projectId: process.env.FIREBASE_PROJECT_ID
      });
      
      console.log('✅ Firebase initialized with environment variables');
    } else {
      // Use application default credentials (GOOGLE_APPLICATION_CREDENTIALS)
      admin.initializeApp({
        credential: admin.credential.applicationDefault(),
        projectId: process.env.FIREBASE_PROJECT_ID
      });
      
      console.log('✅ Firebase initialized with application default credentials');
    }
    
    db = admin.firestore();
    storageMode = determineStorageBackend();
    console.log('✅ Firebase connected. Firestore collection:', FIRESTORE_COLLECTION);
    
    // Check if we're using Firestore and inform about index requirements
    if (isFirestorePrimary()) {
      console.log('📋 FIRESTORE INDEX SETUP REQUIRED:');
      console.log('   For optimal performance, create a composite index in Firebase Console:');
      console.log('   Collection: sensor_readings');
      console.log('   Fields: location.id (Ascending), timestamp (Descending)');
      console.log('   🔗 Auto-create link will be provided in logs when queries fail');
      console.log('   ℹ️  System will use fallback queries until index is created');
    }
  } catch (error) {
    console.error('❌ Firebase initialization failed:', error.message);
    console.log('📝 Falling back to in-memory storage');
    storageMode = "memory";
    db = null;
    admin = null;
  }
} else {
  console.log('📝 No Firebase credentials found. Using in-memory storage.');
  storageMode = "memory";
}

// Network State Tracking (REQ-006)
const networkState = {
  firestore: {
    connected: false,
    retryCount: 0,
    lastAttempt: null
  },
  websocket: {
    lastBroadcast: null
  }
};

// Alert Management State (REQ-026)
const ALERT_STATUS = {
  ACTIVE: 'active',
  ACKNOWLEDGED: 'acknowledged', 
  RESOLVED: 'resolved',
  SILENCED: 'silenced'
};

const MAX_ALERT_HISTORY = 1000;
let alertHistory = [];
let activeAlerts = new Map(); // Track active alerts by key

// In-memory storage
let historicalReadings = [];

// Utility Functions

// Load environment file function
function loadEnvFile(filePath) {
  try {
    if (fs.existsSync(filePath)) {
      const envContent = fs.readFileSync(filePath, 'utf8');
      const lines = envContent.split('\n');
      
      for (const line of lines) {
        const trimmedLine = line.trim();
        if (trimmedLine && !trimmedLine.startsWith('#')) {
          const [key, ...valueParts] = trimmedLine.split('=');
          if (key && valueParts.length > 0) {
            const value = valueParts.join('=').replace(/^["']|["']$/g, '');
            process.env[key.trim()] = value;
          }
        }
      }
    }
  } catch (error) {
    console.warn(`⚠️  Could not load .env file: ${error.message}`);
  }
}

// Network Retry Logic (REQ-006)
async function retryWithBackoff(operation, operationName, customConfig = {}) {
  const config = { ...RETRY_CONFIG, ...customConfig };
  let lastError;
  
  for (let attempt = 1; attempt <= config.maxAttempts; attempt++) {
    try {
      networkState.firestore.lastAttempt = new Date().toISOString();
      const result = await operation();
      
      // Reset retry count on success
      if (networkState.firestore.retryCount > 0) {
        networkState.firestore.retryCount = 0;
      }
      
      return result;
    } catch (error) {
      lastError = error;
      networkState.firestore.retryCount = attempt;
      
      if (attempt === config.maxAttempts) {
        console.error(`❌ ${operationName} failed after ${config.maxAttempts} attempts: ${error.message}`);
        break;
      }
      
      // Calculate delay with exponential backoff and jitter
      const baseDelay = Math.min(
        config.baseDelay * Math.pow(config.backoffMultiplier, attempt - 1),
        config.maxDelay
      );
      
      const jitter = baseDelay * config.jitterPercent * (Math.random() * 2 - 1);
      const delay = Math.max(0, baseDelay + jitter);
      
      console.warn(`⚠️  ${operationName} failed (attempt ${attempt}/${config.maxAttempts}): ${error.message}`);
      console.log(`🔄 Retrying ${operationName} in ${Math.round(delay)}ms...`);
      
      await new Promise(resolve => setTimeout(resolve, delay));
    }
  }
  
  throw lastError;
}

// Data mapping and normalization functions
function mapStoredReading(data, docId) {
  if (!data) return null;
  
  try {
    return {
      id: docId,
      ch4: data.ch4 || 0,
      h2s: data.h2s || 0,
      waterLevel: data.waterLevel || 0,
      waterDistance: data.waterDistance || data.water || 0,
      battery: data.battery || 100,
      sensorStatus: data.sensorStatus || 'ok',
      status: data.status || 'Safe',
      metricStatus: data.metricStatus || {},
      alert: data.alert || false,
      source: data.source || 'unknown',
      lastUpdated: data.timestamp?.toDate?.()?.toISOString() || data.lastUpdated || data.timestamp,
      location: data.location || DEFAULT_LOCATION
    };
  } catch (error) {
    console.error('Error mapping stored reading:', error);
    return null;
  }
}

function normalizeSensorData(rawData) {
  try {
    const location = getLocationById(rawData.locationId || DEFAULT_LOCATION.id);
    
    const reading = {
      ch4: parseFloat(rawData.ch4) || 0,
      h2s: parseFloat(rawData.h2s) || 0,
      waterLevel: parseFloat(rawData.waterLevel) || 0,
      waterDistance: parseFloat(rawData.waterDistance || rawData.water) || 0,
      battery: parseFloat(rawData.battery) || 100,
      sensorStatus: rawData.sensorStatus || 'ok',
      source: rawData.source || 'http',
      lastUpdated: rawData.lastUpdated || new Date().toISOString(),
      location: location
    };
    
    // Calculate water level from distance if needed
    if (reading.waterLevel === 0 && reading.waterDistance > 0) {
      reading.waterLevel = Math.max(0, MANHOLE_DEPTH_CM - reading.waterDistance);
    }
    
    // Determine metric-specific statuses
    reading.metricStatus = {
      ch4: getMetricStatus(reading.ch4, thresholds.ch4Warning, thresholds.ch4Danger),
      h2s: getMetricStatus(reading.h2s, thresholds.h2sWarning, thresholds.h2sDanger),
      waterLevel: getMetricStatus(reading.waterLevel, thresholds.waterLevelWarning, thresholds.waterLevelDanger)
    };
    
    // Determine overall status
    reading.status = calculateOverallStatus([reading.metricStatus]);
    reading.alert = reading.status === 'Warning' || reading.status === 'Danger';
    
    return reading;
  } catch (error) {
    console.error('Error normalizing sensor data:', error);
    return null;
  }
}

function getMetricStatus(value, warningThreshold, dangerThreshold) {
  if (value >= dangerThreshold) return 'danger';
  if (value >= warningThreshold) return 'warning';
  return 'safe';
}

function addReading(reading) {
  historicalReadings.push(reading);
  
  // Keep only the most recent readings in memory
  if (historicalReadings.length > MAX_HISTORY) {
    historicalReadings = historicalReadings.slice(-MAX_HISTORY);
  }
}

// Thresholds Configuration (REQ-001)
const thresholds = {
  ch4Warning: parseFloat(process.env.CH4_WARNING_THRESHOLD || "1000"),
  ch4Danger: parseFloat(process.env.CH4_DANGER_THRESHOLD || "1500"),
  h2sWarning: parseFloat(process.env.H2S_WARNING_THRESHOLD || "10"),
  h2sDanger: parseFloat(process.env.H2S_DANGER_THRESHOLD || "15"),
  waterLevelWarning: parseFloat(process.env.WATER_WARNING_THRESHOLD || "70"),
  waterLevelDanger: parseFloat(process.env.WATER_DANGER_THRESHOLD || "85"),
  // Hysteresis values to prevent alert flapping (REQ-026)
  hysteresis: {
    h2s: parseFloat(process.env.H2S_HYSTERESIS || "1.0"),      // 1 ppm hysteresis
    ch4: parseFloat(process.env.CH4_HYSTERESIS || "50.0"),     // 50 ppm hysteresis  
    waterLevel: parseFloat(process.env.WATER_HYSTERESIS || "2.0") // 2 cm hysteresis
  }
};

// Previous metric status for hysteresis logic (REQ-004)
// Initialize with 'safe' status for all metrics
const previousMetricStatus = {
  ch4: 'safe',
  h2s: 'safe',
  waterLevel: 'safe'
};

// Task 2.4.5: Alert Escalation Configuration (REQ-026)
const ESCALATION_CONFIG = {
  WARNING_ESCALATION_TIME: parseFloat(process.env.WARNING_ESCALATION_MINUTES || "15"), // 15 minutes
  DANGER_ESCALATION_TIME: parseFloat(process.env.DANGER_ESCALATION_MINUTES || "5"),   // 5 minutes
  CRITICAL_ESCALATION_TIME: parseFloat(process.env.CRITICAL_ESCALATION_MINUTES || "2"), // 2 minutes
  MAX_ESCALATION_LEVEL: parseInt(process.env.MAX_ESCALATION_LEVEL || "3"),
  ESCALATION_ACTIONS: {
    1: 'repeat_notification',
    2: 'supervisor_notification', 
    3: 'emergency_protocol'
  }
};

// Missing function implementations

async function storeReadingInFirestore(reading) {
  if (!db || !admin) {
    return null;
  }

  return retryWithBackoff(async () => {
    const timestamp = admin.firestore.Timestamp.fromDate(new Date(reading.lastUpdated));
    
    const docRef = await db.collection(FIRESTORE_COLLECTION).add({
      ...reading,
      timestamp: timestamp
    });

    networkState.firestore.connected = true;
    return docRef.id;
  }, "Firestore sensor storage")
  .catch((error) => {
    networkState.firestore.connected = false;
    console.error(`❌ Failed to store reading in Firestore: ${error.message}`);
    return null;
  });
}

// WebSocket functionality
let wss = null;

function broadcastSensorUpdate(reading) {
  if (!wss) return;
  
  const message = JSON.stringify({
    type: 'sensor_update',
    data: reading,
    timestamp: new Date().toISOString()
  });
  
  wss.clients.forEach(client => {
    if (client.readyState === WebSocket.OPEN) {
      try {
        client.send(message);
        networkState.websocket.lastBroadcast = new Date().toISOString();
      } catch (error) {
        console.error('WebSocket broadcast error:', error);
      }
    }
  });
}

// Alert escalation monitoring
let escalationInterval = null;

function startAlertEscalationMonitoring() {
  // Check for alert escalation every 2 minutes
  const intervalMinutes = 2;
  
  escalationInterval = setInterval(async () => {
    try {
      const escalatedAlerts = await checkAlertEscalation();
      if (escalatedAlerts.length > 0) {
        console.log(`🔄 Alert escalation monitoring: ${escalatedAlerts.length} alerts escalated`);
      }
    } catch (error) {
      console.error('❌ Alert escalation monitoring error:', error);
    }
  }, intervalMinutes * 60 * 1000);
  
  console.log(`🔄 Alert escalation monitoring started (checking every ${intervalMinutes} minutes)`);
}

async function processEscalationAction(escalation) {
  const { alert, escalationLevel, action } = escalation;
  
  try {
    switch (action) {
      case 'repeat_notification':
        // Send repeat notification via Telegram
        if (telegramBot && telegramBot.enabled) {
          const message = `🔄 ESCALATED ALERT (Level ${escalationLevel})\n` +
                         `${alert.message}\n` +
                         `Location: ${alert.locationId}\n` +
                         `Time: ${new Date(alert.createdAt).toLocaleString()}`;
          await telegramBot.sendAlert(message);
        }
        break;
        
      case 'supervisor_notification':
        // Send supervisor notification
        console.log(`📧 Supervisor notification triggered for alert ${alert.id}`);
        if (telegramBot && telegramBot.enabled) {
          const message = `🚨 SUPERVISOR ALERT (Level ${escalationLevel})\n` +
                         `URGENT: ${alert.message}\n` +
                         `Location: ${alert.locationId}\n` +
                         `Requires immediate attention!`;
          await telegramBot.sendAlert(message);
        }
        break;
        
      case 'emergency_protocol':
        // Trigger emergency protocol
        console.log(`🚨 EMERGENCY PROTOCOL activated for alert ${alert.id}`);
        if (telegramBot && telegramBot.enabled) {
          const message = `🚨🚨 EMERGENCY PROTOCOL ACTIVATED 🚨🚨\n` +
                         `CRITICAL: ${alert.message}\n` +
                         `Location: ${alert.locationId}\n` +
                         `IMMEDIATE ACTION REQUIRED!`;
          await telegramBot.sendAlert(message);
        }
        break;
        
      default:
        console.warn(`⚠️  Unknown escalation action: ${action}`);
    }
  } catch (error) {
    console.error(`❌ Failed to process escalation action ${action}:`, error);
  }
}

async function getLatestReadingFromFirestore() {
  if (!db) {
    return null;
  }

  return retryWithBackoff(async () => {
    const snapshot = await db
      .collection(FIRESTORE_COLLECTION)
      .orderBy("timestamp", "desc")
      .limit(1)
      .get();

    networkState.firestore.connected = true;

    if (snapshot.empty) {
      return null;
    }

    return mapStoredReading(snapshot.docs[0].data(), snapshot.docs[0].id);
  }, "Firestore read latest")
  .catch((error) => {
    networkState.firestore.connected = false;
    console.error(`❌ Failed to get latest reading from Firestore: ${error.message}`);
    return null;
  });
}

async function getHistoryFromFirestore(limit) {
  if (!db) {
    return [];
  }

  return retryWithBackoff(async () => {
    const snapshot = await db
      .collection(FIRESTORE_COLLECTION)
      .orderBy("timestamp", "desc")
      .limit(limit)
      .get();

    networkState.firestore.connected = true;

    return snapshot.docs
      .map((doc) => mapStoredReading(doc.data(), doc.id))
      .filter(Boolean)
      .reverse(); // Return in chronological order
  }, "Firestore read history")
  .catch((error) => {
    networkState.firestore.connected = false;
    console.error(`❌ Failed to get history from Firestore: ${error.message}`);
    return [];
  });
}

// Task 2.3: Location-specific helper functions
async function getLatestReadingByLocation(locationId) {
  if (isFirestorePrimary()) {
    return getLatestReadingFromFirestoreByLocation(locationId);
  }
  
  // Filter local readings by location ID
  const locationReadings = historicalReadings.filter(reading => 
    reading.location && reading.location.id === locationId
  );
  
  return locationReadings.length > 0 
    ? locationReadings[locationReadings.length - 1] 
    : null;
}

async function getLatestReadingFromFirestoreByLocation(locationId) {
  if (!db) {
    return null;
  }

  return retryWithBackoff(async () => {
    // Try compound query first (requires index)
    try {
      const snapshot = await db
        .collection(FIRESTORE_COLLECTION)
        .where("location.id", "==", locationId)
        .orderBy("timestamp", "desc")
        .limit(1)
        .get();

      networkState.firestore.connected = true;

      if (snapshot.empty) {
        return null;
      }

      return mapStoredReading(snapshot.docs[0].data(), snapshot.docs[0].id);
    } catch (error) {
      // If index is missing, fall back to simple query and client-side sorting
      if (error.code === 9 || error.message.includes('index')) {
        console.warn(`⚠️  Using fallback query for location ${locationId} (index required for optimal performance)`);
        
        const snapshot = await db
          .collection(FIRESTORE_COLLECTION)
          .where("location.id", "==", locationId)
          .get();

        networkState.firestore.connected = true;

        if (snapshot.empty) {
          return null;
        }

        // Sort client-side and get latest
        const docs = snapshot.docs
          .map(doc => ({ ...doc.data(), docId: doc.id }))
          .sort((a, b) => {
            const aTime = a.timestamp?.toDate?.() || new Date(a.timestamp);
            const bTime = b.timestamp?.toDate?.() || new Date(b.timestamp);
            return bTime - aTime;
          });

        return docs.length > 0 ? mapStoredReading(docs[0], docs[0].docId) : null;
      }
      throw error;
    }
  }, `Firestore read latest for location ${locationId}`)
  .catch((error) => {
    networkState.firestore.connected = false;
    console.error(`❌ Failed to get latest reading for location ${locationId}: ${error.message}`);
    return null;
  });
}

async function getTotalReadingsByLocation(locationId) {
  if (isFirestorePrimary()) {
    return getTotalReadingsFromFirestoreByLocation(locationId);
  }
  
  // Count local readings by location ID
  return historicalReadings.filter(reading => 
    reading.location && reading.location.id === locationId
  ).length;
}

async function getTotalReadingsFromFirestoreByLocation(locationId) {
  if (!db) {
    return 0;
  }

  return retryWithBackoff(async () => {
    const snapshot = await db
      .collection(FIRESTORE_COLLECTION)
      .where("location.id", "==", locationId)
      .count()
      .get();
    
    networkState.firestore.connected = true;
    return snapshot.data().count;
  }, `Firestore count for location ${locationId}`, { maxAttempts: 2, baseDelay: 1000 })
  .catch((error) => {
    networkState.firestore.connected = false;
    console.warn(`⚠️  Could not count readings for location ${locationId}: ${error.message}`);
    return 0;
  });
}

async function getHistoryByLocation(locationId, limit) {
  if (isFirestorePrimary()) {
    return getHistoryFromFirestoreByLocation(locationId, limit);
  }
  
  // Filter and limit local readings by location ID
  const locationReadings = historicalReadings.filter(reading => 
    reading.location && reading.location.id === locationId
  );
  
  return locationReadings.slice(-limit);
}

async function getHistoryFromFirestoreByLocation(locationId, limit) {
  if (!db) {
    return [];
  }

  return retryWithBackoff(async () => {
    // Try compound query first (requires index)
    try {
      const snapshot = await db
        .collection(FIRESTORE_COLLECTION)
        .where("location.id", "==", locationId)
        .orderBy("timestamp", "desc")
        .limit(limit)
        .get();

      networkState.firestore.connected = true;

      return snapshot.docs
        .map((doc) => mapStoredReading(doc.data(), doc.id))
        .filter(Boolean)
        .reverse(); // Return in chronological order
    } catch (error) {
      // If index is missing, fall back to simple query and client-side sorting
      if (error.code === 9 || error.message.includes('index')) {
        console.warn(`⚠️  Using fallback query for location ${locationId} history (index required for optimal performance)`);
        
        const snapshot = await db
          .collection(FIRESTORE_COLLECTION)
          .where("location.id", "==", locationId)
          .get();

        networkState.firestore.connected = true;

        const docs = snapshot.docs
          .map((doc) => mapStoredReading(doc.data(), doc.id))
          .filter(Boolean)
          .sort((a, b) => {
            const aTime = new Date(a.lastUpdated);
            const bTime = new Date(b.lastUpdated);
            return bTime - aTime;
          })
          .slice(0, limit)
          .reverse(); // Return in chronological order

        return docs;
      }
      throw error;
    }
  }, `Firestore read history for location ${locationId}`)
  .catch((error) => {
    networkState.firestore.connected = false;
    console.error(`❌ Failed to get history for location ${locationId}: ${error.message}`);
    return [];
  });
}

function calculateOverallStatus(overview) {
  if (!overview || overview.length === 0) {
    return 'No Data';
  }
  
  const statuses = overview
    .map(item => item.status)
    .filter(status => status && status !== 'No Data');
  
  if (statuses.length === 0) {
    return 'No Data';
  }
  
  // Priority: Danger > Warning > Safe
  if (statuses.some(status => status.toLowerCase() === 'danger')) {
    return 'Danger';
  }
  
  if (statuses.some(status => status.toLowerCase() === 'warning')) {
    return 'Warning';
  }
  
  return 'Safe';
}

// Task 2.4: Alert Management Functions (REQ-026)
function createAlert(type, severity, message, sensorData, locationId = null) {
  const alert = {
    id: `alert_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`,
    type, // 'ch4', 'h2s', 'water_level', 'system'
    severity, // 'warning', 'danger', 'critical'
    message,
    status: ALERT_STATUS.ACTIVE,
    createdAt: new Date().toISOString(),
    updatedAt: new Date().toISOString(),
    acknowledgedAt: null,
    acknowledgedBy: null,
    resolvedAt: null,
    silencedUntil: null,
    locationId: locationId || (sensorData?.location?.id) || 'unknown',
    sensorData: sensorData ? {
      ch4: sensorData.ch4,
      h2s: sensorData.h2s,
      waterLevel: sensorData.waterLevel,
      timestamp: sensorData.lastUpdated
    } : null,
    escalationLevel: 0,
    notificationsSent: []
  };
  
  return alert;
}

async function storeAlertInFirestore(alert) {
  if (!db || !admin) {
    return null;
  }

  return retryWithBackoff(async () => {
    const timestamp = admin.firestore.Timestamp.fromDate(new Date(alert.createdAt));
    const updatedTimestamp = admin.firestore.Timestamp.fromDate(new Date(alert.updatedAt));

    const docRef = await db.collection(ALERTS_COLLECTION).add({
      ...alert,
      createdAt: timestamp,
      updatedAt: updatedTimestamp,
      acknowledgedAt: alert.acknowledgedAt ? admin.firestore.Timestamp.fromDate(new Date(alert.acknowledgedAt)) : null,
      resolvedAt: alert.resolvedAt ? admin.firestore.Timestamp.fromDate(new Date(alert.resolvedAt)) : null,
      silencedUntil: alert.silencedUntil ? admin.firestore.Timestamp.fromDate(new Date(alert.silencedUntil)) : null
    });

    return docRef.id;
  }, "Firestore alert storage")
  .catch((error) => {
    console.error(`❌ Failed to store alert in Firestore: ${error.message}`);
    return null;
  });
}

function addAlertToHistory(alert) {
  alertHistory = alertHistory.concat(alert).slice(-MAX_ALERT_HISTORY);
  
  // Store in Firestore if available
  if (isFirestorePrimary()) {
    storeAlertInFirestore(alert).catch(error => {
      console.error('Failed to store alert in Firestore:', error);
    });
  }
  
  return alert;
}

async function getAlertHistory(limit = 50, status = null, locationId = null) {
  if (isFirestorePrimary()) {
    return getAlertHistoryFromFirestore(limit, status, locationId);
  }
  
  let filteredAlerts = [...alertHistory];
  
  // Filter by status if specified
  if (status) {
    filteredAlerts = filteredAlerts.filter(alert => alert.status === status);
  }
  
  // Filter by location if specified
  if (locationId) {
    filteredAlerts = filteredAlerts.filter(alert => alert.locationId === locationId);
  }
  
  return filteredAlerts.slice(-limit).reverse(); // Most recent first
}

async function getAlertHistoryFromFirestore(limit = 50, status = null, locationId = null) {
  if (!db) {
    return [];
  }

  return retryWithBackoff(async () => {
    let query = db.collection(ALERTS_COLLECTION);
    
    // Apply filters
    if (status) {
      query = query.where("status", "==", status);
    }
    
    if (locationId) {
      query = query.where("locationId", "==", locationId);
    }
    
    // Order and limit
    query = query.orderBy("createdAt", "desc").limit(limit);
    
    const snapshot = await query.get();

    return snapshot.docs.map(doc => {
      const data = doc.data();
      return {
        ...data,
        id: doc.id,
        createdAt: data.createdAt?.toDate?.()?.toISOString() || data.createdAt,
        updatedAt: data.updatedAt?.toDate?.()?.toISOString() || data.updatedAt,
        acknowledgedAt: data.acknowledgedAt?.toDate?.()?.toISOString() || data.acknowledgedAt,
        resolvedAt: data.resolvedAt?.toDate?.()?.toISOString() || data.resolvedAt,
        silencedUntil: data.silencedUntil?.toDate?.()?.toISOString() || data.silencedUntil
      };
    });
  }, "Firestore alert history query")
  .catch((error) => {
    console.error(`❌ Failed to get alert history from Firestore: ${error.message}`);
    return [];
  });
}

async function updateAlertStatus(alertId, status, metadata = {}) {
  const now = new Date().toISOString();
  
  // Update local history
  const alertIndex = alertHistory.findIndex(alert => alert.id === alertId);
  if (alertIndex !== -1) {
    alertHistory[alertIndex] = {
      ...alertHistory[alertIndex],
      status,
      updatedAt: now,
      ...metadata
    };
  }
  
  // Update Firestore if available
  if (isFirestorePrimary()) {
    return updateAlertInFirestore(alertId, status, metadata);
  }
  
  return alertIndex !== -1;
}

async function updateAlertInFirestore(alertId, status, metadata = {}) {
  if (!db) {
    return false;
  }

  return retryWithBackoff(async () => {
    const updateData = {
      status,
      updatedAt: admin.firestore.Timestamp.now(),
      ...metadata
    };
    
    // Convert date strings to Firestore timestamps
    if (metadata.acknowledgedAt) {
      updateData.acknowledgedAt = admin.firestore.Timestamp.fromDate(new Date(metadata.acknowledgedAt));
    }
    if (metadata.resolvedAt) {
      updateData.resolvedAt = admin.firestore.Timestamp.fromDate(new Date(metadata.resolvedAt));
    }
    if (metadata.silencedUntil) {
      updateData.silencedUntil = admin.firestore.Timestamp.fromDate(new Date(metadata.silencedUntil));
    }
    
    await db.collection(ALERTS_COLLECTION).doc(alertId).update(updateData);
    return true;
  }, `Firestore alert update ${alertId}`)
  .catch((error) => {
    console.error(`❌ Failed to update alert ${alertId} in Firestore: ${error.message}`);
    return false;
  });
}

function checkAndCreateAlerts(reading) {
  const alerts = [];
  const locationId = reading.location?.id || 'unknown';
  
  // Check CH4 levels
  if (reading.metricStatus?.ch4 === 'danger') {
    const alertKey = `ch4_danger_${locationId}`;
    if (!activeAlerts.has(alertKey)) {
      const alert = createAlert(
        'ch4',
        'danger',
        `Critical CH4 level detected: ${reading.ch4} ppm (threshold: ${thresholds.ch4Danger} ppm)`,
        reading,
        locationId
      );
      alerts.push(alert);
      activeAlerts.set(alertKey, alert);
    }
  } else if (reading.metricStatus?.ch4 === 'warning') {
    const alertKey = `ch4_warning_${locationId}`;
    if (!activeAlerts.has(alertKey)) {
      const alert = createAlert(
        'ch4',
        'warning',
        `Elevated CH4 level detected: ${reading.ch4} ppm (threshold: ${thresholds.ch4Warning} ppm)`,
        reading,
        locationId
      );
      alerts.push(alert);
      activeAlerts.set(alertKey, alert);
    }
  } else {
    // Resolve CH4 alerts if levels are safe
    const dangerKey = `ch4_danger_${locationId}`;
    const warningKey = `ch4_warning_${locationId}`;
    if (activeAlerts.has(dangerKey)) {
      const alert = activeAlerts.get(dangerKey);
      updateAlertStatus(alert.id, ALERT_STATUS.RESOLVED, { resolvedAt: new Date().toISOString() });
      activeAlerts.delete(dangerKey);
    }
    if (activeAlerts.has(warningKey)) {
      const alert = activeAlerts.get(warningKey);
      updateAlertStatus(alert.id, ALERT_STATUS.RESOLVED, { resolvedAt: new Date().toISOString() });
      activeAlerts.delete(warningKey);
    }
  }
  
  // Check H2S levels
  if (reading.metricStatus?.h2s === 'danger') {
    const alertKey = `h2s_danger_${locationId}`;
    if (!activeAlerts.has(alertKey)) {
      const alert = createAlert(
        'h2s',
        'danger',
        `Critical H2S level detected: ${reading.h2s} ppm (threshold: ${thresholds.h2sDanger} ppm)`,
        reading,
        locationId
      );
      alerts.push(alert);
      activeAlerts.set(alertKey, alert);
    }
  } else if (reading.metricStatus?.h2s === 'warning') {
    const alertKey = `h2s_warning_${locationId}`;
    if (!activeAlerts.has(alertKey)) {
      const alert = createAlert(
        'h2s',
        'warning',
        `Elevated H2S level detected: ${reading.h2s} ppm (threshold: ${thresholds.h2sWarning} ppm)`,
        reading,
        locationId
      );
      alerts.push(alert);
      activeAlerts.set(alertKey, alert);
    }
  } else {
    // Resolve H2S alerts if levels are safe
    const dangerKey = `h2s_danger_${locationId}`;
    const warningKey = `h2s_warning_${locationId}`;
    if (activeAlerts.has(dangerKey)) {
      const alert = activeAlerts.get(dangerKey);
      updateAlertStatus(alert.id, ALERT_STATUS.RESOLVED, { resolvedAt: new Date().toISOString() });
      activeAlerts.delete(dangerKey);
    }
    if (activeAlerts.has(warningKey)) {
      const alert = activeAlerts.get(warningKey);
      updateAlertStatus(alert.id, ALERT_STATUS.RESOLVED, { resolvedAt: new Date().toISOString() });
      activeAlerts.delete(warningKey);
    }
  }
  
  // Check water level
  if (reading.metricStatus?.waterLevel === 'danger') {
    const alertKey = `water_danger_${locationId}`;
    if (!activeAlerts.has(alertKey)) {
      const alert = createAlert(
        'water_level',
        'danger',
        `Critical water level detected: ${reading.waterLevel} cm (threshold: ${thresholds.waterLevelDanger} cm)`,
        reading,
        locationId
      );
      alerts.push(alert);
      activeAlerts.set(alertKey, alert);
    }
  } else if (reading.metricStatus?.waterLevel === 'warning') {
    const alertKey = `water_warning_${locationId}`;
    if (!activeAlerts.has(alertKey)) {
      const alert = createAlert(
        'water_level',
        'warning',
        `Elevated water level detected: ${reading.waterLevel} cm (threshold: ${thresholds.waterLevelWarning} cm)`,
        reading,
        locationId
      );
      alerts.push(alert);
      activeAlerts.set(alertKey, alert);
    }
  } else {
    // Resolve water level alerts if levels are safe
    const dangerKey = `water_danger_${locationId}`;
    const warningKey = `water_warning_${locationId}`;
    if (activeAlerts.has(dangerKey)) {
      const alert = activeAlerts.get(dangerKey);
      updateAlertStatus(alert.id, ALERT_STATUS.RESOLVED, { resolvedAt: new Date().toISOString() });
      activeAlerts.delete(dangerKey);
    }
    if (activeAlerts.has(warningKey)) {
      const alert = activeAlerts.get(warningKey);
      updateAlertStatus(alert.id, ALERT_STATUS.RESOLVED, { resolvedAt: new Date().toISOString() });
      activeAlerts.delete(warningKey);
    }
  }
  
  // Store new alerts
  alerts.forEach(alert => {
    addAlertToHistory(alert);
    console.log(`🚨 New alert created: ${alert.type} ${alert.severity} at ${alert.locationId}`);
  });
  
  return alerts;
}

// Task 2.4.5: Alert Escalation Logic (REQ-026)
function getEscalationTimeThreshold(severity) {
  switch (severity) {
    case 'critical':
      return ESCALATION_CONFIG.CRITICAL_ESCALATION_TIME;
    case 'danger':
      return ESCALATION_CONFIG.DANGER_ESCALATION_TIME;
    case 'warning':
      return ESCALATION_CONFIG.WARNING_ESCALATION_TIME;
    default:
      return ESCALATION_CONFIG.WARNING_ESCALATION_TIME;
  }
}

async function checkAlertEscalation() {
  const now = new Date();
  const escalatedAlerts = [];
  
  // Check all active alerts for escalation
  for (const [key, alert] of activeAlerts.entries()) {
    // Skip if alert is acknowledged, resolved, or silenced
    if (alert.status !== ALERT_STATUS.ACTIVE) {
      continue;
    }
    
    // Skip if already at max escalation level
    if (alert.escalationLevel >= ESCALATION_CONFIG.MAX_ESCALATION_LEVEL) {
      continue;
    }
    
    // Calculate time since creation or last escalation
    const alertTime = new Date(alert.updatedAt || alert.createdAt);
    const minutesSinceAlert = (now - alertTime) / (1000 * 60);
    const escalationThreshold = getEscalationTimeThreshold(alert.severity);
    
    // Check if escalation is needed
    if (minutesSinceAlert >= escalationThreshold) {
      const newEscalationLevel = alert.escalationLevel + 1;
      const escalationAction = ESCALATION_CONFIG.ESCALATION_ACTIONS[newEscalationLevel];
      
      // Update alert with new escalation level
      alert.escalationLevel = newEscalationLevel;
      alert.updatedAt = now.toISOString();
      
      // Add escalation notification to history
      if (!alert.notificationsSent) {
        alert.notificationsSent = [];
      }
      
      alert.notificationsSent.push({
        level: newEscalationLevel,
        action: escalationAction,
        timestamp: now.toISOString(),
        reason: `Unacknowledged ${alert.severity} alert for ${minutesSinceAlert.toFixed(1)} minutes`
      });
      
      // Update in storage
      await updateAlertStatus(alert.id, ALERT_STATUS.ACTIVE, {
        escalationLevel: newEscalationLevel,
        notificationsSent: alert.notificationsSent
      });
      
      escalatedAlerts.push({
        alert,
        escalationLevel: newEscalationLevel,
        action: escalationAction,
        minutesSinceAlert: minutesSinceAlert.toFixed(1)
      });
      
      console.log(`🚨 Alert escalated: ${alert.id} to level ${newEscalationLevel} (${escalationAction}) after ${minutesSinceAlert.toFixed(1)} minutes`);
    }
  }
  
  // Process escalation actions
  for (const escalation of escalatedAlerts) {
    await processEscalationAction(escalation);
  }
  
  return escalatedAlerts;
}

async function processEscalationAction(escalation) {
  const { alert, escalationLevel, action } = escalation;
  
  try {
    switch (action) {
      case 'repeat_notification':
        // Send repeat notification via Telegram
        if (telegramBot && telegramBot.enabled) {
          const message = `🔄 ESCALATED ALERT (Level ${escalationLevel})\n` +
                         `${alert.message}\n` +
                         `Location: ${alert.locationId}\n` +
                         `Duration: ${escalation.minutesSinceAlert} minutes\n` +
                         `Please acknowledge this alert immediately.`;
          await telegramBot.sendAlert(message);
        }
        break;
        
      case 'supervisor_notification':
        // Send supervisor notification
        if (telegramBot && telegramBot.enabled) {
          const message = `⚠️ SUPERVISOR ALERT (Level ${escalationLevel})\n` +
                         `Unacknowledged ${alert.severity} alert requires attention:\n` +
                         `${alert.message}\n` +
                         `Location: ${alert.locationId}\n` +
                         `Duration: ${escalation.minutesSinceAlert} minutes\n` +
                         `This alert has been escalated to supervisor level.`;
          await telegramBot.sendAlert(message);
        }
        
        // Could also send email, SMS, or other notifications here
        console.log(`📧 Supervisor notification sent for alert ${alert.id}`);
        break;
        
      case 'emergency_notification':
        // Send emergency notification
        if (telegramBot && telegramBot.enabled) {
          const message = `🚨 EMERGENCY ALERT (Level ${escalationLevel})\n` +
                         `CRITICAL: Unacknowledged ${alert.severity} alert!\n` +
                         `${alert.message}\n` +
                         `Location: ${alert.locationId}\n` +
                         `Duration: ${escalation.minutesSinceAlert} minutes\n` +
                         `IMMEDIATE ACTION REQUIRED!`;
          await telegramBot.sendAlert(message);
        }
        
        // Could trigger additional emergency protocols here
        console.log(`🚨 Emergency notification sent for alert ${alert.id}`);
        break;
        
      default:
        console.warn(`Unknown escalation action: ${action}`);
    }
  } catch (error) {
    console.error(`❌ Failed to process escalation action ${action} for alert ${alert.id}:`, error.message);
  }
}

function startAlertEscalationMonitoring() {
  // Check for escalations every 2 minutes
  const intervalMinutes = 2;
  
  if (escalationInterval) {
    clearInterval(escalationInterval);
  }
  
  escalationInterval = setInterval(async () => {
    try {
      const escalatedAlerts = await checkAlertEscalation();
      if (escalatedAlerts.length > 0) {
        console.log(`🔄 Processed ${escalatedAlerts.length} alert escalations`);
      }
    } catch (error) {
      console.error('❌ Error in alert escalation monitoring:', error.message);
    }
  }, intervalMinutes * 60 * 1000);
  
  console.log(`🔄 Alert escalation monitoring started (checking every ${intervalMinutes} minutes)`);
}

function stopAlertEscalationMonitoring() {
  if (escalationInterval) {
    clearInterval(escalationInterval);
    escalationInterval = null;
    console.log('🔄 Alert escalation monitoring stopped');
  }
}

// Initialize Telegram Bot
const TELEGRAM_BOT_TOKEN = process.env.TELEGRAM_BOT_TOKEN;
const TELEGRAM_CHAT_IDS = (process.env.TELEGRAM_CHAT_IDS || "").split(",").filter(id => id.trim());
telegramBot = new TelegramAlertBot(TELEGRAM_BOT_TOKEN, TELEGRAM_CHAT_IDS);

// Initialize MQTT Client (REQ-007)
let mqttClient = null;
let mqttConnected = false;
let mqttStats = {
  messagesReceived: 0,
  lastMessageTime: null,
  reconnections: 0,
  connectionState: 'disconnected'
};

function initializeMQTT() {
  console.log(`📡 Connecting to MQTT broker: ${MQTT_CONFIG.broker}`);
  
  const options = {
    clientId: MQTT_CONFIG.clientId,
    keepalive: MQTT_CONFIG.keepalive,
    reconnectPeriod: MQTT_CONFIG.reconnectPeriod,
    connectTimeout: MQTT_CONFIG.connectTimeout,
    clean: true
  };
  
  if (MQTT_CONFIG.username) {
    options.username = MQTT_CONFIG.username;
    options.password = MQTT_CONFIG.password;
  }
  
  mqttClient = mqtt.connect(MQTT_CONFIG.broker, options);
  
  mqttClient.on('connect', () => {
    console.log('✅ MQTT broker connected successfully');
    mqttConnected = true;
    mqttStats.connectionState = 'connected';
    
    // Subscribe to all topics for this location
    const topicPattern = `${MQTT_CONFIG.topicPrefix}/${MQTT_CONFIG.locationId}/+`;
    mqttClient.subscribe(topicPattern, { qos: 1 }, (err) => {
      if (err) {
        console.error('❌ MQTT subscription failed:', err);
      } else {
        console.log(`📡 Subscribed to MQTT topic pattern: ${topicPattern}`);
      }
    });
  });
  
  mqttClient.on('message', (topic, message) => {
    try {
      const data = JSON.parse(message.toString());
      mqttStats.messagesReceived++;
      mqttStats.lastMessageTime = new Date().toISOString();
      
      console.log(`📡 MQTT message received on ${topic}`);
      
      // Route message based on topic
      const topicParts = topic.split('/');
      const messageType = topicParts[topicParts.length - 1]; // Last part of topic
      
      switch (messageType) {
        case 'sensors':
          handleSensorData(data);
          break;
        case 'alerts':
          handleAlertData(data);
          break;
        case 'diagnostics':
          handleDiagnosticData(data);
          break;
        case 'status':
          handleStatusData(data);
          break;
        default:
          console.log(`📡 Unknown MQTT message type: ${messageType}`);
      }
    } catch (error) {
      console.error('❌ Error processing MQTT message:', error);
    }
  });
  
  mqttClient.on('error', (error) => {
    console.error('❌ MQTT connection error:', error);
    mqttConnected = false;
    mqttStats.connectionState = 'error';
  });
  
  mqttClient.on('offline', () => {
    console.warn('⚠️  MQTT client offline');
    mqttConnected = false;
    mqttStats.connectionState = 'offline';
  });
  
  mqttClient.on('reconnect', () => {
    console.log('🔄 MQTT reconnecting...');
    mqttStats.reconnections++;
    mqttStats.connectionState = 'reconnecting';
  });
}

// MQTT Message Handlers
function handleSensorData(data) {
  // Process sensor data same as HTTP endpoint
  console.log('📊 Processing sensor data from MQTT');
  processSensorReading(data);
}

function handleAlertData(data) {
  console.log('🚨 Alert received via MQTT:', data);
  // Forward alert to Telegram if configured
  if (telegramBot && data.alert_active) {
    const alertMessage = `🚨 ALERT: ${data.alert_type} detected at ${data.device_id}\n` +
                        `CH4: ${data.ch4_ppm} ppm\n` +
                        `H2S: ${data.h2s_ppm} ppm\n` +
                        `Water Level: ${data.water_level_cm} cm`;
    telegramBot.sendAlert(alertMessage);
  }
}

function handleDiagnosticData(data) {
  console.log('🔧 Diagnostic data received via MQTT:', data);
  // Store diagnostic data or forward to monitoring system
}

function handleStatusData(data) {
  console.log('📊 Status update received via MQTT:', data);
  // Update device status tracking
}

// Shared sensor data processing function (REQ-007)
async function processSensorReading(rawData) {
  try {
    // Convert MQTT format to bridge format if needed
    let sensorData = rawData;
    if (rawData.ch4_ppm !== undefined) {
      // Convert from MQTT format to bridge format
      sensorData = {
        ch4: rawData.ch4_ppm || rawData.ch4,
        h2s: rawData.h2s_ppm || rawData.h2s,
        water: rawData.distance_cm || rawData.water,
        waterLevel: rawData.water_level_cm || rawData.waterLevel,
        battery: rawData.battery || 100,
        source: rawData.source || "mqtt",
        lastUpdated: rawData.ntpTime || rawData.lastUpdated || new Date().toISOString()
      };
    }
    
    const reading = normalizeSensorData(sensorData);

    if (!reading) {
      console.error('❌ Invalid sensor data received via MQTT');
      return;
    }

    let firebaseDocId = null;

    if (isFirestorePrimary()) {
      firebaseDocId = await storeReadingInFirestore(reading);
    }

    addReading(reading);

    console.log(
      `[MQTT INGESTED] ${reading.source} | CH4 ${reading.ch4} ppm | H2S ${reading.h2s} ppm | Water Level ${reading.waterLevel} cm | ${reading.status}`
    );

    // Broadcast real-time update to WebSocket clients (REQ-005)
    broadcastSensorUpdate(reading);

    // Task 2.4: Check and create alerts (REQ-026)
    checkAndCreateAlerts(reading);

    // Check thresholds and send Telegram alerts
    telegramBot.checkAndAlert(reading).catch((error) => {
      console.error("❌ Telegram alert error:", error.message);
    });

    return reading;
  } catch (error) {
    console.error(`❌ MQTT sensor processing error: ${error.message}`);
  }
}

const app = express();
app.use(express.json());

// API Key Authentication Middleware (REQ-003)
function authenticateApiKey(req, res, next) {
  // Skip authentication for health endpoint
  if (req.path === "/health") {
    return next();
  }

  // Skip authentication if API key is not configured or not required
  if (!API_KEY || !REQUIRE_API_KEY) {
    return next();
  }

  const providedKey = req.headers[API_KEY_HEADER.toLowerCase()] || req.headers[API_KEY_HEADER];

  if (!providedKey) {
    console.warn(`⚠️  API request without key: ${req.method} ${req.path} from ${req.ip}`);
    return res.status(401).json({
      error: "API key required",
      message: `Include ${API_KEY_HEADER} header with valid API key`,
      code: "MISSING_API_KEY"
    });
  }

  // Constant-time comparison to prevent timing attacks
  if (!constantTimeCompare(providedKey, API_KEY)) {
    console.warn(`⚠️  API request with invalid key: ${req.method} ${req.path} from ${req.ip}`);
    return res.status(403).json({
      error: "Invalid API key",
      message: "The provided API key is not valid",
      code: "INVALID_API_KEY"
    });
  }

  // Log successful authentication (without exposing the key)
  console.log(`✅ Authenticated request: ${req.method} ${req.path} from ${req.ip}`);
  next();
}

/**
 * Constant-time string comparison to prevent timing attacks
 * @param {string} a - First string
 * @param {string} b - Second string
 * @returns {boolean} - True if strings are equal
 */
function constantTimeCompare(a, b) {
  if (typeof a !== 'string' || typeof b !== 'string') {
    return false;
  }

  if (a.length !== b.length) {
    return false;
  }

  let result = 0;
  for (let i = 0; i < a.length; i++) {
    result |= a.charCodeAt(i) ^ b.charCodeAt(i);
  }

  return result === 0;
}

// Apply authentication middleware to all routes
app.use(authenticateApiKey);

app.use((req, res, next) => {
  res.header("Access-Control-Allow-Origin", "*");
  res.header("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  res.header("Access-Control-Allow-Headers", `Content-Type, ${API_KEY_HEADER}`);

  if (req.method === "OPTIONS") {
    return res.sendStatus(204);
  }

  next();
});

// Create HTTP server and WebSocket server (REQ-005)
const server = http.createServer(app);
wss = new WebSocket.Server({ 
  server,
  path: '/ws',
  // WebSocket authentication - check API key in query params or headers
  verifyClient: (info) => {
    // Skip authentication if API key is not required
    if (!API_KEY || !REQUIRE_API_KEY) {
      return true;
    }

    // Check for API key in query parameters
    const url = new URL(info.req.url, `http://${info.req.headers.host}`);
    const queryApiKey = url.searchParams.get('apiKey');
    
    // Check for API key in headers
    const headerApiKey = info.req.headers[API_KEY_HEADER.toLowerCase()] || info.req.headers[API_KEY_HEADER];
    
    const providedKey = queryApiKey || headerApiKey;
    
    if (!providedKey) {
      console.warn(`⚠️  WebSocket connection without API key from ${info.req.socket.remoteAddress}`);
      return false;
    }
    
    if (!constantTimeCompare(providedKey, API_KEY)) {
      console.warn(`⚠️  WebSocket connection with invalid API key from ${info.req.socket.remoteAddress}`);
      return false;
    }
    
    console.log(`✅ WebSocket connection authenticated from ${info.req.socket.remoteAddress}`);
    return true;
  }
});

// WebSocket connection handling (REQ-005)
wss.on('connection', (ws, req) => {
  console.log(`🔌 WebSocket client connected from ${req.socket.remoteAddress}`);
  
  // Send current status immediately upon connection
  getLatestReading().then(reading => {
    if (reading) {
      ws.send(JSON.stringify({
        type: 'sensor_update',
        data: reading,
        timestamp: new Date().toISOString()
      }));
    }
  }).catch(error => {
    console.error('Error sending initial data to WebSocket client:', error);
  });
  
  // Handle client messages (ping/pong for connection health)
  ws.on('message', (message) => {
    try {
      const data = JSON.parse(message);
      if (data.type === 'ping') {
        ws.send(JSON.stringify({ type: 'pong', timestamp: new Date().toISOString() }));
      }
    } catch (error) {
      console.warn('Invalid WebSocket message received:', error.message);
    }
  });
  
  ws.on('close', () => {
    console.log(`🔌 WebSocket client disconnected from ${req.socket.remoteAddress}`);
  });
  
  ws.on('error', (error) => {
    console.error('WebSocket error:', error.message);
  });
});

/**
 * Broadcast sensor data to all connected WebSocket clients (REQ-005)
 * @param {Object} reading - Sensor reading data
 */
function broadcastSensorUpdate(reading) {
  if (wss.clients.size === 0) {
    return; // No clients connected
  }
  
  const message = JSON.stringify({
    type: 'sensor_update',
    data: reading,
    timestamp: new Date().toISOString()
  });
  
  let broadcastCount = 0;
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      try {
        client.send(message);
        broadcastCount++;
      } catch (error) {
        console.error('Error broadcasting to WebSocket client:', error.message);
      }
    }
  });
  
  // Update network state (REQ-006)
  networkState.websocket.clientCount = wss.clients.size;
  networkState.websocket.lastBroadcast = Date.now();
  
  if (broadcastCount > 0) {
    console.log(`📡 Broadcasted sensor update to ${broadcastCount} WebSocket client(s)`);
  }
}

/**
 * Calculate next retry delay with exponential backoff and jitter (REQ-006)
 * @param {number} attempt - Current attempt number (0-based)
 * @param {number} baseDelay - Base delay in milliseconds
 * @param {number} maxDelay - Maximum delay in milliseconds
 * @param {number} jitterPercent - Jitter percentage (0.0 to 1.0)
 * @param {number} backoffMultiplier - Backoff multiplier
 * @returns {number} - Delay in milliseconds
 */
function calculateRetryDelay(attempt, baseDelay = RETRY_CONFIG.baseDelay, maxDelay = RETRY_CONFIG.maxDelay, jitterPercent = RETRY_CONFIG.jitterPercent, backoffMultiplier = RETRY_CONFIG.backoffMultiplier) {
  // Exponential backoff: baseDelay * (backoffMultiplier ^ attempt)
  const exponentialDelay = baseDelay * Math.pow(backoffMultiplier, attempt);
  
  // Cap at maximum delay
  const cappedDelay = Math.min(exponentialDelay, maxDelay);
  
  // Add jitter: ±jitterPercent of the delay
  const jitter = cappedDelay * jitterPercent * (Math.random() * 2 - 1);
  const finalDelay = Math.max(cappedDelay + jitter, 100); // Minimum 100ms
  
  return Math.round(finalDelay);
}

/**
 * Execute operation with exponential backoff retry (REQ-006)
 * @param {Function} operation - Async operation to retry
 * @param {string} operationName - Name for logging
 * @param {Object} options - Retry options
 * @returns {Promise} - Result of successful operation
 */
async function retryWithBackoff(operation, operationName, options = {}) {
  const maxAttempts = options.maxAttempts || RETRY_CONFIG.maxAttempts;
  const baseDelay = options.baseDelay || RETRY_CONFIG.baseDelay;
  
  let lastError;
  
  for (let attempt = 0; attempt < maxAttempts; attempt++) {
    try {
      const result = await operation();
      
      // Success - reset retry state if this was a retry
      if (attempt > 0) {
        console.log(`✅ ${operationName} succeeded after ${attempt + 1} attempts`);
      }
      
      return result;
    } catch (error) {
      lastError = error;
      
      // Don't retry on certain errors (authentication, validation, etc.)
      if (error.code === 'ENOTFOUND' || error.code === 'ECONNREFUSED' || 
          error.message.includes('authentication') || error.message.includes('permission')) {
        console.error(`❌ ${operationName} failed with non-retryable error: ${error.message}`);
        throw error;
      }
      
      // Calculate delay for next attempt
      const delay = calculateRetryDelay(attempt, baseDelay);
      
      if (attempt < maxAttempts - 1) {
        console.warn(`⚠️  ${operationName} failed (attempt ${attempt + 1}/${maxAttempts}): ${error.message}`);
        console.log(`🔄 Retrying ${operationName} in ${delay}ms...`);
        
        // Wait before retry
        await new Promise(resolve => setTimeout(resolve, delay));
      } else {
        console.error(`❌ ${operationName} failed after ${maxAttempts} attempts: ${error.message}`);
      }
    }
  }
  
  throw lastError;
}

initializeStorage();

function loadEnvFile(filePath) {
  if (!fs.existsSync(filePath)) {
    return;
  }

  const fileContents = fs.readFileSync(filePath, "utf8");

  for (const line of fileContents.split(/\r?\n/)) {
    const trimmedLine = line.trim();
    if (!trimmedLine || trimmedLine.startsWith("#")) {
      continue;
    }

    const separatorIndex = trimmedLine.indexOf("=");
    if (separatorIndex === -1) {
      continue;
    }

    const key = trimmedLine.slice(0, separatorIndex).trim();
    let value = trimmedLine.slice(separatorIndex + 1).trim();

    if (
      (value.startsWith('"') && value.endsWith('"')) ||
      (value.startsWith("'") && value.endsWith("'"))
    ) {
      value = value.slice(1, -1);
    }

    if (process.env[key] === undefined) {
      process.env[key] = value;
    }
  }
}

function initializeStorage() {
  const firestoreReady = initializeFirebase();
  const useFirestoreAsPrimary =
    firestoreReady &&
    (STORAGE_BACKEND === "auto" || STORAGE_BACKEND === "firestore");

  if (useFirestoreAsPrimary) {
    localCacheEnabled = process.env.LOCAL_CACHE_ENABLED === "true";
    storageMode = localCacheEnabled ? "firestore+local-cache" : "firestore";
  } else {
    if (STORAGE_BACKEND === "firestore") {
      console.warn(
        "⚠️  Firestore was requested but Firebase credentials are missing or invalid. Falling back to local-cache mode."
      );
    }

    localCacheEnabled = true;
    storageMode = "local-cache";
  }

  historicalReadings = localCacheEnabled ? loadHistoricalReadings() : [];
}

function getFirebaseCredentialsFromEnv() {
  const projectId = process.env.FIREBASE_PROJECT_ID;
  const clientEmail = process.env.FIREBASE_CLIENT_EMAIL;
  const privateKey = process.env.FIREBASE_PRIVATE_KEY;

  if (!projectId || !clientEmail || !privateKey) {
    return null;
  }

  return {
    project_id: projectId,
    client_email: clientEmail,
    private_key: privateKey.replace(/\\n/g, "\n"),
  };
}

function initializeFirebase() {
  const envCredentials = getFirebaseCredentialsFromEnv();

  if (!envCredentials && !fs.existsSync(FIREBASE_KEY_PATH)) {
    console.warn(
      "⚠️  Firebase credentials not found. Bridge will use local-cache mode until Firestore credentials are configured."
    );
    networkState.firestore.connected = false;
    return false;
  }

  // Use retry logic for Firebase initialization (REQ-006)
  return retryWithBackoff(async () => {
    admin = require("firebase-admin");
    const serviceAccount = envCredentials || require(FIREBASE_KEY_PATH);

    if (!admin.apps.length) {
      admin.initializeApp({
        credential: admin.credential.cert(serviceAccount),
        projectId: serviceAccount.project_id || "manhole-monitoring-e5d2b",
      });
    }

    db = admin.firestore();
    
    // Test connection with a simple operation
    await db.collection(FIRESTORE_COLLECTION).limit(1).get();
    
    networkState.firestore.connected = true;
    networkState.firestore.retryCount = 0;
    networkState.firestore.nextRetryDelay = RETRY_CONFIG.baseDelay;
    
    console.log(`✅ Firebase connected. Firestore collection: ${FIRESTORE_COLLECTION}`);
    return true;
  }, "Firebase initialization", { maxAttempts: 3, baseDelay: 2000 })
  .catch((error) => {
    admin = null;
    db = null;
    networkState.firestore.connected = false;
    console.warn(`⚠️  Firebase initialization failed after retries: ${error.message}`);
    return false;
  });
}

function loadHistoricalReadings() {
  if (!fs.existsSync(READINGS_FILE_PATH)) {
    return [];
  }

  try {
    const fileContents = fs.readFileSync(READINGS_FILE_PATH, "utf8");
    const parsedReadings = JSON.parse(fileContents);

    if (!Array.isArray(parsedReadings)) {
      return [];
    }

    return parsedReadings.filter(isValidReading).slice(-MAX_HISTORY);
  } catch (error) {
    console.warn(
      `⚠️  Could not read cached sensor history (${error.message}). Starting fresh.`
    );
    return [];
  }
}

function persistHistoricalReadings() {
  if (!localCacheEnabled) {
    return;
  }

  try {
    fs.writeFileSync(
      READINGS_FILE_PATH,
      JSON.stringify(historicalReadings.slice(-MAX_HISTORY), null, 2)
    );
  } catch (error) {
    console.warn(`⚠️  Could not persist sensor history (${error.message}).`);
  }
}

function isValidReading(reading) {
  return (
    reading &&
    Number.isFinite(Number(reading.ch4)) &&
    Number.isFinite(Number(reading.h2s)) &&
    Number.isFinite(Number(reading.waterLevel)) &&
    typeof reading.lastUpdated === "string"
  );
}

function parseNumber(value, fallback = null) {
  const parsed = Number(value);
  return Number.isFinite(parsed) ? parsed : fallback;
}

function roundToTwo(value) {
  return Math.round((value + Number.EPSILON) * 100) / 100;
}

function clamp(value, min, max) {
  return Math.min(Math.max(value, min), max);
}

function getMetricStatus(value, warningThreshold, dangerThreshold) {
  if (value >= dangerThreshold) {
    return "danger";
  }

  if (value >= warningThreshold) {
    return "warning";
  }

  return "safe";
}

/**
 * Apply hysteresis logic to prevent alert spam near thresholds (REQ-004)
 * @param {number} value - Current sensor value
 * @param {number} warningThreshold - Warning threshold
 * @param {number} dangerThreshold - Danger threshold  
 * @param {number} hysteresisOffset - Hysteresis offset
 * @param {string} previousStatus - Previous status for this metric
 * @returns {string} - New status with hysteresis applied
 */
function getMetricStatusWithHysteresis(value, warningThreshold, dangerThreshold, hysteresisOffset, previousStatus) {
  // Calculate thresholds with hysteresis
  let adjustedWarningThreshold = warningThreshold;
  let adjustedDangerThreshold = dangerThreshold;
  
  // Apply hysteresis based on previous status
  if (previousStatus === "warning") {
    // If previously warning, need to go below warning-hysteresis to become safe
    adjustedWarningThreshold = warningThreshold - hysteresisOffset;
  } else if (previousStatus === "danger") {
    // If previously danger, need to go below danger-hysteresis to become warning
    adjustedDangerThreshold = dangerThreshold - hysteresisOffset;
    // And below warning-hysteresis to become safe
    adjustedWarningThreshold = warningThreshold - hysteresisOffset;
  }
  
  // Determine new status with adjusted thresholds
  if (value >= adjustedDangerThreshold) {
    return "danger";
  }
  
  if (value >= adjustedWarningThreshold) {
    return "warning";
  }
  
  return "safe";
}

function getOverallStatus(metricStatuses) {
  if (metricStatuses.includes("danger")) {
    return "Danger";
  }

  if (metricStatuses.includes("warning")) {
    return "Warning";
  }

  return "Safe";
}

function normalizeLocation(location) {
  if (!location) {
    return DEFAULT_LOCATION;
  }

  return {
    id: location.id || DEFAULT_LOCATION.id,
    lat: parseNumber(location.lat, DEFAULT_LOCATION.lat),
    lng: parseNumber(location.lng, DEFAULT_LOCATION.lng),
  };
}

function normalizeSensorData(rawData = {}) {
  const ch4 = parseNumber(rawData.ch4);
  const h2s = parseNumber(rawData.h2s);
  const requestedWaterLevel = parseNumber(rawData.waterLevel);
  const requestedWaterDistance = parseNumber(
    rawData.waterDistance,
    parseNumber(rawData.water)
  );

  if (
    ch4 === null ||
    h2s === null ||
    (requestedWaterLevel === null && requestedWaterDistance === null)
  ) {
    return null;
  }

  const waterLevel =
    requestedWaterLevel !== null
      ? clamp(requestedWaterLevel, 0, MANHOLE_DEPTH_CM)
      : clamp(
          MANHOLE_DEPTH_CM - Math.max(requestedWaterDistance, 0),
          0,
          MANHOLE_DEPTH_CM
        );
  const normalizedWaterDistance =
    requestedWaterDistance !== null
      ? Math.max(requestedWaterDistance, 0)
      : clamp(MANHOLE_DEPTH_CM - waterLevel, 0, MANHOLE_DEPTH_CM);

  const ch4Status = getMetricStatusWithHysteresis(
    ch4,
    thresholds.ch4Warning,
    thresholds.ch4Danger,
    thresholds.hysteresis.ch4,
    previousMetricStatus.ch4
  );
  const h2sStatus = getMetricStatusWithHysteresis(
    h2s,
    thresholds.h2sWarning,
    thresholds.h2sDanger,
    thresholds.hysteresis.h2s,
    previousMetricStatus.h2s
  );
  const waterStatus = getMetricStatusWithHysteresis(
    waterLevel,
    thresholds.waterLevelWarning,
    thresholds.waterLevelDanger,
    thresholds.hysteresis.waterLevel,
    previousMetricStatus.waterLevel
  );

  // Update previous status for next iteration (REQ-004)
  previousMetricStatus.ch4 = ch4Status;
  previousMetricStatus.h2s = h2sStatus;
  previousMetricStatus.waterLevel = waterStatus;

  const status = getOverallStatus([ch4Status, h2sStatus, waterStatus]);
  const parsedTimestamp = new Date(rawData.lastUpdated || Date.now());
  const lastUpdated = Number.isNaN(parsedTimestamp.getTime())
    ? new Date().toISOString()
    : parsedTimestamp.toISOString();

  return {
    ch4: roundToTwo(ch4),
    h2s: roundToTwo(h2s),
    waterDistance: roundToTwo(normalizedWaterDistance),
    waterLevel: roundToTwo(waterLevel),
    alert: status === "Danger",
    status,
    battery: clamp(Math.round(parseNumber(rawData.battery, 100)), 0, 100),
    sensorStatus: rawData.sensorStatus || "Working",
    source: rawData.source || "wokwi",
    location: normalizeLocation(rawData.location),
    thresholds,
    lastUpdated,
    metricStatus: {
      ch4: ch4Status,
      h2s: h2sStatus,
      waterLevel: waterStatus,
    },
  };
}

function addReading(reading) {
  if (!localCacheEnabled) {
    return reading;
  }

  historicalReadings = historicalReadings.concat(reading).slice(-MAX_HISTORY);
  persistHistoricalReadings();
  return reading;
}

function isFirestorePrimary() {
  return Boolean(db) && storageMode.startsWith("firestore");
}

function toIsoString(value) {
  if (!value) {
    return new Date().toISOString();
  }

  if (typeof value === "string") {
    return value;
  }

  if (typeof value.toDate === "function") {
    return value.toDate().toISOString();
  }

  const parsedDate = new Date(value);
  return Number.isNaN(parsedDate.getTime())
    ? new Date().toISOString()
    : parsedDate.toISOString();
}

function mapStoredReading(data = {}, id = null) {
  const normalized = normalizeSensorData({
    ...data,
    lastUpdated: data.lastUpdated || toIsoString(data.timestamp),
  });

  if (!normalized) {
    return null;
  }

  return {
    ...normalized,
    id: id || data.id || undefined,
    alert:
      typeof data.alert === "boolean" ? data.alert : normalized.alert,
    status: data.status || normalized.status,
    battery: clamp(
      Math.round(parseNumber(data.battery, normalized.battery)),
      0,
      100
    ),
    sensorStatus: data.sensorStatus || normalized.sensorStatus,
    source: data.source || normalized.source,
    location: normalizeLocation(data.location || normalized.location),
    thresholds: data.thresholds || normalized.thresholds,
    metricStatus: data.metricStatus || normalized.metricStatus,
    lastUpdated: data.lastUpdated || normalized.lastUpdated,
  };
}

async function storeReadingInFirestore(reading) {
  if (!db || !admin) {
    return null;
  }

  return retryWithBackoff(async () => {
    const timestamp = admin.firestore.Timestamp.fromDate(
      new Date(reading.lastUpdated)
    );

    const docRef = await db.collection(FIRESTORE_COLLECTION).add({
      ...reading,
      timestamp,
    });

    networkState.firestore.connected = true;
    return docRef.id;
  }, "Firestore write operation")
  .catch((error) => {
    networkState.firestore.connected = false;
    console.error(`❌ Failed to store reading in Firestore: ${error.message}`);
    return null;
  });
}

async function getLatestReadingFromFirestore() {
  if (!db) {
    return null;
  }

  return retryWithBackoff(async () => {
    const snapshot = await db
      .collection(FIRESTORE_COLLECTION)
      .orderBy("timestamp", "desc")
      .limit(1)
      .get();

    networkState.firestore.connected = true;

    if (snapshot.empty) {
      return null;
    }

    return mapStoredReading(snapshot.docs[0].data(), snapshot.docs[0].id);
  }, "Firestore read latest")
  .catch((error) => {
    networkState.firestore.connected = false;
    console.error(`❌ Failed to get latest reading from Firestore: ${error.message}`);
    return null;
  });
}

async function getHistoryFromFirestore(limit) {
  if (!db) {
    return [];
  }

  return retryWithBackoff(async () => {
    const snapshot = await db
      .collection(FIRESTORE_COLLECTION)
      .orderBy("timestamp", "desc")
      .limit(limit)
      .get();

    networkState.firestore.connected = true;

    return snapshot.docs
      .map((doc) => mapStoredReading(doc.data(), doc.id))
      .filter(Boolean)
      .reverse();
  }, "Firestore read history")
  .catch((error) => {
    networkState.firestore.connected = false;
    console.error(`❌ Failed to get history from Firestore: ${error.message}`);
    return [];
  });
}

async function getFilteredHistoryFromFirestore(limit, startDate, endDate) {
  if (!db) {
    return [];
  }

  return retryWithBackoff(async () => {
    let query = db.collection(FIRESTORE_COLLECTION);
    
    // Apply date filters if provided
    if (startDate) {
      const start = admin.firestore.Timestamp.fromDate(new Date(startDate));
      query = query.where("timestamp", ">=", start);
    }
    
    if (endDate) {
      const end = admin.firestore.Timestamp.fromDate(new Date(endDate));
      query = query.where("timestamp", "<=", end);
    }
    
    // Apply limit and ordering
    query = query.orderBy("timestamp", "desc");
    if (limit && Number.isFinite(limit)) {
      query = query.limit(Math.min(limit, 10000)); // Cap at 10k records for performance
    }
    
    const snapshot = await query.get();
    networkState.firestore.connected = true;

    return snapshot.docs
      .map((doc) => mapStoredReading(doc.data(), doc.id))
      .filter(Boolean)
      .reverse(); // Return in chronological order
  }, "Firestore filtered export query")
  .catch((error) => {
    networkState.firestore.connected = false;
    console.error(`❌ Failed to get filtered history from Firestore: ${error.message}`);
    return [];
  });
}

async function getFilteredHistoryFromLocal(limit, startDate, endDate) {
  let filteredReadings = [...historicalReadings];
  
  // Apply date filters if provided
  if (startDate || endDate) {
    filteredReadings = filteredReadings.filter(reading => {
      const readingDate = new Date(reading.lastUpdated);
      
      if (startDate && readingDate < new Date(startDate)) {
        return false;
      }
      
      if (endDate && readingDate > new Date(endDate)) {
        return false;
      }
      
      return true;
    });
  }
  
  // Apply limit
  if (limit && Number.isFinite(limit)) {
    filteredReadings = filteredReadings.slice(-limit);
  }
  
  return filteredReadings;
}

function generateCSV(readings) {
  if (readings.length === 0) {
    return '';
  }
  
  // CSV headers
  const headers = [
    'Timestamp',
    'CH4 (ppm)',
    'H2S (ppm)', 
    'Water Level (cm)',
    'Water Distance (cm)',
    'Overall Status',
    'CH4 Status',
    'H2S Status',
    'Water Status',
    'Battery (%)',
    'Sensor Status',
    'Alert',
    'Source',
    'Location ID',
    'Latitude',
    'Longitude'
  ];
  
  // Generate CSV rows
  const rows = readings.map(reading => [
    reading.lastUpdated,
    reading.ch4,
    reading.h2s,
    reading.waterLevel,
    reading.waterDistance,
    reading.status,
    reading.metricStatus?.ch4 || 'unknown',
    reading.metricStatus?.h2s || 'unknown', 
    reading.metricStatus?.waterLevel || 'unknown',
    reading.battery,
    reading.sensorStatus,
    reading.alert ? 'true' : 'false',
    reading.source,
    reading.location?.id || '',
    reading.location?.lat || '',
    reading.location?.lng || ''
  ]);
  
  // Combine headers and rows
  const csvLines = [headers, ...rows];
  
  // Convert to CSV format with proper escaping
  return csvLines.map(row => 
    row.map(field => {
      // Convert to string and handle special characters
      const str = String(field || '');
      // Escape quotes and wrap in quotes if contains comma, quote, or newline
      if (str.includes(',') || str.includes('"') || str.includes('\n')) {
        return `"${str.replace(/"/g, '""')}"`;
      }
      return str;
    }).join(',')
  ).join('\n');
}

async function getTotalReadingsFromFirestore() {
  if (!db) {
    return historicalReadings.length;
  }

  return retryWithBackoff(async () => {
    const snapshot = await db.collection(FIRESTORE_COLLECTION).count().get();
    networkState.firestore.connected = true;
    return snapshot.data().count;
  }, "Firestore count operation", { maxAttempts: 2, baseDelay: 1000 })
  .catch((error) => {
    networkState.firestore.connected = false;
    console.warn(`⚠️  Could not count Firestore readings: ${error.message}`);
    return null;
  });
}

async function getLatestReading() {
  if (isFirestorePrimary()) {
    return getLatestReadingFromFirestore();
  }

  return historicalReadings.length
    ? historicalReadings[historicalReadings.length - 1]
    : null;
}

async function getHistory(limit) {
  if (isFirestorePrimary()) {
    return getHistoryFromFirestore(limit);
  }

  return historicalReadings.slice(-limit);
}

async function getTotalReadings() {
  if (isFirestorePrimary()) {
    return getTotalReadingsFromFirestore();
  }

  return historicalReadings.length;
}

app.get("/health", async (req, res) => {
  try {
    const [latestReading, totalReadings] = await Promise.all([
      getLatestReading(),
      getTotalReadings(),
    ]);

    res.json({
      status: "ok",
      storageMode,
      firebaseEnabled: Boolean(db),
      firestoreCollection: db ? FIRESTORE_COLLECTION : null,
      totalReadings,
      latestReadingAt: latestReading ? latestReading.lastUpdated : null,
      manholeDepthCm: MANHOLE_DEPTH_CM,
      thresholds,
      apiBase: `http://localhost:${PORT}`,
      // Security status (REQ-003)
      security: {
        apiKeyRequired: REQUIRE_API_KEY,
        apiKeyConfigured: Boolean(API_KEY),
        apiKeyHeader: API_KEY_HEADER
      },
      // Network status (REQ-006)
      network: {
        firestore: {
          connected: networkState.firestore.connected,
          retryCount: networkState.firestore.retryCount,
          lastAttempt: networkState.firestore.lastAttempt
        },
        websocket: {
          clientCount: wss ? wss.clients.size : 0,
          lastBroadcast: networkState.websocket.lastBroadcast
        },
        // MQTT status (REQ-007)
        mqtt: {
          connected: mqttConnected,
          broker: MQTT_CONFIG.broker,
          messagesReceived: mqttStats.messagesReceived,
          lastMessageTime: mqttStats.lastMessageTime,
          reconnections: mqttStats.reconnections,
          connectionState: mqttStats.connectionState
        }
      },
      // Retry configuration (REQ-006)
      retryConfig: {
        maxAttempts: RETRY_CONFIG.maxAttempts,
        baseDelay: RETRY_CONFIG.baseDelay,
        maxDelay: RETRY_CONFIG.maxDelay,
        jitterPercent: RETRY_CONFIG.jitterPercent
      }
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.get("/api/sensor/latest", async (req, res) => {
  try {
    res.json({
      storageMode,
      totalReadings: await getTotalReadings(),
      reading: await getLatestReading(),
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.get("/api/sensor/history", async (req, res) => {
  try {
    const requestedLimit = Number.parseInt(req.query.limit, 10);
    const limit = clamp(
      Number.isFinite(requestedLimit) ? requestedLimit : 20,
      1,
      MAX_HISTORY
    );

    res.json({
      storageMode,
      totalReadings: await getTotalReadings(),
      readings: await getHistory(limit),
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Task 2.2: Historical Data Export (REQ-024)
app.get("/api/sensor/export", async (req, res) => {
  try {
    const format = (req.query.format || 'csv').toLowerCase();
    const requestedLimit = Number.parseInt(req.query.limit, 10);
    const startDate = req.query.startDate;
    const endDate = req.query.endDate;
    
    // Validate format
    if (!['csv', 'json'].includes(format)) {
      return res.status(400).json({
        error: "Invalid format. Supported formats: csv, json"
      });
    }
    
    // Get readings with optional date filtering
    let readings;
    if (isFirestorePrimary()) {
      readings = await getFilteredHistoryFromFirestore(requestedLimit, startDate, endDate);
    } else {
      readings = await getFilteredHistoryFromLocal(requestedLimit, startDate, endDate);
    }
    
    if (readings.length === 0) {
      return res.status(404).json({
        error: "No sensor data found for the specified criteria"
      });
    }
    
    // Generate filename with timestamp
    const timestamp = new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);
    const filename = `sensor-data-${timestamp}.${format}`;
    
    // Set appropriate headers for file download
    res.setHeader('Content-Disposition', `attachment; filename="${filename}"`);
    res.setHeader('Cache-Control', 'no-cache');
    
    if (format === 'csv') {
      res.setHeader('Content-Type', 'text/csv; charset=utf-8');
      
      // Generate CSV content
      const csvContent = generateCSV(readings);
      res.send(csvContent);
    } else {
      res.setHeader('Content-Type', 'application/json; charset=utf-8');
      
      // Generate JSON content with metadata
      const jsonContent = {
        metadata: {
          exportDate: new Date().toISOString(),
          totalRecords: readings.length,
          dateRange: {
            start: readings.length > 0 ? readings[0].lastUpdated : null,
            end: readings.length > 0 ? readings[readings.length - 1].lastUpdated : null
          },
          format: 'json',
          storageMode
        },
        readings: readings
      };
      
      res.json(jsonContent);
    }
    
    console.log(`📊 Data export completed: ${readings.length} records in ${format.toUpperCase()} format`);
    
  } catch (error) {
    console.error(`❌ Export error: ${error.message}`);
    res.status(500).json({ error: error.message });
  }
});

// Task 2.3: Multi-Location Support (REQ-025)
// Get all available locations
app.get("/api/locations", async (req, res) => {
  try {
    res.json({
      locations: getAllLocations(),
      defaultLocationId: LOCATIONS_CONFIG.defaultLocationId,
      totalLocations: LOCATIONS_CONFIG.locations.length
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Get latest reading for specific location
app.get("/api/locations/:locationId/sensor/latest", async (req, res) => {
  try {
    const locationId = req.params.locationId;
    const location = getLocationById(locationId);
    
    if (!location) {
      return res.status(404).json({
        error: `Location '${locationId}' not found`
      });
    }
    
    // Get readings filtered by location
    const reading = await getLatestReadingByLocation(locationId);
    
    res.json({
      storageMode,
      location,
      totalReadings: await getTotalReadingsByLocation(locationId),
      reading
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Get history for specific location
app.get("/api/locations/:locationId/sensor/history", async (req, res) => {
  try {
    const locationId = req.params.locationId;
    const location = getLocationById(locationId);
    
    if (!location) {
      return res.status(404).json({
        error: `Location '${locationId}' not found`
      });
    }
    
    const requestedLimit = Number.parseInt(req.query.limit, 10);
    const limit = clamp(
      Number.isFinite(requestedLimit) ? requestedLimit : 20,
      1,
      MAX_HISTORY
    );

    const readings = await getHistoryByLocation(locationId, limit);

    res.json({
      storageMode,
      location,
      totalReadings: await getTotalReadingsByLocation(locationId),
      readings
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

// Get overview of all locations with latest readings
app.get("/api/locations/overview", async (req, res) => {
  try {
    const locations = getAllLocations();
    const overview = [];
    
    for (const location of locations) {
      const latestReading = await getLatestReadingByLocation(location.id);
      const totalReadings = await getTotalReadingsByLocation(location.id);
      
      overview.push({
        location,
        latestReading,
        totalReadings,
        status: latestReading ? latestReading.status : 'No Data',
        lastUpdated: latestReading ? latestReading.lastUpdated : null
      });
    }
    
    res.json({
      storageMode,
      locations: overview,
      totalLocations: locations.length,
      overallStatus: calculateOverallStatus(overview)
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post("/api/sensor", async (req, res) => {
  try {
    const reading = normalizeSensorData(req.body);

    if (!reading) {
      return res.status(400).json({
        error: "Expected numeric ch4, h2s, and water fields in the sensor payload.",
      });
    }

    let firebaseDocId = null;

    if (isFirestorePrimary()) {
      firebaseDocId = await storeReadingInFirestore(reading);
    }

    addReading(reading);

    console.log(
      `[INGESTED] ${reading.source} | CH4 ${reading.ch4} ppm | H2S ${reading.h2s} ppm | Water Level ${reading.waterLevel} cm | ${reading.status}`
    );

    // Broadcast real-time update to WebSocket clients (REQ-005)
    broadcastSensorUpdate(reading);

    // Task 2.4: Check and create alerts (REQ-026)
    checkAndCreateAlerts(reading);

    // Check thresholds and send Telegram alerts
    telegramBot.checkAndAlert(reading).catch((error) => {
      console.error("❌ Telegram alert error:", error.message);
    });

    res.json({
      status: "ok",
      storageMode,
      firebaseDocId,
      reading,
    });
  } catch (error) {
    console.error(`❌ Bridge error: ${error.message}`);
    res.status(500).json({ error: error.message });
  }
});

// Task 2.4: Alert Management API Endpoints (REQ-026)

// Get alert history
app.get("/api/alerts", async (req, res) => {
  try {
    const limit = Math.min(parseInt(req.query.limit) || 50, 200);
    const status = req.query.status || null;
    const locationId = req.query.locationId || null;
    
    const alerts = await getAlertHistory(limit, status, locationId);
    
    res.json({
      alerts,
      totalAlerts: alerts.length,
      activeAlerts: Array.from(activeAlerts.values()).length,
      filters: {
        status,
        locationId,
        limit
      }
    });
  } catch (error) {
    console.error(`❌ Alert history error: ${error.message}`);
    res.status(500).json({ error: error.message });
  }
});

// Get active alerts
app.get("/api/alerts/active", async (req, res) => {
  try {
    const locationId = req.query.locationId || null;
    
    let alerts = Array.from(activeAlerts.values());
    
    // Filter by location if specified
    if (locationId) {
      alerts = alerts.filter(alert => alert.locationId === locationId);
    }
    
    res.json({
      alerts,
      totalActive: alerts.length,
      locationId
    });
  } catch (error) {
    console.error(`❌ Active alerts error: ${error.message}`);
    res.status(500).json({ error: error.message });
  }
});

// Acknowledge an alert
app.post("/api/alerts/:alertId/acknowledge", async (req, res) => {
  try {
    const { alertId } = req.params;
    const { acknowledgedBy = 'dashboard_user' } = req.body;
    
    const success = await updateAlertStatus(alertId, ALERT_STATUS.ACKNOWLEDGED, {
      acknowledgedAt: new Date().toISOString(),
      acknowledgedBy
    });
    
    if (success) {
      console.log(`✅ Alert ${alertId} acknowledged by ${acknowledgedBy}`);
      res.json({
        status: 'ok',
        message: 'Alert acknowledged successfully',
        alertId,
        acknowledgedBy
      });
    } else {
      res.status(404).json({
        error: 'Alert not found',
        alertId
      });
    }
  } catch (error) {
    console.error(`❌ Alert acknowledge error: ${error.message}`);
    res.status(500).json({ error: error.message });
  }
});

// Silence an alert for a specified duration
app.post("/api/alerts/:alertId/silence", async (req, res) => {
  try {
    const { alertId } = req.params;
    const { durationMinutes = 60, silencedBy = 'dashboard_user' } = req.body;
    
    const silencedUntil = new Date(Date.now() + (durationMinutes * 60 * 1000)).toISOString();
    
    const success = await updateAlertStatus(alertId, ALERT_STATUS.SILENCED, {
      silencedUntil,
      silencedBy
    });
    
    if (success) {
      console.log(`🔇 Alert ${alertId} silenced for ${durationMinutes} minutes by ${silencedBy}`);
      res.json({
        status: 'ok',
        message: `Alert silenced for ${durationMinutes} minutes`,
        alertId,
        silencedUntil,
        silencedBy
      });
    } else {
      res.status(404).json({
        error: 'Alert not found',
        alertId
      });
    }
  } catch (error) {
    console.error(`❌ Alert silence error: ${error.message}`);
    res.status(500).json({ error: error.message });
  }
});

// Resolve an alert manually
app.post("/api/alerts/:alertId/resolve", async (req, res) => {
  try {
    const { alertId } = req.params;
    const { resolvedBy = 'dashboard_user', reason = 'Manual resolution' } = req.body;
    
    const success = await updateAlertStatus(alertId, ALERT_STATUS.RESOLVED, {
      resolvedAt: new Date().toISOString(),
      resolvedBy,
      resolutionReason: reason
    });
    
    if (success) {
      // Remove from active alerts if it exists
      for (const [key, alert] of activeAlerts.entries()) {
        if (alert.id === alertId) {
          activeAlerts.delete(key);
          break;
        }
      }
      
      console.log(`✅ Alert ${alertId} resolved by ${resolvedBy}: ${reason}`);
      res.json({
        status: 'ok',
        message: 'Alert resolved successfully',
        alertId,
        resolvedBy,
        reason
      });
    } else {
      res.status(404).json({
        error: 'Alert not found',
        alertId
      });
    }
  } catch (error) {
    console.error(`❌ Alert resolve error: ${error.message}`);
    res.status(500).json({ error: error.message });
  }
});

// Get alert statistics
app.get("/api/alerts/stats", async (req, res) => {
  try {
    const locationId = req.query.locationId || null;
    
    // Get recent alerts (last 24 hours)
    const last24Hours = new Date(Date.now() - 24 * 60 * 60 * 1000).toISOString();
    const recentAlerts = await getAlertHistory(1000, null, locationId);
    const recent24h = recentAlerts.filter(alert => alert.createdAt >= last24Hours);
    
    // Calculate statistics
    const stats = {
      total: recentAlerts.length,
      last24Hours: recent24h.length,
      active: Array.from(activeAlerts.values()).filter(alert => 
        !locationId || alert.locationId === locationId
      ).length,
      byStatus: {
        active: 0,
        acknowledged: 0,
        resolved: 0,
        silenced: 0
      },
      bySeverity: {
        warning: 0,
        danger: 0,
        critical: 0
      },
      byType: {
        ch4: 0,
        h2s: 0,
        water_level: 0,
        system: 0
      }
    };
    
    // Count by categories
    recent24h.forEach(alert => {
      stats.byStatus[alert.status] = (stats.byStatus[alert.status] || 0) + 1;
      stats.bySeverity[alert.severity] = (stats.bySeverity[alert.severity] || 0) + 1;
      stats.byType[alert.type] = (stats.byType[alert.type] || 0) + 1;
    });
    
    res.json({
      stats,
      locationId,
      timeRange: '24h'
    });
  } catch (error) {
    console.error(`❌ Alert stats error: ${error.message}`);
    res.status(500).json({ error: error.message });
  }
});

// Task 2.4.5: Get escalation configuration and status (REQ-026)
app.get("/api/alerts/escalation", async (req, res) => {
  try {
    // Get escalation statistics
    const activeAlertsArray = Array.from(activeAlerts.values());
    const escalationStats = {
      totalActive: activeAlertsArray.length,
      byEscalationLevel: {
        0: 0, // No escalation
        1: 0, // Level 1
        2: 0, // Level 2  
        3: 0  // Level 3
      },
      escalatedAlerts: activeAlertsArray.filter(alert => alert.escalationLevel > 0).length,
      averageEscalationLevel: 0
    };
    
    // Calculate escalation statistics
    activeAlertsArray.forEach(alert => {
      const level = alert.escalationLevel || 0;
      escalationStats.byEscalationLevel[level] = (escalationStats.byEscalationLevel[level] || 0) + 1;
    });
    
    // Calculate average escalation level
    const totalEscalationLevels = activeAlertsArray.reduce((sum, alert) => sum + (alert.escalationLevel || 0), 0);
    escalationStats.averageEscalationLevel = activeAlertsArray.length > 0 
      ? (totalEscalationLevels / activeAlertsArray.length).toFixed(2)
      : 0;
    
    res.json({
      config: {
        warningEscalationTime: ESCALATION_CONFIG.WARNING_ESCALATION_TIME,
        dangerEscalationTime: ESCALATION_CONFIG.DANGER_ESCALATION_TIME,
        criticalEscalationTime: ESCALATION_CONFIG.CRITICAL_ESCALATION_TIME,
        maxEscalationLevel: ESCALATION_CONFIG.MAX_ESCALATION_LEVEL,
        escalationActions: ESCALATION_CONFIG.ESCALATION_ACTIONS
      },
      stats: escalationStats,
      monitoringActive: escalationInterval !== null
    });
  } catch (error) {
    console.error(`❌ Escalation status error: ${error.message}`);
    res.status(500).json({ error: error.message });
  }
});

app.get("/api/telegram/test", async (req, res) => {
  try {
    if (!telegramBot.enabled) {
      return res.status(400).json({
        error: "Telegram bot is not enabled. Set TELEGRAM_BOT_TOKEN and TELEGRAM_CHAT_IDS environment variables.",
      });
    }

    const success = await telegramBot.sendTest();
    res.json({
      status: success ? "ok" : "failed",
      message: success
        ? "Test message sent successfully"
        : "Failed to send test message",
      chatCount: telegramBot.chatIds.length,
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

server.listen(PORT, () => {
  console.log(`\n🚀 Wokwi bridge running on http://localhost:${PORT}`);
  console.log(`📡 POST sensor data to: http://localhost:${PORT}/api/sensor`);
  console.log(`📈 Latest reading: http://localhost:${PORT}/api/sensor/latest`);
  console.log(`🕒 History feed: http://localhost:${PORT}/api/sensor/history?limit=20`);
  console.log(`💚 Health check: http://localhost:${PORT}/health`);
  console.log(`🔌 WebSocket endpoint: ws://localhost:${PORT}/ws`);
  console.log(`🗂️  Storage mode: ${storageMode}`);
  if (db) {
    console.log(`🔥 Firestore collection: ${FIRESTORE_COLLECTION}`);
  }
  
  // Security status logging (REQ-003)
  if (REQUIRE_API_KEY) {
    if (API_KEY) {
      console.log(`🔐 API Key authentication: ENABLED (${API_KEY_HEADER} header required)`);
      console.log(`🔐 WebSocket authentication: ENABLED (apiKey query param or ${API_KEY_HEADER} header)`);
    } else {
      console.log(`⚠️  API Key authentication: ENABLED but NO KEY CONFIGURED!`);
      console.log(`   Set API_KEY environment variable to secure the API`);
    }
  } else {
    console.log(`🔓 API Key authentication: DISABLED (set REQUIRE_API_KEY=true to enable)`);
    console.log(`🔓 WebSocket authentication: DISABLED`);
  }
  
  // Initialize MQTT client (REQ-007)
  console.log(`📡 MQTT broker: ${MQTT_CONFIG.broker}`);
  initializeMQTT();
  
  // Task 2.4.5: Start alert escalation monitoring (REQ-026)
  startAlertEscalationMonitoring();
  
  console.log("");
});