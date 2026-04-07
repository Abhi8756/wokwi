# 🚨 Manhole Monitoring System - Real-Time Dashboard

## 📖 Project Overview

**Manhole Monitoring System** is a **complete real-time monitoring solution** for underground manhole environments. It simulates an **ESP32 microcontroller** deployed in a manhole that measures dangerous gases (methane and hydrogen sulfide) and water levels, then displays this critical data on a **live React dashboard** with instant alerts when dangerous levels are detected.

### What This Project Does

This system monitors **toxic gas buildup and water intrusion** in underground sewage/utility manholes - environments where workers face serious safety risks. The system:

- **Simulates real sensor readings** from an ESP32 microcontroller
- **Captures and transmits data** in real-time to a cloud database
- **Displays live metrics** on an interactive web dashboard
- **Triggers instant alerts** when dangerous thresholds are exceeded
- **Stores historical data** for trend analysis and reporting

### Why It's Needed

**Safety-Critical Application:** Manhole workers face two major threats:
1. **Toxic Gas Poisoning** - Methane (CH4) and Hydrogen Sulfide (H2S) buildup from decomposing waste
2. **Water Accumulation** - Flooding due to heavy rain or system overflow

**Traditional Solution Problems:**
- ❌ Manual entry → Dangerous, slow, unreliable
- ❌ Delayed alerting → Workers may enter unsafe conditions
- ❌ No historical data → Can't predict future incidents
- ❌ Disconnected systems → Multiple apps, no integration

**This Solution:**
- ✅ **Automatic monitoring** - Real-time sensor readings every second
- ✅ **Instant alerts** - Dashboard notifies immediately when thresholds exceeded
- ✅ **Cloud storage** - Data persists in Firebase for analysis
- ✅ **Integrated platform** - All data in one React dashboard
- ✅ **Scalable** - Can monitor multiple manholes simultaneously

---

## 🏗️ System Architecture

---

## � Complete File-by-File Explanation

### **1. ESP32 Firmware - `src/main.cpp`**

**What It Does:**
- Simulates an ESP32 microcontroller deployed in a manhole
- Reads sensor values (or simulates them)
- Outputs data in JSON format every 1 second
- Detects alert conditions

**Key Code:**
```cpp
// Simulates CH4 level (in ppm - parts per million)
float ch4_ppm = 100.0 + (millis() / 100) * 0.5;

// Simulates H2S level
float h2s_ppm = 50.0 + (millis() / 200) * 0.3;

// Simulates water level (in cm)
float water_level = 90.0 + (millis() / 500) * 0.1;

// Creates JSON output
Serial.print("{\"ch4\":");
Serial.print(ch4_ppm, 2);
Serial.print(",\"h2s\":");
Serial.print(h2s_ppm, 2);
Serial.print(",\"water\":");
Serial.print(water_level, 2);
Serial.println("}");
```

**Why This Design:**
- JSON format is universal and easy to parse
- One reading per second matches real sensor update rates
- Simulation allows development without real hardware
- Runs in Wokwi VS Code extension (no actual ESP32 needed)

---

### **2. Firebase Bridge - `bridge.js` (Node.js + Express)**

**What It Does:**
- Acts as a **gateway between the simulator and Firebase**
- Receives HTTP POST requests with sensor JSON data
- Validates the data format
- Stores data in Firebase Firestore database
- Provides health check endpoint

**How It Works:**
```javascript
// Receives POST request at http://localhost:3001/api/sensor
app.post("/api/sensor", async (req, res) => {
  const { ch4, h2s, water, alert } = req.body;
  
  // Validate data exists
  if (ch4 === undefined || h2s === undefined || water === undefined) {
    return res.status(400).json({ error: "Missing fields" });
  }
  
  // Store in Firestore with timestamp
  await db.collection("sensor_readings").add({
    ch4,
    h2s,
    water,
    alert: ch4 > 200 || h2s > 100,
    timestamp: admin.firestore.Timestamp.now()
  });
  
  res.json({ success: true });
});
```

**Key Features:**
- **Validation** - Ensures data has all required fields
- **Timestamp** - Server adds exact time reading was received
- **Auto-calculation** - Alert status computed from thresholds
- **Error handling** - Returns proper HTTP status codes
- **CORS enabled** - React dashboard can call it from browser

**Port:** `3001` (to avoid conflict with React dashboard on 3000)

