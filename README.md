ESP32-CAM Web Stream + Wi‑Fi Manager + OTA (Esp32camV8)

This project provides a robust and enhanced web interface for the AI‑Thinker ESP32‑CAM with live MJPEG streaming, camera controls, Wi‑Fi network management, SD capture, and OTA firmware updates. Version 8 includes significant improvements in reliability, security, and performance.


Features
- Live MJPEG stream at `/stream` with enhanced error handling
- Photo capture to SD (`/capture`), timestamped filenames with write verification
- Camera settings UI (framesize, brightness, contrast, saturation, white balance, quality, mirror/flip, AGC/AEC2, AE level, AWB gain, effects, sharpness)
- Saved Wi‑Fi networks (add/delete), auto‑connect on boot with AP fallback
- OTA update portal at `/update` (HTTP Basic Auth)
- mDNS discovery (`http://esp32camgray.local` by default)
- Enhanced status logging over Serial with emoji prefixes
- Memory monitoring and system diagnostics
- Input validation and security enhancements
- Improved WiFi connection handling with detailed status reporting


Hardware
- Module: AI‑Thinker ESP32‑CAM (OV2640)
- Flash LED: GPIO 4
- SD Card: `SD_MMC` interface
- Board selection in Arduino IDE: `AI Thinker ESP32‑CAM`


Project Structure
- `Esp32camV8/Esp32camV8.ino` — main sketch (enhanced version)
- `API.md` — HTTP API reference with V8 improvements


Quick Start
1) Install ESP32 support
- Arduino IDE: Boards Manager → install “ESP32 by Espressif Systems”.

2) Libraries
- `ElegantOTA` (install via Library Manager) — used for OTA portal.
- All other includes are provided by the ESP32 core (`esp_camera`, `WiFi`, `WebServer`, `Preferences`, `SD_MMC`, `ESPmDNS`).

3) Configure
- Open `Esp32camV8/Esp32camV8.ino` and update:
  - `MDNS_HOSTNAME` — device hostname (lowercase, default: "esp32camgray")
  - `OTA_USER` and `OTA_PASS` — credentials for OTA portal (change from defaults!)

4) Build & Flash
- Board: `AI Thinker ESP32‑CAM`
- Port: your device’s COM port
- Flash sketch normally. First boot will attempt Wi‑Fi connections from saved list (empty on first run).


Using the Web Interface
- Open `http://<device-ip>/` or `http://<hostname>.local/` (default `http://esp32camgray.local/`).
- Live stream appears under "Live Stream" with enhanced error handling.
- Adjust camera settings and click "Apply & Save" (persists to Preferences with input validation).
- "Capture & Save" takes a photo (requires SD card) and saves it to `/` on the SD card with write verification.
- Manage Wi‑Fi networks in "Saved Networks" and "Add Network" with enhanced validation.
- Use "OTA Update" to access the update portal; use your configured credentials.
- Monitor system status through enhanced Serial logging with emoji prefixes.


Wi‑Fi Behavior
- Enhanced connection logic with detailed status monitoring and early failure detection
- On boot, the device iterates saved networks and tries to connect (10‑second timeout per network).
- On successful connect:
  - Serial shows SSID, local IP, and signal strength.
  - LED on GPIO 4 blinks 5 times as a success indicator.
  - The connected network's IP is stored with that SSID.
- Connection attempts now include:
  - Detailed status monitoring during connection attempts
  - Early failure detection for invalid credentials or unavailable networks
  - Signal strength reporting on successful connections
  - Improved timeout handling with proper cleanup
- If none connect:
  - Fallback AP mode starts with SSID `ESP32-CAM-Fallback`, password `esp32cam`.
  - LED stays ON while in AP mode.


mDNS
- If mDNS starts successfully, the device registers HTTP service on port 80.
- Access via `http://<MDNS_HOSTNAME>.local/` (defaults to `esp32camgray`).


OTA Updates
- Visit `http://<device>/update`.
- Authenticate with `OTA_USER` / `OTA_PASS` (configured in the sketch).
- Upload the compiled binary; upon success, the device will reboot automatically.


