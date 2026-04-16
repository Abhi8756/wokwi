# Documentation Directory Structure

Complete overview of the documentation organization.

---

## 📁 Directory Tree

```
docs/
│
├── README.md                           # Main documentation index
├── DIRECTORY_STRUCTURE.md              # This file
│
├── getting-started/                    # Quick start guides
│   ├── README.md                       # Getting started index
│   ├── HOW_TO_START.md                 # Main startup guide ⭐
│   ├── START_MQTT.md                   # MQTT quick start
│   └── QUICK_START_TELEGRAM.md         # Telegram quick start
│
├── mqtt/                               # MQTT documentation
│   ├── README.md                       # MQTT documentation index
│   ├── MQTT_QUICK_START.md             # Quick reference ⭐
│   ├── MQTT_SETUP_GUIDE.md             # Complete setup guide
│   ├── MQTT_ARCHITECTURE.md            # Architecture details
│   └── MQTT_SETUP_COMPLETE.md          # Setup summary
│
├── vscode/                             # VS Code tasks
│   ├── README.md                       # VS Code tasks index
│   ├── TASKS_QUICK_REFERENCE.md        # One-page cheat sheet ⭐
│   ├── VSCODE_TASKS_GUIDE.md           # Complete guide
│   └── README_TASKS.md                 # Configuration summary
│
└── telegram/                           # Telegram bot
    ├── README.md                       # Telegram documentation index
    ├── TELEGRAM_BOT_SETUP.md           # Setup guide ⭐
    ├── TELEGRAM_BOT_IMPLEMENTATION.md  # Implementation details
    ├── TELEGRAM_BOT_TECHNICAL.md       # Technical documentation
    └── TELEGRAM_BOT.md                 # Feature overview
```

---

## 📚 Documentation Categories

### 🚀 Getting Started (4 files)
Quick start guides for new users.

**Start here if you're new!**

| File | Purpose | When to Read |
|------|---------|-------------|
| `README.md` | Index | Navigation |
| `HOW_TO_START.md` ⭐ | Main guide | First time setup |
| `START_MQTT.md` | MQTT setup | Setting up MQTT |
| `QUICK_START_TELEGRAM.md` | Telegram setup | Setting up alerts |

---

### 📡 MQTT (5 files)
Complete MQTT documentation.

**Read this to understand MQTT communication.**

| File | Purpose | When to Read |
|------|---------|-------------|
| `README.md` | Index | Navigation |
| `MQTT_QUICK_START.md` ⭐ | Quick start | Fast setup |
| `MQTT_SETUP_GUIDE.md` | Complete guide | Detailed setup |
| `MQTT_ARCHITECTURE.md` | Architecture | Understanding system |
| `MQTT_SETUP_COMPLETE.md` | Summary | After setup |

---

### 🔧 VS Code (4 files)
Task automation documentation.

**Read this to automate your workflow.**

| File | Purpose | When to Read |
|------|---------|-------------|
| `README.md` | Index | Navigation |
| `TASKS_QUICK_REFERENCE.md` ⭐ | Cheat sheet | Daily reference |
| `VSCODE_TASKS_GUIDE.md` | Complete guide | Learning tasks |
| `README_TASKS.md` | Summary | Understanding setup |

---

### 📱 Telegram (5 files)
Telegram bot documentation.

**Read this to set up alert notifications.**

| File | Purpose | When to Read |
|------|---------|-------------|
| `README.md` | Index | Navigation |
| `TELEGRAM_BOT_SETUP.md` ⭐ | Setup guide | Setting up bot |
| `TELEGRAM_BOT_IMPLEMENTATION.md` | Code details | Understanding code |
| `TELEGRAM_BOT_TECHNICAL.md` | Technical | Deep dive |
| `TELEGRAM_BOT.md` | Overview | Quick overview |

---

## 🎯 Quick Navigation

### By User Type

#### New Users
1. [Getting Started Index](getting-started/README.md)
2. [How to Start](getting-started/HOW_TO_START.md)
3. [MQTT Quick Start](mqtt/MQTT_QUICK_START.md)

#### Developers
1. [MQTT Architecture](mqtt/MQTT_ARCHITECTURE.md)
2. [VS Code Tasks Guide](vscode/VSCODE_TASKS_GUIDE.md)
3. [Telegram Implementation](telegram/TELEGRAM_BOT_IMPLEMENTATION.md)

#### System Administrators
1. [MQTT Setup Guide](mqtt/MQTT_SETUP_GUIDE.md)
2. [Telegram Bot Setup](telegram/TELEGRAM_BOT_SETUP.md)
3. [VS Code Tasks](vscode/README_TASKS.md)

---

### By Task

#### I want to start the system
→ [How to Start](getting-started/HOW_TO_START.md)

#### I want to set up MQTT
→ [MQTT Quick Start](mqtt/MQTT_QUICK_START.md)