**Why This Exists:**
- Bridges the gap between simulator and cloud
- Prevents direct simulator-to-Firebase connections
- Adds validation layer for data quality
- Can be extended with additional logic (email alerts, SMS, etc.)

---

### **3. Wokwi File Watcher - `watch_wokwi_file.py` (Python)**

**What It Does:**
- **Monitors `wokwi_output.log`** for new JSON lines written by the ESP32 simulation
- Reads new JSON data every 0.5 seconds
- **Parses the JSON** to extract sensor values
- **HTTP POSTs** the data to the Firebase Bridge
- Displays formatted status messages to console

**How It Works:**
```python
# Continuously watch the log file
with open("wokwi_output.log", 'r') as f:
    f.seek(last_position)  # Start from where we left off
    lines = f.readlines()  # Get new lines only
    
    for line in lines:
        if line.startswith('{'):
            data = json.loads(line)  # Parse JSON
            
            # POST to bridge
            requests.post(
                "http://localhost:3001/api/sensor",
                json=data,
                timeout=5
            )
```

**Data Flow Through Watcher:**
```
wokwi_output.log (file)
    ↓ (read every 0.5s)
Parse JSON line
    ↓ (extract ch4, h2s, water)
Validate format
    ↓ (check required fields exist)
HTTP POST request
    ↓ (send to localhost:3001)
Firebase Bridge
    ↓ (stores in Firestore)
Firebase Database
```

**Why Python for This:**
- Easy file I/O and JSON parsing
- Lightweight and fast
- Can run independently in background
- Simple to debug and modify

---

### **4. Test Data Simulator - `simulate_wokwi_output.py` (Python)**

**What It Does:**
- Generates **fake but realistic sensor data** to test the system
- Creates `wokwi_output.log` with JSON lines
- Simulates gradual gas leak progression
- Useful if Wokwi simulator isn't available

**When to Use:**
- Testing dashboard without ESP32/Wokwi running
- Verifying entire pipeline works
- Stress testing with continuous data
- Development when Wokwi is acting up

**Data Generation Logic:**
```python
ch4 += 5.0 + (count % 3) * 2      # Gradually increasing
h2s += 2.0 + (count % 2) * 1.5    # Realistic progression
alert = ch4 > 200 or h2s > 100    # Triggers at thresholds
```

---

### **5. React Dashboard - `Sewerly/src/components/Dashboard.jsx`**

**What It Does:**
- **Displays real-time sensor data** in a beautiful, interactive interface
- **Connects to Firebase** for real-time updates
- **Shows live charts** tracking sensor values over time
- **Generates alerts** when thresholds exceeded
- **Updates automatically** every second without page refresh

**Architecture:**
```jsx
// Real-time listener to Firebase Firestore
const subscribeToLatestReading = (callback) => {
  const q = query(
    collection(db, "sensor_readings"),
    orderBy("timestamp", "desc"),  // Most recent first
    limit(1)                         // Only latest reading
  );
  
  return onSnapshot(q, (snapshot) => {
    const data = snapshot.docs[0]?.data();
    callback(data);  // Update React state
  });
};

// In component - updates UI whenever data changes
useEffect(() => {
  const unsubscribe = subscribeToLatestReading((data) => {
    setData(data);  // Triggers re-render
    generateAlerts(data);  // Check for alerts
  });
  
  return () => unsubscribe();  // Cleanup
}, []);
```

**Dashboard Features:**
- **Live Metrics** - CH4, H2S, Water values in large display
- **Real-time Charts** - LineChart showing last 20 readings trend
- **Alert Box** - Red highlighting when dangerous levels detected
- **Timestamp** - Shows when last reading was received
- **Status Indicators** - ✓ Normal or 🚨 Alert

**Port:** `3000` (Vite dev server)

**Why React for This:**
- Real-time UI updates (re-render on data change)
- Beautiful component-based architecture
- Firebase integration is seamless
- Responsive design for mobile/tablet access
- Fast performance with Vite

---

### **6. Firebase Configuration - `Sewerly/src/utils/firebase.js`**

**What It Does:**
- **Initializes Firebase connection** with credentials
- **Sets up Firestore listeners** for real-time data
- **Exports subscription functions** for components to use
- **Handles authentication** with Firebase service account

