ESP32-CAM Web Stream + Wi‑Fi Manager + OTA (Esp32camV7)

This project provides a lightweight web interface for the AI‑Thinker ESP32‑CAM with live MJPEG streaming, camera controls, Wi‑Fi network management, SD capture, and OTA firmware updates.


Features
- Live MJPEG stream at `/stream`
- Photo capture to SD (`/capture`), timestamped filenames
- Camera settings UI (framesize, brightness, contrast, saturation, white balance, quality, mirror/flip, AGC/AEC2, AE level, AWB gain, effects, sharpness)
- Saved Wi‑Fi networks (add/delete), auto‑connect on boot with AP fallback
- OTA update portal at `/update` (HTTP Basic Auth)
- mDNS discovery (`http://esp32cam2.local` by default)
- Status logging over Serial with emoji prefixes


Hardware
- Module: AI‑Thinker ESP32‑CAM (OV2640)
- Flash LED: GPIO 4
- SD Card: `SD_MMC` interface
- Board selection in Arduino IDE: `AI Thinker ESP32‑CAM`


Project Structure
- `Esp32camV7/Esp32camV7.ino` — main sketch
- `API.md` — HTTP API reference


Quick Start
1) Install ESP32 support
- Arduino IDE: Boards Manager → install “ESP32 by Espressif Systems”.

2) Libraries
- `ElegantOTA` (install via Library Manager) — used for OTA portal.
- All other includes are provided by the ESP32 core (`esp_camera`, `WiFi`, `WebServer`, `Preferences`, `SD_MMC`, `ESPmDNS`).

3) Configure
- Open `Esp32camV7/Esp32camV7.ino` and update:
  - `MDNS_HOSTNAME` — device hostname (lowercase)
  - `OTA_USER` and `OTA_PASS` — credentials for OTA portal

4) Build & Flash
- Board: `AI Thinker ESP32‑CAM`
- Port: your device’s COM port
- Flash sketch normally. First boot will attempt Wi‑Fi connections from saved list (empty on first run).


Using the Web Interface
- Open `http://<device-ip>/` or `http://<hostname>.local/` (default `http://esp32cam2.local/`).
- Live stream appears under “Live Stream”.
- Adjust camera settings and click “Apply & Save” (persists to Preferences).
- “Capture & Save” takes a photo (requires SD card) and saves it to `/` on the SD card.
- Manage Wi‑Fi networks in “Saved Networks” and “Add Network”.
- Use “OTA Update” to access the update portal; use your configured credentials.


Wi‑Fi Behavior
- On boot, the device iterates saved networks and tries to connect (10‑second timeout per network).
- On successful connect:
  - Serial shows SSID and local IP.
  - LED on GPIO 4 blinks 5 times as a success indicator.
  - The connected network’s IP is stored with that SSID.
- If none connect:
  - Fallback AP mode starts with SSID `ESP32-CAM-Fallback`, password `esp32cam`.
  - LED stays ON while in AP mode.


mDNS
- If mDNS starts successfully, the device registers HTTP service on port 80.
- Access via `http://<MDNS_HOSTNAME>.local/` (defaults to `esp32cam2`).


OTA Updates
- Visit `http://<device>/update`.
- Authenticate with `OTA_USER` / `OTA_PASS` (configured in the sketch).
- Upload the compiled binary; upon success, the device will reboot automatically.


API Reference
- See `API.md` for full details and cURL examples for all endpoints.


Serial Logs
- The sketch uses `Serial.printf()` with emoji prefixes for quick scanning:
  - 🔄 ongoing actions (e.g., OTA progress, connect attempts)
  - ✅ successes
  - ⚠️ warnings (non‑critical)
  - ❌ critical errors
  - 🌐 network info
  - 📸 camera operations
  - 🗑️ deletions
  - 🌟 status indicators
  - 🚀 startup complete


Troubleshooting
- Camera init failed (`❌ Camera init failed`):
  - Power the module with a stable 5V/≥1A supply.
  - Ensure the camera ribbon cable is fully seated.
  - Reduce framesize/quality if memory constrained; confirm PSRAM is available.

- SD_MMC mount failed (`⚠️ SD_MMC Mount Failed`):
  - Use a formatted microSD (FAT/FAT32), insert firmly before boot.
  - Try a different card; some cards are unreliable on ESP32‑CAM.

- No Wi‑Fi connection:
  - Add a network via the web UI (AP mode SSID `ESP32-CAM-Fallback`, pass `esp32cam`).
  - Verify credentials and signal strength; relocate closer to the router.

- Stream stutters:
  - Lower framesize or increase JPEG quality value (worse compression) to reduce CPU.
  - Close extra browser tabs/clients; multiple viewers increase load.

- Capture doesn’t save:
  - Requires SD card mounted. Check Serial for errors.


Security Notes
- Change `OTA_USER` and `OTA_PASS` before deploying.
- Keep the device on a trusted network. API is intentionally simple and unauthenticated (except OTA) for ease of use.


License
- Provided as‑is for personal/educational use. Adapt as needed for your project.


