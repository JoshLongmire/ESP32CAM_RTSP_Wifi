ESP32-CAM HTTP API (Esp32camV8.ino)

This document describes the HTTP API exposed by `Esp32camV8/Esp32camV8.ino` running on an AI‑Thinker ESP32‑CAM.

Base URL examples:
- `http://<device-ip>` (e.g., `http://192.168.1.50`)
- `http://<mdns-hostname>.local` (default `http://esp32camgray.local`)

Authentication:
- Most endpoints are open on the local network.
- OTA Update at `/update` uses HTTP Basic Auth (`OTA_USER` / `OTA_PASS`).

## Recent Improvements (V8)
- Enhanced error handling and input validation
- Improved memory management and streaming performance
- Better WiFi connection handling with detailed status reporting
- Enhanced security with input sanitization and bounds checking
- Added memory monitoring and system diagnostics
- Fixed critical bugs in streaming and network management


Endpoints Overview

| Method | Path          | Description                               | Notes |
|-------:|---------------|-------------------------------------------|-------|
| GET    | `/`           | Main HTML UI                              | Shows live stream, camera controls, Wi‑Fi manager |
| GET    | `/stream`     | MJPEG stream (multipart/x-mixed-replace)  | Continuous JPEG frames until client disconnects |
| GET    | `/capture`    | Capture photo and save to SD              | 204 No Content on success; 503 if camera/SD not ready |
| GET    | `/setCamera`  | Apply camera settings (query parameters)  | Persists to Preferences and redirects to `/` |
| POST   | `/addNetwork` | Add a Wi‑Fi network (form POST)           | Redirects to `/` |
| POST   | `/deleteNetwork` | Delete a saved network                 | Redirects to `/` |
| POST   | `/restart`    | Restart the device                        | Responds 200 then reboots shortly after |
| GET    | `/update`     | OTA update portal                         | Basic Auth required (provided by ElegantOTA) |


Details

GET `/` (Root)
- Renders the web UI which includes:
  - Live stream element displaying `/stream`
  - Camera settings form (submits to `/setCamera`)
  - Saved networks list with delete buttons (posts to `/deleteNetwork`)
  - Add network form (posts to `/addNetwork`)
  - System actions (Restart, OTA Update link)
- Status header shows Station info or AP fallback indicator.


GET `/stream`
- Content-Type: `multipart/x-mixed-replace; boundary=frame`
- Streams continuous JPEG frames until the client disconnects or an OTA begins.
- Enhanced error handling and client connection monitoring
- Notes:
  - If the camera isn't initialized, the handler returns `503 Camera not initialized`.
  - During streaming, frames are acquired via `esp_camera_fb_get()` and returned immediately.
  - Improved client disconnection detection and frame buffer cleanup
  - Better error reporting for incomplete frame transmission


GET `/capture`
- Captures a single JPEG and saves it to the SD card root.
- Filename format: `/<MM-DD-YYYY>_<HH-MM-SS>.jpg` (local time via NTP if available).
- Enhanced error handling with detailed write verification
- Responses:
  - `204 No Content` on success
  - `503 Camera not initialized` if camera failed to init
  - `503 SD not mounted` if SD_MMC is unavailable
  - `503 Camera capture failed` if frame buffer acquisition fails
  - `503 Incomplete file write` if SD write is incomplete
  - `503 File creation failed` if SD file creation fails


GET `/setCamera`
- Applies camera settings and persists them to Preferences under key `camSettings`.
- Enhanced input validation with automatic bounds checking using `constrain()`
- After applying, responds with `302` and `Location: /` (browser returns to main page).
- Query parameters (all optional unless noted):

  Frames and quality
  - `framesize` (required): Enum value
    - `0=QQVGA`, `1=QVGA`, `2=CIF`, `3=VGA`, `4=SVGA`, `5=XGA`, `6=HD`, `7=SXGA`, `8=UXGA`
  - `quality`: JPEG quality (lower is better). Range `4..63`. Default `10`.

  Image adjustments
  - `brightness`: `-2..2` (default `0`)
  - `contrast`: `-2..2` (default `0`)
  - `saturation`: `-2..2` (default `0`)
  - `sharpness`: `-3..3` (default `0`)

  Orientation
  - `hmirror`: present → enable horizontal mirror
  - `vflip`: present → enable vertical flip

  Auto controls
  - `whitebal`: present → enable auto white balance
  - `agc`: present → enable auto gain control
  - `gainceiling`: `0..6`
  - `aec2`: present → enable AEC2
  - `ae_level`: `-2..2`
  - `awb_gain`: present → enable AWB gain

  Effects
  - `effect`: `0..9` where `0=None`, `1=Negative`, `2=Grayscale`, `3=Red Tint`, `4=Green Tint`, `5=Blue Tint`, `6=Sepia`, `7=Film`, `8=Warm`, `9=Cool`.