API Reference
- See `API.md` for full details and cURL examples for all endpoints.


Serial Logs
- Enhanced logging system with detailed status reporting and system diagnostics:
  - 🔄 ongoing actions (e.g., OTA progress, connect attempts)
  - ✅ successes
  - ⚠️ warnings (non‑critical)
  - ❌ critical errors
  - 🌐 network info
  - 📸 camera operations
  - 🗑️ deletions
  - 🌟 status indicators
  - 🚀 startup complete
  - 💾 memory monitoring (every 30 seconds)
- New diagnostic features:
  - PSRAM detection and reporting
  - Memory usage monitoring (heap and PSRAM)
  - Enhanced WiFi connection status with signal strength
  - Camera initialization status reporting
  - SD card mount status reporting


Troubleshooting
- Camera init failed (`❌ Camera init failed`):
  - Power the module with a stable 5V/≥1A supply.
  - Ensure the camera ribbon cable is fully seated.
  - Reduce framesize/quality if memory constrained; confirm PSRAM is available.
  - Check Serial logs for specific error codes and PSRAM detection status.

- SD_MMC mount failed (`⚠️ SD_MMC Mount Failed`):
  - Use a formatted microSD (FAT/FAT32), insert firmly before boot.
  - Try a different card; some cards are unreliable on ESP32‑CAM.
  - Enhanced error reporting now shows specific failure reasons.

- No Wi‑Fi connection:
  - Add a network via the web UI (AP mode SSID `ESP32-CAM-Fallback`, pass `esp32cam`).
  - Verify credentials and signal strength; relocate closer to the router.
  - Enhanced connection logs now show detailed status and failure reasons.

- Stream stutters or disconnections:
  - Improved client disconnection detection and frame buffer management.
  - Lower framesize or increase JPEG quality value (worse compression) to reduce CPU.
  - Close extra browser tabs/clients; multiple viewers increase load.
  - Check Serial logs for streaming error messages.

- Capture doesn't save:
  - Requires SD card mounted. Enhanced error reporting shows specific failure reasons.
  - Check Serial logs for detailed write verification results.
  - New validation ensures complete file writes before reporting success.

- Memory issues:
  - Monitor memory usage through Serial logs (reported every 30 seconds).
  - PSRAM detection status is reported at startup.
  - Consider reducing frame size or quality if memory is constrained.


Security Notes
- Change `OTA_USER` and `OTA_PASS` before deploying (defaults are insecure!).
- Enhanced input validation and bounds checking on all API endpoints.
- WiFi credential validation with length limits (SSID: 32 chars max, Password: 63 chars max).
- Camera parameter validation with automatic bounds checking.
- Keep the device on a trusted network. API is intentionally simple and unauthenticated (except OTA) for ease of use.


## Version 8 Improvements

### Critical Bug Fixes
- **Fixed memory leaks** in streaming function with proper client disconnection handling
- **Fixed buffer overflow** potential in network credential parsing with bounds checking
- **Fixed race condition** in camera settings loading (now loads after camera initialization)
- **Enhanced SD card error handling** with write verification and detailed error reporting
- **Improved WiFi connection handling** with proper status monitoring and early failure detection

### Performance Enhancements
- **Optimized memory usage** with PSRAM detection and dual frame buffer support
- **Enhanced streaming performance** with better client connection monitoring
- **Added memory monitoring** with periodic heap/PSRAM reporting every 30 seconds
- **Improved camera initialization** with detailed status reporting and error handling

### Security Improvements
- **Input validation** on all API endpoints with automatic bounds checking
- **WiFi credential validation** with proper length limits and duplicate checking
- **Camera parameter sanitization** using constrain() functions for all inputs
- **Enhanced error handling** with specific error codes and detailed reporting

### System Diagnostics
- **Enhanced logging** with detailed startup sequence and status reporting
- **Memory monitoring** with heap and PSRAM usage tracking
- **WiFi status reporting** with signal strength and connection quality
- **Camera diagnostics** with PSRAM detection and initialization status
- **SD card monitoring** with mount status and operation verification

License
- Provided as‑is for personal/educational use. Adapt as needed for your project.