**Configuration:**
```javascript
import { initializeApp } from "firebase/app";
import { getFirestore } from "firebase/firestore";

const firebaseConfig = {
  apiKey: process.env.REACT_APP_FIREBASE_API_KEY,
  authDomain: "manhole-monitoring-e5d2b.firebaseapp.com",
  projectId: "manhole-monitoring-e5d2b",
  // ... more config
};

const app = initializeApp(firebaseConfig);
export const db = getFirestore(app);
```

**Real-Time Listener Function:**
```javascript
export const subscribeToLatestReading = (callback) => {
  const q = query(
    collection(db, "sensor_readings"),
    orderBy("timestamp", "desc"),
    limit(1)
  );
  
  // onSnapshot = real-time listener
  // Fires every time data changes in Firestore
  return onSnapshot(
    q,
    (snapshot) => {
      const data = snapshot.docs[0]?.data();
      callback(data);  // Call React callback with new data
    },
    (error) => console.error("Firebase error:", error)
  );
};
```

**Why Firestore Real-Time Listeners:**
- Data appears on dashboard within milliseconds
- No polling needed (efficient)
- Automatic connection management
- Works even if app loses internet briefly

---

### **7. PlatformIO Configuration - `platformio.ini`**

**What It Does:**
- **Configures ESP32 build settings** for compilation
- **Specifies board type** (esp32doit-devkit-v1)
- **Sets serial monitor speed** (115200 baud)
- **Defines framework** (Arduino)

```ini
[env:esp32doit-devkit-v1]
platform = espressif32           # Espressif ESP32 platform
board = esp32doit-devkit-v1     # Specific board model
framework = arduino              # Arduino code style
monitor_speed = 115200           # Serial port speed
```

---

### **8. Wokwi Simulator Configuration - `wokwi.toml`**

**What It Does:**
- **Configures Wokwi simulator** settings
- **Specifies firmware file** to simulate
- **Links circuit diagram** (breadboard layout)

```toml
[wokwi]
version = 1
elf = "firmware.elf"           # Compiled ESP32 code
firmware = "firmware.bin"       # Binary firmware

[wokwi.diagram]
version = 1
file = "diagram.json"           # Circuit diagram
```

---

### **9. Circuit Diagram - `diagram.json`**

**What It Does:**
- **Defines the virtual circuit** in Wokwi
- **Lists all components** (sensors, boards)
- **Specifies connections** between components

**Example Components:**
- ESP32 microcontroller (main processor)
- MQ-4 sensor (CH4 detection)
- MQ-136 sensor (H2S detection)
- Water level sensor
- Capacitor, resistors (sensor support components)

---

## 💾 Data Storage - Firebase Firestore

### Where Data Is Stored

**Database:** Firebase Firestore (Cloud NoSQL database)  
**Collection Name:** `sensor_readings`  
**Location:** Google Cloud (auto-replicated for backup)

### Data Schema

Each document in `sensor_readings` contains:

```json
{
  "ch4": 117.00,              // Methane level (ppm)
  "h2s": 1.00,                // Hydrogen sulfide (ppm)
  "water": 0.00,              // Water level (cm)
  "alert": false,             // Safety alert flag
  "timestamp": "2026-04-07T22:53:57Z"  // Server timestamp
}
```

### How Data Is Stored

1. **Sensor generates** JSON every second in `main.cpp`
2. **Watcher reads** from `wokwi_output.log`
3. **Watcher POSTs** to Bridge (`http://localhost:3001/api/sensor`)
4. **Bridge validates** the data
5. **Bridge calls Firestore** with `db.collection("sensor_readings").add(data)`
6. **Firestore stores** with auto-generated document ID and server timestamp
7. **Document saved** with auto-sync to Google Cloud

### Data Retention

- **Historical Data:** Stored indefinitely in Firestore
- **Real-Time Data:** Latest reading cached in browser React state
- **Queries:** Can retrieve last 20 readings, hourly averages, etc.

---

## 🔄 How Data Flows Through The System

### Complete Data Pipeline

