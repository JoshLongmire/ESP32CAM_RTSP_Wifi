# 📋 Changelog

All notable changes to the ESP32-CAM project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## 📌 Table of Contents

- [Unreleased](#unreleased)
- [Version 8.0](#version-80---2024)
- [Version 7.0](#version-70---previous-version)
- [How to Use New Features](#how-to-use-new-features)
- [Development Notes](#development-notes)

---

## [Unreleased] 🚧

### ✨ Added
- **Reset to Defaults Feature**: New button in camera settings UI to reset all camera settings to optimal defaults
- **Enhanced Camera Settings UI**: Improved formatting with proper line breaks between controls
- **Smart Quality Selection**: Reset function intelligently chooses optimal JPEG quality based on PSRAM availability
- **Detailed Reset Logging**: Enhanced Serial output showing which settings are being applied during reset

### 🔄 Changed
- **Camera Settings UI Layout**: Fixed formatting issues where controls were running together
- **Reset Timing**: Added proper delays between camera setting applications to ensure settings take effect
- **JavaScript Reset Function**: Added 500ms delay before page reload to allow settings to process

### 🐛 Fixed
- **UI Formatting Issues**: Fixed "Auto White Balance" and "JPEG Quality" labels running together
- **Control Spacing**: Added proper line breaks between all camera control sections
- **Reset Functionality**: Fixed timing issues where reset wasn't applying settings properly
- **Camera Setting Application**: Ensured all default settings are properly applied to camera hardware

### 🔧 Technical Details
- **New Endpoint**: `GET /resetDefaults` - Resets camera settings to optimal defaults
- **Default Settings Applied**:
  - Frame Size: VGA (640x480)
  - JPEG Quality: 10 (with PSRAM) / 12 (without PSRAM)
  - Image Settings: All neutral (brightness: 0, contrast: 0, saturation: 0, sharpness: 0)
  - Auto Controls: Auto white balance, auto gain control, and AWB gain enabled
  - Gain Ceiling: 2X (optimal for most lighting conditions)
  - Effects: None (no special effects)
  - Mirror/Flip: Both disabled

---

## [Version 8.0] - 2024 🎯

### ✨ Added
- Enhanced error handling and input validation
- Improved memory management and streaming performance
- Better WiFi connection handling with detailed status reporting
- Enhanced security with input sanitization and bounds checking
- Added memory monitoring and system diagnostics
- Fixed critical bugs in streaming and network management

### 🚨 Critical Bug Fixes
- **Fixed memory leaks** in streaming function with proper client disconnection handling
- **Fixed buffer overflow** potential in network credential parsing with bounds checking
- **Fixed race condition** in camera settings loading (now loads after camera initialization)
- **Enhanced SD card error handling** with write verification and detailed error reporting
- **Improved WiFi connection handling** with proper status monitoring and early failure detection

### ⚡ Performance Enhancements
- **Optimized memory usage** with PSRAM detection and dual frame buffer support
- **Enhanced streaming performance** with better client connection monitoring
- **Added memory monitoring** with periodic heap/PSRAM reporting every 30 seconds
- **Improved camera initialization** with detailed status reporting and error handling

### 🔒 Security Improvements
- **Input validation** on all API endpoints with automatic bounds checking
- **WiFi credential validation** with proper length limits and duplicate checking
- **Camera parameter sanitization** using constrain() functions for all inputs
- **Enhanced error handling** with specific error codes and detailed reporting

### 📊 System Diagnostics
- **Enhanced logging** with detailed startup sequence and status reporting
- **Memory monitoring** with heap and PSRAM usage tracking
- **WiFi status reporting** with signal strength and connection quality
- **Camera diagnostics** with PSRAM detection and initialization status
- **SD card monitoring** with mount status and operation verification

---

## [Version 7.0] - Previous Version 📜

### 🎯 Features
- Live MJPEG stream at `/stream`
- Photo capture to SD with timestamped filenames
- Camera settings UI with comprehensive controls
- Saved WiFi networks with auto-connect and AP fallback
- OTA update portal with HTTP Basic Auth
- mDNS discovery support
- Enhanced status logging with emoji prefixes

---

## 🚀 How to Use New Features

### 🔄 Reset to Defaults
1. Open the web interface at `http://<device-ip>/` or `http://esp32camshit.local/`
2. Navigate to the "Camera Settings" section
3. Click the "Reset to Defaults" button
4. Confirm the reset in the dialog
5. The page will reload with optimal camera settings applied

### ⚙️ Default Settings Explained
- **VGA Resolution**: Good balance of quality and performance
- **Optimal Quality**: Automatically selected based on your ESP32-CAM's memory configuration
- **Neutral Image Settings**: All adjustments reset to center values for natural colors
- **Auto Controls Enabled**: Automatic white balance and gain control for best image quality
- **No Effects**: Clean, natural image without filters

---

## 👨‍💻 Development Notes

### 📝 Code Standards
- Follow ESP32 Arduino coding standards with 2-space indentation
- Use emoji prefixes in Serial output for better debugging
- Always validate sensor pointers before use
- Implement proper error handling with descriptive messages
- Use `delay()` sparingly in main loop - prefer non-blocking code

### 🧪 Testing
- Test camera reset functionality with different PSRAM configurations
- Verify streaming performance after reset
- Check memory usage during extended operation
- Validate WiFi connection stability after changes

---

## 📚 Additional Resources

- **📖 README.md** - Complete user guide and feature overview
- **🔧 API.md** - Full HTTP API reference with examples
- **🏷️ GitHub Issues** - Report bugs and request features
- **💬 Discussions** - Community support and questions

---

*Last updated: December 2024*