POST `/addNetwork`
- Content-Type: `application/x-www-form-urlencoded`
- Enhanced input validation and security checks
- Form fields:
  - `ssid` (required, max 32 characters)
  - `password` (required, max 63 characters)
- Behavior:
  - Validates SSID length (1-32 chars) and password length (max 63 chars)
  - If SSID already exists in storage, request is ignored with warning log.
  - On success, responds with `302` and `Location: /`.
- Error handling:
  - Invalid SSID length returns `302` redirect with warning
  - Password too long returns `302` redirect with warning


POST `/deleteNetwork`
- Content-Type: `application/x-www-form-urlencoded`
- Form fields:
  - `ssid` (required)
- Behavior:
  - Removes the saved network if present.
  - Responds with `302` and `Location: /`.


POST `/restart`
- Immediately schedules a restart (~200 ms later).
- Responds with `200 OK` and text `Restarting...`.


GET `/update`
- OTA update portal powered by ElegantOTA.
- Requires HTTP Basic Auth using `OTA_USER` and `OTA_PASS` defined in the sketch.
- On successful OTA, the device will auto‑restart shortly after upload completes.


Persistent Storage (Preferences)
- Namespace: `my-app`
- Keys:
  - `networks`: Packed string of saved networks in the format `ssid:password:localIP;...`
  - `camSettings`: CSV of camera settings values applied via `/setCamera`


Fallback AP Mode
- Enhanced connection logic with better status monitoring and early failure detection
- If no saved network connects, the device starts a Wi‑Fi AP:
  - SSID: `ESP32-CAM-Fallback`
  - Password: `esp32cam`
  - AP IP is printed to Serial (and shown in the UI header)
- Connection attempts now include:
  - Detailed status monitoring during connection attempts
  - Early failure detection for invalid credentials or unavailable networks
  - Signal strength reporting on successful connections
  - Improved timeout handling with proper cleanup


LED Status
- AP mode: flash LED on GPIO 4 is solid ON.
- After connecting to Wi‑Fi: LED blinks 5 times (500 ms ON / 500 ms OFF) as a success indicator.


Response Codes Summary
- `200 OK`: Standard success for actions like `/restart`
- `204 No Content`: Successful capture with no body (`/capture`)
- `302 Found`: Redirect to `/` after POST/GET actions
- `503 Service Unavailable`: Various error conditions:
  - Camera not initialized
  - Camera sensor not available
  - SD not mounted
  - Camera capture failed
  - Incomplete file write
  - File creation failed


cURL Examples

Stream MJPEG to stdout (Ctrl+C to stop):
```bash
curl -v http://esp32camgray.local/stream | cat
```

Capture a photo:
```bash
curl -X GET http://esp32camgray.local/capture -I
```

Set camera settings (VGA, quality 10, horizontal mirror):
```bash
curl "http://esp32camgray.local/setCamera?framesize=3&quality=10&hmirror=1"
```

Add Wi‑Fi network:
```bash
curl -X POST http://esp32camgray.local/addNetwork \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data-urlencode "ssid=MySSID" \
  --data-urlencode "password=MyPassword" -I
```

Delete Wi‑Fi network:
```bash
curl -X POST http://esp32camgray.local/deleteNetwork \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data-urlencode "ssid=MySSID" -I
```

Restart device:
```bash
curl -X POST http://esp32camgray.local/restart -i
```

OTA update (browser):
- Navigate to `http://esp32camgray.local/update`, log in with `OTA_USER` / `OTA_PASS`, and upload a new firmware binary.

## System Diagnostics

The device now includes enhanced logging and monitoring:
- Memory usage monitoring (heap and PSRAM) every 30 seconds
- Detailed startup sequence with emoji-prefixed status messages
- Enhanced WiFi connection status reporting
- Camera initialization status with PSRAM detection
- SD card mount status reporting