```
┌──────────────────────────────────────────────────────────────┐
│ 1. ESP32 Firmware (src/main.cpp)                             │
│    - Reads from simulated sensors (or real sensors)          │
│    - Outputs JSON every 1 second to serial/console:          │
│    {"ch4":117.00,"h2s":1.00,"water":0.00,"alert":false}     │
└─────────────────────┬──────────────────────────────────────┘
                      │ JSON output
                      ▼
┌──────────────────────────────────────────────────────────────┐
│ 2. Wokwi Simulator (VS Code Extension)                       │
│    - Runs ESP32 firmware in virtual environment              │
│    - Terminal shows JSON output (human-readable)             │
│    - Output redirected to wokwi_output.log file              │
└─────────────────────┬──────────────────────────────────────┘
                      │ Writes JSON lines to file
                      ▼
┌──────────────────────────────────────────────────────────────┐
│ 3. File Watcher (watch_wokwi_file.py)                        │
│    - Monitors wokwi_output.log for new lines                 │
│    - Reads file every 0.5 seconds                            │
│    - Parses JSON to extract ch4, h2s, water values           │
│    - Validates data has required fields                      │
└─────────────────────┬──────────────────────────────────────┘
                      │ HTTP POST request
                      ▼
┌──────────────────────────────────────────────────────────────┐
│ 4. Firebase Bridge (bridge.js - Node.js/Express)             │
│    - Listens on http://localhost:3001/api/sensor             │
│    - Receives JSON POST data                                 │
│    - Validates: ch4, h2s, water fields exist                 │
│    - Calculates alert: true if ch4>200 or h2s>100            │
│    - Adds server timestamp                                   │
│    - Logs receipt: "✅ Stored in Firestore"                  │
└─────────────────────┬──────────────────────────────────────┘
                      │ Firestore write operation
                      ▼
┌──────────────────────────────────────────────────────────────┐
│ 5. Firebase Firestore (Cloud Database)                       │
│    - Collection: sensor_readings                             │
│    - Stores document: {ch4, h2s, water, alert, timestamp}    │
│    - Auto-generates unique document ID                       │
│    - Real-time sync to connected clients                     │
│    - Backup to Google Cloud storage                          │
└─────────────────────┬──────────────────────────────────────┘
                      │ Real-time listener notification
                      ▼
┌──────────────────────────────────────────────────────────────┐
│ 6. React Dashboard (Sewerly/src/components/Dashboard.jsx)    │
│    - subscribeToLatestReading() listening to Firestore       │
│    - Receives notification of new document                   │
│    - Extracts: ch4, h2s, water, alert values                 │
│    - Updates React state: setData(newReading)                │
│    - Component re-renders with new values                    │
│    - Charts update with new data point                       │
│    - Alert box shows if alert=true                           │
└─────────────────────┬──────────────────────────────────────┘
                      │ Browser display
                      ▼
┌──────────────────────────────────────────────────────────────┐
│ 7. Browser (http://localhost:3000)                           │
│    - User sees live metrics updating every second            │
│    - Charts animate with new data                            │
│    - Alert appears in red if thresholds exceeded             │
│    - All data displayed in real-time                         │
└──────────────────────────────────────────────────────────────┘

⏱️  Total Latency: ~100-500ms (sensor to browser)
🔄 Update Frequency: Every 1 second
```

---

## 🔗 How Dashboard Connects to Everything

### Connection Architecture

```
React Component (Dashboard.jsx)
    │
    ├─► Firebase Listener (firebase.js)
    │   │
    │   ├─► Firestore Real-Time Listener (subscribeToLatestReading)
    │   │   │
    │   │   └─► Query: Latest sensor_readings sorted by timestamp
    │   │       └─► onSnapshot() triggers on every database change
    │   │
    │   └─► Returns data to React callback
    │
    ├─► useEffect Hook
    │   │
    │   └─► Updates React State (setData)
    │       └─► Triggers component re-render
    │
    └─► UI Components (Metric Display, Charts, Alerts)
        │
        ├─► Display CH4 value
        ├─► Display H2S value
        ├─► Display Water level
        ├─► Show Alert if needed
        └─► Recharts updates with new data point
```

### Code Flow

```javascript
// 1. Component mounts
useEffect(() => {
  
  // 2. Subscribe to Firebase real-time listener
  const unsubscribe = subscribeToLatestReading((latestReading) => {
    
    // 3. Data received from Firebase
    // 4. Update React state
    setData(latestReading);
    
    // 5. Generate alerts based on thresholds
    generateAlerts(latestReading);
    
  });
  
  // 6. Cleanup when component unmounts
  return () => unsubscribe();
  
}, []);  // Empty dependency = runs once on mount

// 7. When state updates, React automatically re-renders
// 8. New values display in JSX
// 9. Charts update with new data point
// 10. Cycle repeats when next Firebase update arrives
```