#### I want to use VS Code tasks
→ [Tasks Quick Reference](vscode/TASKS_QUICK_REFERENCE.md)

#### I want to set up Telegram
→ [Telegram Bot Setup](telegram/TELEGRAM_BOT_SETUP.md)

#### I want to understand architecture
→ [MQTT Architecture](mqtt/MQTT_ARCHITECTURE.md)

#### I want to troubleshoot
→ Check troubleshooting sections in relevant guides

---

## 📖 File Naming Convention

### Prefixes
- `README.md` - Index/navigation files
- `HOW_TO_` - Step-by-step guides
- `MQTT_` - MQTT-related documentation
- `TELEGRAM_BOT_` - Telegram bot documentation
- `VSCODE_` or `TASKS_` - VS Code tasks documentation

### Suffixes
- `_GUIDE.md` - Complete guides
- `_QUICK_START.md` - Quick reference
- `_REFERENCE.md` - Cheat sheets
- `_SETUP.md` - Setup instructions
- `_ARCHITECTURE.md` - Architecture details
- `_IMPLEMENTATION.md` - Implementation details
- `_TECHNICAL.md` - Technical documentation

---

## 🔗 Cross-References

Documentation files reference each other:

```
Main README
    ├─> Getting Started Index
    │   ├─> How to Start
    │   ├─> MQTT Quick Start
    │   └─> Telegram Quick Start
    │
    ├─> MQTT Index
    │   ├─> MQTT Quick Start
    │   ├─> MQTT Setup Guide
    │   ├─> MQTT Architecture
    │   └─> MQTT Setup Complete
    │
    ├─> VS Code Index
    │   ├─> Tasks Quick Reference
    │   ├─> VS Code Tasks Guide
    │   └─> README Tasks
    │
    └─> Telegram Index
        ├─> Telegram Bot Setup
        ├─> Telegram Bot Implementation
        ├─> Telegram Bot Technical
        └─> Telegram Bot Overview
```

---

## 📊 Documentation Statistics

### Total Files: 19

- **Index files**: 5 (README.md in each folder)
- **Getting Started**: 3 guides
- **MQTT**: 4 documents
- **VS Code**: 3 documents
- **Telegram**: 4 documents

### File Sizes (Approximate)

- **Quick Start**: 1-2 pages
- **Complete Guides**: 5-10 pages
- **Architecture**: 10-15 pages
- **Index Files**: 1-2 pages

---

## 🎨 Documentation Style

### Consistent Elements

All documentation includes:
- ✅ Clear headings with emojis
- ✅ Table of contents (for long docs)
- ✅ Code examples
- ✅ Troubleshooting sections
- ✅ Cross-references
- ✅ Quick tips
- ✅ Related documentation links

### Formatting

- **Headers**: Use emojis for visual appeal
- **Code blocks**: Syntax highlighting
- **Tables**: For comparisons and references
- **Lists**: For steps and features
- **Callouts**: For important notes

---

## 🔄 Maintenance

### Adding New Documentation

1. **Choose appropriate folder**:
   - Getting started → `getting-started/`
   - MQTT-related → `mqtt/`
   - VS Code tasks → `vscode/`
   - Telegram bot → `telegram/`

2. **Follow naming convention**:
   - Use descriptive names
   - Use appropriate prefix/suffix
   - Use UPPERCASE for main docs

3. **Update index files**:
   - Add to folder README.md
   - Add to main docs/README.md
   - Update cross-references

4. **Include standard sections**:
   - Introduction
   - Main content
   - Troubleshooting
   - Related documentation
   - External resources

---

## 💡 Best Practices

### For Readers
1. Start with index files (README.md)
2. Use ⭐ marked files for quick start
3. Follow cross-references for deep dives
4. Check troubleshooting sections first

### For Writers
1. Keep files focused on one topic
2. Use consistent formatting
3. Include code examples
4. Add troubleshooting sections
5. Cross-reference related docs
6. Update index files

---

## 🔍 Search Tips

### Find by Topic
- MQTT: Check `mqtt/` folder
- Tasks: Check `vscode/` folder
- Alerts: Check `telegram/` folder
- Setup: Check `getting-started/` folder

### Find by Type
- Quick start: Look for `QUICK_START` or ⭐
- Complete guide: Look for `GUIDE`
- Reference: Look for `REFERENCE`
- Architecture: Look for `ARCHITECTURE`

---

## 📚 External Links

All documentation links to:
- [Main README](../README.md) - Project overview
- [Mosquitto Docs](https://mosquitto.org/documentation/)
- [MQTT Spec](https://mqtt.org/mqtt-specification/)
- [Telegram Bot API](https://core.telegram.org/bots/api)
- [ESP32 Docs](https://docs.espressif.com/projects/esp-idf/)

---

**Last Updated**: April 2026

**Total Documentation**: 19 files, ~100 pages

**Maintained by**: IoT Development Team
