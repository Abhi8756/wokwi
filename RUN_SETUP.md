# 🚀 Real Wokwi Integration Setup - 4 Terminal Process

## ⚠️ Port Fix Applied
- **Bridge**: Now runs on port **3001** (fixed from 3000)
- **Dashboard**: Runs on port **3000** (Sewerly folder)
- **No more conflicts!**

---

## 📋 Step-by-Step Instructions

### **TERMINAL 1: Firebase Bridge** (Manhole root folder)
```powershell
cd c:\Users\sw\Downloads\Manhole
node bridge.js
```
✅ You should see:
```
🚀 Firebase Bridge running on http://localhost:3001
📡 POST sensor data to: http://localhost:3001/api/sensor
```
**Keep this running!**

---

### **TERMINAL 2: React Dashboard** (Sewerly folder - ⚠️ IMPORTANT!)
```powershell
cd c:\Users\sw\Downloads\Manhole\Sewerly
npm run dev
```
✅ You should see:
```
➜ Local: http://localhost:3000
```
**Keep this running! Open http://localhost:3000 in browser**

---

### **TERMINAL 3: Wokwi File Watcher** (Manhole root folder)
```powershell
cd c:\Users\sw\Downloads\Manhole
python watch_wokwi_file.py
```
✅ You should see:
```
👀 Watching wokwi_output.log for sensor data...
📡 Sending to: http://localhost:3001/api/sensor
⏳ Waiting for Wokwi to start...
```
**Keep this running! It will wait for Step 4**

---

### **TERMINAL 4: Wokwi Simulation** (Manhole root folder)
```powershell
cd c:\Users\sw\Downloads\Manhole
wokwi simulate . > wokwi_output.log 2>&1
```
Or click the Wokwi Play button in VS Code

✅ The watcher (Terminal 3) will start showing data immediately when this runs!

---

## 📊 Data Flow Verification

When all 4 terminals are running:

1. **Terminal 3 (Watcher)** should show:
   ```
   ✅ Stored CH4: 100.5, H2S: 50.2, Water: 90.1
   ✅ Stored CH4: 105.2, H2S: 52.3, Water: 91.5
   ```

2. **Terminal 1 (Bridge)** should show:
   ```
   ✅ Stored in Firestore: sensor_readings/...
   ```

3. **Browser (Port 3000)** should show:
   - Real-time values updating every 1 second
   - Charts moving
   - Alerts when thresholds exceeded

---

## 🆘 Quick Fixes

### Error: "Port 3000 already in use"
```powershell
Get-Process -Name node -ErrorAction SilentlyContinue | Stop-Process -Force
```

### Error: "wokwi_output.log" not found - Terminal 3 keeps waiting
- This is **NORMAL**! Terminal 4 hasn't started yet
- Run the wokwi command in Terminal 4
- Watcher will auto-detect when file is created

### Error: Running npm from wrong directory
- ❌ Don't run from `c:\Users\sw\Downloads\Manhole`
- ✅ Run from `c:\Users\sw\Downloads\Manhole\Sewerly`

---

## ✅ Success Indicators

- [ ] Terminal 1: Bridge running on 3001
- [ ] Terminal 2: Dashboard at localhost:3000
- [ ] Terminal 3: Watching wokwi_output.log
- [ ] Terminal 4: Wokwi simulation running
- [ ] Browser shows live data updating
- [ ] Terminal 3 shows "✅ Stored CH4:" messages

---

## 🎯 Now Go! 🚀

**Open 4 PowerShell windows and follow the steps above!**