### Why Real-Time Listeners vs Polling

**Using Firebase `onSnapshot()` (Current):**
- ✅ Data appears instantly when updated
- ✅ No wasted network requests
- ✅ Efficient bandwidth usage
- ✅ Automatic connection management
- ✅ Handles offline gracefully

**vs API Polling (Alternative):**
- ❌ Constant network requests (wasteful)
- ❌ 5-10 second delay in updates
- ❌ More server load
- ❌ Worse battery life on mobile
- ❌ Unnecessarily complex

---

## 🗄️ Data Request Examples

### Getting Latest Reading (Real-Time)

```javascript
// Dashboard component - always has latest data through listener
const [data, setData] = useState(null);

useEffect(() => {
  return subscribeToLatestReading((latestReading) => {
    setData(latestReading);  // Updates automatically
  });
}, []);
```

### Getting Last 20 Readings (For Charts)

```javascript
const lastReadings = db
  .collection("sensor_readings")
  .orderBy("timestamp", "desc")
  .limit(20);
```

### Getting Readings from Last Hour

```javascript
const oneHourAgo = new Date(Date.now() - 3600000);

const hourReadings = db
  .collection("sensor_readings")
  .where("timestamp", ">", oneHourAgo)
  .orderBy("timestamp", "desc");
```

### Getting All Alerts

```javascript
const alerts = db
  .collection("sensor_readings")
  .where("alert", "==", true)
  .orderBy("timestamp", "desc");
```

---

## 📊 Sensor Thresholds & Safety Levels

| Sensor | Safe Level | Warning | Dangerous | Unit |
|--------|-----------|---------|-----------|------|
| **CH4** (Methane) | 0-50 ppm | 50-200 ppm | > 200 ppm | ppm |
| **H2S** (H-Sulfide) | 0-20 ppm | 20-100 ppm | > 100 ppm | ppm |
| **Water** | 0-80 cm | 80-100 cm | > 100 cm | cm |

**Alert Trigger:**
- Alert = TRUE when: `CH4 > 200 OR H2S > 100`
- Dashboard shows 🚨 ALERT in red
- Both metrics must exceed threshold to be safe again

---

## 🔑 Key Concepts

### JSON as Universal Format
- ESP32 outputs data as JSON
- Watcher reads and validates JSON
- Bridge transmits JSON to cloud
- Firestore stores JSON documents
- Dashboard decodes JSON for display
- **Benefit:** Any component can be replaced without changing data format

### Real-Time vs Historical
- **Real-Time:** Latest reading shown on dashboard (from React state)
- **Historical:** All past readings stored in Firestore forever
- **Charts:** Show trend of last 20 readings (pseudo-historical)

### Separation of Concerns
- **ESP32:** Only generates sensor data
- **Watcher:** Only reads and forwards data
- **Bridge:** Only validates and stores
- **Firestore:** Only persists data
- **Dashboard:** Only displays data
- **Benefit:** Each component can be fixed/upgraded independently

### Scalability
- Add 100 maltholes? → Just deploy more watchers + sensors
- Same bridge server handles all
- Firestore auto-scales with load
- Dashboard queries just slice of data
- **System grows without changes to core logic**

---

```
┌─────────────────────┐
│   Wokwi Simulator   │ (ESP32 firmware)
│   Outputs JSON      │
└──────────┬──────────┘
           │ JSON lines to wokwi_output.log
           ▼
┌─────────────────────────────────────┐
│  watch_wokwi_file.py (Watcher)      │ (Terminal 3)
│  Reads JSON, POSTs to Bridge        │
└──────────┬──────────────────────────┘
           │ HTTP POST :3001/api/sensor
           ▼
┌─────────────────────────────────────┐
│  Node.js Express Bridge             │ (Terminal 1, Port 3001)
│  Receives data, stores in Firebase  │
└──────────┬──────────────────────────┘
           │ Firestore Database
           ▼
┌─────────────────────────────────────┐
│  React Dashboard (Vite)             │ (Terminal 2, Port 3000)
│  Real-time listeners to Firebase    │
│  Shows live metrics & alerts        │
└─────────────────────────────────────┘
```

---

## 🎯 Quick Start - 4 Terminal Process

