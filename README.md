# 🚨 Manhole Monitoring System - Real-Time Dashboard

A complete integration of an **ESP32 Wokwi simulator** with a **React real-time dashboard** connected via **Firebase**. Displays live sensor data (CH4, H2S, water level) with alerts when gas leak thresholds are exceeded.

---

## 📋 System Architecture

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
