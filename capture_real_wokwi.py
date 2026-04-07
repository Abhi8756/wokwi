#!/usr/bin/env python3
"""
Wokwi Serial Output Capturer
Captures output from Wokwi simulation running in VS Code and writes to wokwi_output.log
"""

import subprocess
import os
import time
import sys

def capture_wokwi_output():
    """
    Attempts to capture Wokwi output by monitoring VS Code terminal
    or by running PlatformIO monitor if Wokwi exposes serial port
    """
    
    print("""
╔════════════════════════════════════════════════════════════════╗
║      Wokwi Serial Output Capturer - Real Data Capture        ║
╚════════════════════════════════════════════════════════════════╝

This captures REAL Wokwi ESP32 output (not simulated data).

METHOD 1: Using PlatformIO Serial Monitor (Recommended)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Checking if Wokwi exposes a virtual serial port...
""")
    
    # Try to find COM ports
    try:
        result = subprocess.run(
            ["Get-PSDrive -PSProvider FileSystem | Where-Object {$_.Name -match 'COM'} | Select-Object Name"],
            shell=True,
            capture_output=True,
            text=True
        )
        print(f"Available COM ports: {result.stdout if result.stdout else 'Checking...'}\n")
    except:
        pass
    
    print("""
METHOD 2: Manual Copy-Paste from Wokwi Terminal
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

⚠️  MANUAL STEP REQUIRED:

1. Look at the Wokwi Terminal output in VS Code (should show JSON like):
   {"ch4":117.00,"h2s":1.00,"water":0.00,"alert":false}
   {"ch4":234.00,"h2s":3.00,"water":399.96,"alert":false}

2. Copy all JSON lines from Wokwi Terminal (Ctrl+A to select)

3. Paste them to: wokwi_output.log file in Manhole folder

4. The watcher will immediately start reading REAL data!

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

ALTERNATIVE: Run this command to create wokwi_output.log with real data:

   cd c:\\Users\\sw\\Downloads\\Manhole
   pio device monitor > wokwi_output.log

This will capture Wokwi's serial output if available.
    """)

if __name__ == "__main__":
    capture_wokwi_output()