### **Prerequisites**
- ✅ Node.js v16+ installed
- ✅ Python 3.7+ installed
- ✅ Firebase project configured (`serviceAccountKey.json` in root)
- ✅ Dependencies installed (run `npm install` in root and `Sewerly/npm install` in `Sewerly/`)
- ✅ Wokwi simulation already running in VS Code

---

## ⚡ Terminal 1: Firebase Bridge (Port 3001)

**Location:** Manhole root folder  
**Purpose:** Receives sensor JSON, validates, stores in Firebase Firestore

```powershell
cd c:\Users\sw\Downloads\Manhole
node bridge.js
```

**Expected Output:**
```
🚀 Firebase Bridge running on http://localhost:3001
📡 POST sensor data to: http://localhost:3001/api/sensor
💚 Health check: http://localhost:3001/health
```

**Keep this running!** This is the backend that routes data to Firebase.

---

## ⚡ Terminal 2: React Dashboard (Port 3000)

**Location:** Manhole/Sewerly folder ⚠️ **IMPORTANT**  
**Purpose:** Real-time React dashboard with Firebase listeners

```powershell
cd c:\Users\sw\Downloads\Manhole\Sewerly
npm run dev
```

**Expected Output:**
```
VITE v5.0.0  ready in 234 ms

➜  Local:   http://localhost:3000/
```

**Open in browser:** http://localhost:3000  
You should see the dashboard (even with no data initially).

---

## ⚡ Terminal 3: Wokwi File Watcher

**Location:** Manhole root folder  
**Purpose:** Monitors `wokwi_output.log`, parses JSON, sends to bridge

```powershell
cd c:\Users\sw\Downloads\Manhole
python .\watch_wokwi_file.py
```

**Expected Output:**
```
👀 Watching wokwi_output.log for sensor data...
📡 Sending to: http://localhost:3001/api/sensor
⏳ Waiting for Wokwi to start...
```

**Waits for Wokwi output file** - that's normal! Once Wokwi simulator starts, it will auto-detect.

---

## ⚡ Terminal 4: Choose ONE Option

### **Option A: Real Wokwi Data (Recommended)**

Use PlatformIO to capture actual ESP32 simulator output:

```powershell
cd c:\Users\sw\Downloads\Manhole
pio device monitor > wokwi_output.log
```

This captures **REAL firmware output** from your Wokwi simulation to the log file.

### **Option B: Test/Simulated Data**

If Option A doesn't work, use the test simulator:

```powershell
cd c:\Users\sw\Downloads\Manhole
python simulate_wokwi_output.py
```

This generates **fake but realistic sensor data** for testing the dashboard.

---

## 📊 What Happens When All 4 Terminals Are Running

1. **Wokwi simulation** (Terminal 4) outputs JSON every second:
   ```json
   {"ch4": 117.00, "h2s": 1.00, "water": 0.00, "alert": false}
   ```

2. **File watcher** (Terminal 3) reads JSON and shows:
   ```
   [22:53:57] [1] ✓ CH4: 117.0ppm | H2S: 1.0ppm | Water: 0.0cm
   [22:53:58] [2] ✓ CH4: 234.0ppm | H2S: 3.0ppm | Water: 399.96cm
   ```

3. **Bridge** (Terminal 1) receives data and stores in Firebase:
   ```
   ✅ Stored in Firestore: sensor_readings/...
   ```

4. **Dashboard** (Terminal 2) at http://localhost:3000 shows:
   - CH4 level (ppm)
   - H2S level (ppm)
   - Water level (cm)
   - Alert status (🚨 if CH4 > 200 or H2S > 100)
   - Real-time charts updating every second

---

## ✅ Success Verification Checklist

- [ ] Terminal 1: Bridge running on `http://localhost:3001` ✓
- [ ] Terminal 2: Dashboard accessible at `http://localhost:3000` ✓
- [ ] Terminal 3: File watcher showing data flowing (shows CH4/H2S values) ✓
- [ ] Terminal 4: Wokwi simulation running (produces output) ✓
- [ ] Browser shows **live values updating** every second ✓
- [ ] Terminal 3 shows `✅ Stored` messages ✓
- [ ] When CH4 > 200: Alert shows `🚨 ALERT!` ✓

---

## 🔧 Project File Structure

```
Manhole/
├── src/
│   └── main.cpp              # ESP32 firmware (outputs JSON)
├── bridge.js                 # Express server (receives data)
├── watch_wokwi_file.py       # Reads wokwi_output.log (sends to bridge)
├── simulate_wokwi_output.py  # Optional: generates fake data
├── wokwi_output.log          # Created by Wokwi simulation
├── Sewerly/                  # React dashboard folder
│   ├── src/
│   │   ├── components/Dashboard.jsx
│   │   └── utils/firebase.js
│   └── package.json
├── platformio.ini            # PlatformIO config
└── wokwi.toml               # Wokwi config

```

---

## 🚀 Data Flow Summary

```
Wokwi Simulator (Real ESP32 Firmware)
    ↓ (JSON every 1 second)
wokwi_output.log (file)
    ↓ (read by watcher)
watch_wokwi_file.py (Terminal 3)
    ↓ (HTTP POST)
Firebase Bridge (Terminal 1, :3001)
    ↓ (Firestore write)
Firebase Firestore Database
    ↓ (Real-time listener)
React Dashboard (Terminal 2, :3000)
    ↓ (Display)
Browser (http://localhost:3000) 🎉
```

---

## 🆘 Troubleshooting

### Port Already In Use
```powershell
# Kill all Node processes
Get-Process -Name node -ErrorAction SilentlyContinue | Stop-Process -Force
```

### Dashboard shows no data after 30 seconds
1. **Check Terminal 3:** Is it showing `✅ Stored CH4:`? 
   - If YES → Data is flowing, check browser console (F12) for Firebase errors
   - If NO → Terminal 4 (Wokwi) didn't start properly

2. **Check Terminal 1:** Is bridge receiving POSTs?
   - Should show messages like `✅ Stored in Firestore`
   - If not → watcher not sending data (check Terminal 3)

3. **Check Terminal 2:** Any errors in React dev server output?
   - Check for Firebase connection errors
   - Try refreshing browser

### "wokwi_output.log not found"
This is **NORMAL** until you run Terminal 4. The watcher waits for the file to be created by Wokwi.

### npm install missing dependencies
```powershell
# In Manhole root
npm install

# In Manhole/Sewerly
cd Sewerly
npm install
```

### Firebase authentication errors
- Verify `serviceAccountKey.json` exists in Manhole root
- Check file has correct Firebase credentials
- Ensure Firestore database exists in Firebase Console

---

## 🎯 Data Schema

**Firestore Collection:** `sensor_readings`

```json
{
  "ch4": 117.00,           // Methane level (ppm)
  "h2s": 1.00,             // Hydrogen sulfide level (ppm)
  "water": 0.00,           // Water level (cm)
  "alert": false,          // Alert triggered (CH4 > 200 OR H2S > 100)
  "timestamp": "2026-04-07T22:53:57Z"
}
```

---

## 📈 Alert Thresholds

| Threshold | Status |
|-----------|--------|
| CH4 ≤ 200 ppm AND H2S ≤ 100 ppm | ✓ Normal |
| CH4 > 200 ppm OR H2S > 100 ppm | 🚨 Alert |

When alert triggers, the dashboard shows:
- Red background alert box
- Alert message with gas levels
- Sound notification (optional)

---

## 🔄 Real-Time Features

✅ **Firebase Real-Time Listeners** - Updates every second without page refresh  
✅ **Live Charts** - CH4/H2S/Water updated in real-time  
✅ **Alert System** - Instant notifications when thresholds exceeded  
✅ **Historical Data** - All readings stored in Firestore for analysis  

---

## 📝 Commands Reference

| Command | Purpose |
|---------|---------|
| `node bridge.js` | Start Firebase Bridge (Terminal 1) |
| `npm run dev` | Start Dashboard (Terminal 2, from Sewerly/) |
| `python watch_wokwi_file.py` | Start File Watcher (Terminal 3) |
| `pio device monitor > wokwi_output.log` | Capture real Wokwi output (Terminal 4) |
| `python simulate_wokwi_output.py` | Generate test data (Terminal 4 alternative) |

---

## 🎉 You're Done!

Once all 4 terminals are running, your **real-time manhole monitoring system is live**. Open http://localhost:3000 and watch your ESP32 sensor data flow in real-time! 🚀

---

**Project Built With:**
- ESP32 (Wokwi Simulator)
- PlatformIO + Arduino
- Node.js + Express
- React + Vite
- Firebase Firestore
- Python (data pipeline)
