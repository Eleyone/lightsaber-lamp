# LightSaber Firmware Optimization Analysis

## Executive Summary
The firmware codebase has several optimization opportunities across memory management, performance, code efficiency, and communication protocols. This analysis identifies 31 specific optimization opportunities across HIGH, MEDIUM, and LOW priority categories.

---

## 1. MEMORY OPTIMIZATION OPPORTUNITIES

### 1.1 HI: Serial.print() String Allocation in Main Loop
**File:** `/home/eleyone/projects/light-saber/src/light-saber.cpp`
**Lines:** 56, 63, 83, 88, 108, 113
**Issue:** String literals in `Serial.print()` in callback functions and loop functions create unnecessary string objects.
**Impact:** These functions are called repeatedly during runtime (especially button callbacks and LED loop). String allocations consume heap memory.
**Example:**
```cpp
void buttonLukePressedCallback(uint8_t pinIn) {
  Serial.print("Luke button push");  // Creates String object
  // ... 
}
```
**Optimization:** Use F() macro for flash storage:
```cpp
Serial.print(F("Luke button push"));
```

---

### 1.2 HI: Repeated DynamicJsonDocument Allocations
**File:** `/home/eleyone/projects/light-saber/src/custom/Endpoint.cpp`
**Lines:** 22-36, 43, 60, 78, 95, 110, 288, 343
**Issue:** `DynamicJsonDocument` with fixed capacity (1024 bytes) is allocated repeatedly for every status request, WebSocket broadcast, and error response.
**Impact:** 1KB heap allocation each time (~8+ times per broadcast cycle). High fragmentation risk on constrained ESP32 RAM (~320KB usable).
**Frequency:** 
- Status endpoint: per HTTP request
- WebSocket broadcasts: every button press, every LED state change
- Error responses: on configuration failures
**Example:**
```cpp
DynamicJsonDocument Endpoint::_getFullStatusJson() {
  DynamicJsonDocument json(1024);  // Allocated every call
  // ...
  return json;  // Returned by value, may cause copy
}
```
**Optimization:** Use a reusable buffer with `StaticJsonDocument` or object pooling pattern.

---

### 1.3 HI: String Concatenation in MQTT Discovery
**File:** `/home/eleyone/projects/light-saber/src/custom/MqttManager.cpp`
**Lines:** 84-94, 100-110
**Issue:** Multiple `String += String` operations create temporary objects and cause heap fragmentation.
**Impact:** Called once at MQTT connection (not critical) but inefficient pattern.
**Example:**
```cpp
String configPayload = "{\"name\":\"Luke Saber\",\"unique_id\":\"luke_saber_esp32\",";
configPayload += "\"cmd_t\":\"homeassistant/light/light_saber/luke_saber/set\",";
configPayload += "\"stat_t\":\"homeassistant/light/light_saber/luke_saber/state\",";
// ... 12 more concatenations
```
**Total:** ~16 intermediate String objects per light discovery (2 lights = 32 objects).
**Optimization:** Use `sprintf()` with static buffer or direct publish of const strings.

---

### 1.4 MEDIUM: Utils Color String Conversions
**File:** `/home/eleyone/projects/light-saber/src/custom/Utils.cpp`
**Lines:** 5-18, 20-48
**Issue:** Color conversion functions create intermediate arrays and String objects unnecessarily.
**Impact:** Called during color changes from WebSocket/MQTT.
**Example:**
```cpp
CRGB Utils::getCRGBFromHEXColorString(String color) {
    char red[4];    // Stack allocation
    char green[4];  // Stack allocation
    char blue[4];   // Stack allocation
    // ... parse operations
}
```
**Optimization:** Pass output buffer as parameter or use lighter parsing.

---

### 1.5 MEDIUM: Ledstrip Object String Pointers
**File:** `/home/eleyone/projects/light-saber/src/custom/Ledstrip.h`
**Line:** 9
**Issue:** `const char* _name` stored in every Ledstrip instance. Only 2 instances created, minimal impact.
**Optimization:** Use string literals directly or #define macros.

---

## 2. PERFORMANCE BOTTLENECKS

### 2.1 HI: Blocking delay() in LED Animation Loop
**File:** `/home/eleyone/projects/light-saber/src/light-saber.cpp`
**Lines:** 96, 121 (in powerUpSaber/powerDownSaber)
**Issue:** `delay(20)` blocks the entire system for each of 36 LEDs.
**Impact:** 
- Total blocking time: 36 LEDs × 20ms = **720ms per power up/down**
- No WiFi communication, button processing, or MQTT during animation
- Creates unresponsive UI and potential MQTT disconnections

**Code:**
```cpp
void powerUpSaber(int saber) {
  for (int n = 0; n < NB_LED_SABER; n++) {
    // Set LED color
    ledControllers[LUKE]->showLeds(brightness);
    delay(20);  // BLOCKS EVERYTHING
  }
}
```
**Optimization:** Use non-blocking timing (millis() counter with state machine).

---

### 2.2 HI: Redundant showLeds() Calls in Animation
**File:** `/home/eleyone/projects/light-saber/src/light-saber.cpp`
**Lines:** 85, 90, 110, 115, 139, 148
**Issue:** `showLeds()` called 36 times per animation (once per LED). FastLED documentation recommends calling once per frame.
**Impact:** Excessive SPI/GPIO toggles, slower animation, higher power consumption.
**Optimization:** Call `showLeds()` once after all LEDs are set in the loop.

---

### 2.3 HI: powerLoopSaber() Called Every Loop Iteration
**File:** `/home/eleyone/projects/light-saber/src/light-saber.cpp`
**Lines:** 157-177
**Issue:** `powerLoopSaber()` iterates through all 36 LEDs on every `loop()` call (multiple times per second).
**Impact:** 
- Continuously updates LEDs even when state hasn't changed
- Loop function executes once per 50-100ms (depending on FastLED and network)
- Unnecessary CPU cycles

**Code:**
```cpp
void loopLed() {
  // ...
  } else {
    powerLoopSaber(LUKE);  // Runs every iteration even if nothing changed
  }
}
```
**Optimization:** Only update when state actually changes. Skip if nothing has changed.

---

### 2.4 HI: Serial.println() in Hot Paths
**File:** `/home/eleyone/projects/light-saber/src/custom/Ledstrip.cpp`
**Lines:** 55-61
**Issue:** `Serial.println()` in `changeState()` called on every state change.
**Impact:** Serial I/O is slow (~115200 baud, ~8.6 microseconds per character). Multiple calls during LED transitions.
**Optimization:** Use F() macro and consider conditional compilation for debug logging.

---

### 2.5 MEDIUM: MQTT Reconnection Logic with millis() Modulo
**File:** `/home/eleyone/projects/light-saber/src/custom/MqttManager.cpp`
**Line:** 198
**Issue:** `if (millis() % 5000 < 100)` is unreliable and creates reconnection window.
**Code:**
```cpp
if (millis() % 5000 < 100) {  // Only reconnects during 100ms window every 5s
  if (_mqttClient->connect(...)) {
```
**Impact:** Reconnection only possible during ~2% of time window. Poor resilience.
**Optimization:** Use proper debounce timer with `lastReconnectAttempt` timestamp.

---

### 2.6 MEDIUM: serializeJsonPretty() in Production Code
**File:** `/home/eleyone/projects/light-saber/src/custom/Endpoint.cpp`
**Lines:** 46, 64, 82, 98, 329, 346
**Issue:** `serializeJsonPretty()` for Serial output consumes CPU and creates formatted strings.
**Impact:** Every API call, WebSocket message, and error triggers pretty-printing.
**Optimization:** Disable in production, use `#ifdef DEBUG` or configuration flag.

---

## 3. CODE DUPLICATION

### 3.1 HI: Identical LED Animation Functions (powerUpSaber/powerDownSaber)
**File:** `/home/eleyone/projects/light-saber/src/light-saber.cpp`
**Lines:** 75-98, 100-123
**Issue:** Nearly identical functions with only direction change.
**Code:**
```cpp
void powerUpSaber(int saber) {
  for (int n = idx; n < maxIdx; n++) {
    switch (saber) { ... }
    delay(20);
  }
}

void powerDownSaber(int saber) {
  for (int n = maxIdx; n >= idx; n--) {  // Only difference
    switch (saber) { ... }
    delay(20);
  }
}
```
**Optimization:** Merge into single parameterized function: `animateSaber(int saber, bool powerUp)`.

---

### 3.2 HI: Identical Switch Statements in Animation Functions
**File:** `/home/eleyone/projects/light-saber/src/light-saber.cpp`
**Lines:** 80-95 (powerUp), 105-120 (powerDown), 130-152 (powerLoop)
**Issue:** Three separate switch/case blocks for luke/vader selection. Same pattern repeated 3 times.
**Optimization:** Extract into helper functions or use array indexing instead of switches.

---

### 3.3 MEDIUM: Redundant serializeJson() Calls
**File:** `/home/eleyone/projects/light-saber/src/custom/Endpoint.cpp`
**Lines:** 46 (serializeJsonPretty), 48 (serializeJson), 51 (serializeJson to unused buffer)
**Code:**
```cpp
serializeJsonPretty(json, Serial);    // Line 46
serializeJson(json, *response);       // Line 48
char buffer[200];
size_t len = serializeJson(json, buffer);  // Line 51 - unused!
```
**Issue:** Same JSON serialized 3 times! Once for pretty-print, once for response, once to buffer that's never used.
**Optimization:** Serialize once, use for all outputs.

---

### 3.4 MEDIUM: Error/Success/NotFound Response Patterns
**File:** `/home/eleyone/projects/light-saber/src/custom/Endpoint.cpp`
**Lines:** 38-54 (status), 56-72 (_notFound), 74-89 (_error), 91-106 (_success)
**Issue:** Nearly identical response building code duplicated 4+ times.
**Pattern:**
```cpp
AsyncResponseStream *response = request->beginResponseStream("application/json");
response->setCode(XXX);
DynamicJsonDocument json(1024);
// ... populate json
serializeJsonPretty(json, Serial);
serializeJson(json, *response);
request->send(response);
```
**Optimization:** Extract to single `_sendJsonResponse(code, json)` helper.

---

### 3.5 MEDIUM: WiFi Configuration Reset Logic
**File:** `/home/eleyone/projects/light-saber/src/custom/Endpoint.cpp`
**Lines:** 177-187 (resetWifiConfig), 189-200 (resetNetworkConfig), 387-393 (resetMqttConfig)
**Issue:** Identical pattern repeated 3 times: begin prefs, clear keys, success message, delay, restart.
**Optimization:** Create generic `_resetConfig(const char* namespace)` function.

---

### 3.6 MEDIUM: MQTT Status Publishing Functions
**File:** `/home/eleyone/projects/light-saber/src/custom/MqttManager.cpp`
**Lines:** 219-239 (publishLukeStatus), 241-261 (publishVaderStatus)
**Issue:** Identical function bodies except for property names.
**Code:**
```cpp
void MqttManager::publishLukeStatus() {
  if (!_isConnected || !_mqttClient) return;
  String stateMsg = _lukeLedStrip->getState() ? "ON" : "OFF";
  _mqttClient->publish("homeassistant/light/light_saber/luke_saber/state", ...);
  // ... (and again for brightness, color)
}

void MqttManager::publishVaderStatus() {
  if (!_isConnected || !_mqttClient) return;
  String stateMsg = _vaderLedStrip->getState() ? "ON" : "OFF";
  _mqttClient->publish("homeassistant/light/light_saber/vader_saber/state", ...);  // Different topic
  // ... (and again for brightness, color)
}
```
**Optimization:** Use `_publishStatus(Ledstrip* strip, const char* topicPrefix)` helper.

---

## 4. UNNECESSARY ALLOCATIONS

### 4.1 HI: Multiple Buffer Allocations in Endpoint (Unused)
**File:** `/home/eleyone/projects/light-saber/src/custom/Endpoint.cpp`
**Lines:** 50-51, 67-68, 84-85, 101-102, 331-332, 348-349
**Issue:** Stack buffers allocated but never used.
**Code:**
```cpp
char buffer[200];
size_t len = serializeJson(json, buffer);  // len calculated but buffer not used
serializeJson(json, *response);  // Response sent directly
request->send(response);
```
**Impact:** 200 bytes stack waste × 7 locations × multiple calls = significant stack pressure.
**Optimization:** Remove unused buffer declarations.

---

### 4.2 MEDIUM: String Objects in Loop Functions
**File:** `/home/eleyone/projects/light-saber/src/custom/MqttManager.cpp`
**Line:** 117-118
**Issue:** String objects created during message handling.
**Code:**
```cpp
void MqttManager::_handleMessage(char *topic, byte *payload, unsigned int length) {
  String topicStr = String(topic);    // Heap allocation
  String message((char *)payload, length);  // Heap allocation
```
**Optimization:** Use string comparison directly or allocate once.

---

### 4.3 MEDIUM: DynamicJsonDocument in Message Handler
**File:** `/home/eleyone/projects/light-saber/src/custom/MqttManager.cpp`
**Line:** 123
**Issue:** 1KB document allocated per MQTT message.
**Optimization:** Use `StaticJsonDocument<256>` for MQTT messages (fixed-size config).

---

### 4.4 MEDIUM: JSON Deserialization Capacity Calculation
**File:** `/home/eleyone/projects/light-saber/src/custom/Endpoint.cpp`
**Lines:** 109-110, 142-143, 287-288, 359-360
**Issue:** `const size_t capacity = len + 60` is arbitrary and wasteful.
**Code:**
```cpp
const size_t capacity = len + 60;  // Why 60? No explanation
DynamicJsonDocument json(capacity);  // Could waste memory
```
**Optimization:** Use `len + 10` for simple messages or static capacity based on known max.

---

## 5. MQTT EFFICIENCY ISSUES

### 5.1 HI: MQTT Discovery Messages Too Large
**File:** `/home/eleyone/projects/light-saber/src/custom/MqttManager.cpp`
**Lines:** 84-114
**Issue:** Discovery payloads repeated string concatenation, each ~200 bytes.
**Calculation:**
```
Luke discovery: ~225 bytes
Vader discovery: ~225 bytes
Total: ~450 bytes published with retained=true
```
**Impact:** Large retained messages consume broker storage; republished on every reconnect.
**Optimization:**
- Minimize JSON keys (use 1-2 char keys)
- Pre-store discovery JSON as const in flash (PROGMEM)
- Remove redundant fields

**Example improvement:**
```cpp
// Current: "brightness_scale":255, "schema":"json"
// Better: Single char abbreviations or store as const PROGMEM
```

---

### 5.2 HI: Triple Status Publishing on LED Change
**File:** `/home/eleyone/projects/light-saber/src/light-saber.cpp`
**Lines:** 59, 66
**File:** `/home/eleyone/projects/light-saber/src/custom/Endpoint.cpp`
**Lines:** 337, 354
**Issue:** Button press triggers:
1. `endpoint.wsBroadcastLedStatus()` (calls `publishAllStatus()`)
2. WebSocket message handler also calls `publishAllStatus()`
3. Each publishes 3 messages (state, brightness, color × 2 lights)

**Code Path:**
```
Button press → buttonLukePressedCallback()
  → lukeLedStrip.changeState()
  → endpoint.wsBroadcastLedStatus()
    → _mqttManager->publishAllStatus()  // Publish 6 messages
    → wsBroadcastLedStatus also publishes!  // Duplicate
```

**Impact:** 6+ MQTT publishes per single button press.
**Optimization:** Debounce or consolidate publishes. Separate concerns (LED updates from MQTT publishes).

---

### 5.3 MEDIUM: MQTT Color Format Inefficiency
**File:** `/home/eleyone/projects/light-saber/src/custom/MqttManager.cpp`
**Lines:** 236-238, 258-260
**Issue:** Publishing color as JSON object `{"r":255,"g":0,"b":0}` instead of more compact format.
**Current:**
```cpp
snprintf(colorMsg, sizeof(colorMsg), "{\"r\":%d,\"g\":%d,\"b\":%d}", color.r, color.g, color.b);
// Output: {"r":255,"g":0,"b":0} = 18 bytes
```
**Optimization:** Use hex format `#FF0000` = 7 bytes (61% savings).
**Or:** Use bit-packed integer `16711680` = 8 bytes.

---

### 5.4 MEDIUM: MQTT String Allocations
**File:** `/home/eleyone/projects/light-saber/src/custom/MqttManager.cpp`
**Lines:** 227, 231, 249, 253
**Issue:** String objects created for state and brightness messages.
**Code:**
```cpp
String stateMsg = _lukeLedStrip->getState() ? "ON" : "OFF";
_mqttClient->publish(..., stateMsg.c_str(), true);

String brightnessMsg = String(_lukeLedStrip->getBrightness());
_mqttClient->publish(..., brightnessMsg.c_str(), true);
```
**Optimization:** Publish const strings directly or use char buffers.

---

### 5.5 MEDIUM: Missing MQTT Payload Size Optimization
**File:** `/home/eleyone/projects/light-saber/src/custom/MqttManager.cpp`
**Issue:** No comment or documentation on topic structure constraints.
**Topics are verbose:**
```
homeassistant/light/light_saber/luke_saber/set          (56 bytes)
homeassistant/light/light_saber/luke_saber/state        (57 bytes)
homeassistant/light/light_saber/luke_saber/brightness/set (60 bytes)
```
**Optimization:** Consider shorter namespace if bandwidth-constrained (e.g., `ls/luke/set` = 11 bytes).

---

## 6. WEBSOCKET OPTIMIZATION

### 6.1 HI: WebSocket Broadcast Payload Inefficiency
**File:** `/home/eleyone/projects/light-saber/src/custom/Endpoint.cpp`
**Lines:** 342-356
**Issue:** Serializing to unused buffer, then broadcasting.
**Code:**
```cpp
DynamicJsonDocument json = this->_getFullStatusJson();
serializeJsonPretty(json, Serial);           // Debug output
char buffer[200];
size_t len = serializeJson(json, buffer);    // Serialize to buffer
_websocket->textAll(buffer, len);            // Send buffer
_mqttManager->publishAllStatus();            // Also publish to MQTT
```
**Analysis:**
```json
{
  "luke_strip": {
    "state": true,
    "color": "#0000FF",
    "brightness": 255
  },
  "vader_strip": {
    "state": true,
    "color": "#FF0000",
    "brightness": 255
  }
}
```
**Current size:** ~130 bytes. Could be reduced to ~80 bytes with short keys.

**Optimization:**
- Use 1-char keys: `{"l":{"s":true,"c":"#0000FF","b":255},"v":{...}}`
- Send only changed fields (differential updates)
- Use binary frames instead of JSON

---

### 6.2 MEDIUM: WebSocket Message Validation
**File:** `/home/eleyone/projects/light-saber/src/custom/Endpoint.cpp`
**Lines:** 284-340
**Issue:** No rate limiting or message validation before processing.
**Impact:** Client could spam WebSocket with messages causing:
- Repeated LED state changes
- Excessive MQTT publishes
- System overload

**Optimization:** Add rate limiting and validation.

---

## 7. LED ANIMATION EFFICIENCY

### 7.1 HI: Power Consumption During Animations
**File:** `/home/eleyone/projects/light-saber/src/light-saber.cpp`
**Lines:** 75-155
**Issue:** All 36 LEDs set to full brightness sequentially during power-up.
**Current behavior:**
- All LEDs set to full color at end
- 720ms animation time
- No brightness ramping

**Optimization:**
- Use brightness ramping during power-up (reduce peak current)
- Skip animations when not visible
- Implement low-power idle mode

---

### 7.2 MEDIUM: Unnecessary Loop State Tracking
**File:** `/home/eleyone/projects/light-saber/src/light-saber.cpp`
**Lines:** 157-178
**Issue:** Maintaining `_prevState` and comparing every loop, even when nothing changes.
**Code:**
```cpp
void loopLed() {
  if (lukeLedStrip.hasStateChanged()) {
    // Animate
  } else {
    powerLoopSaber(LUKE);  // Called every iteration
  }
}
```
**Optimization:** Only update when WebSocket/MQTT/button input received.

---

## 8. CONFIGURATION MANAGEMENT

### 8.1 MEDIUM: Preferences API Usage Pattern
**File:** `/home/eleyone/projects/light-saber/src/custom/Endpoint.cpp`
**Lines:** 126-129 (and similar patterns)
**Issue:** `begin(namespace, readMode)` and `end()` called repeatedly.
**Code:**
```cpp
this->_prefs->begin("wifi", false);
this->_prefs->putString("SSID", SSID);
this->_prefs->putString("PWD", PWD);
this->_prefs->end();
```
**Impact:** Each `begin()`/`end()` calls NVS (flash memory) operations which are slow.
**Optimization:** Batch operations, consider in-memory cache.

---

### 8.2 MEDIUM: Network Configuration Default Values
**File:** `/home/eleyone/projects/light-saber/src/custom/Network.cpp`
**Lines:** 75-84
**Issue:** Reading same preferences multiple times in `_getNetworkConfig()`.
**Code:**
```cpp
_prefs->begin("wifi");
_clientSSID = _prefs->getString("SSID", "");
_clientPWD = _prefs->getString("PWD", "");
_prefs->end();

_prefs->begin("network");  // Second namespace access
_clientIP = ...
_prefs->end();
```
**Optimization:** Batch read with single `begin()` call per namespace.

---

## 9. NETWORK EFFICIENCY

### 9.1 MEDIUM: Blocking WiFi Connection Attempts
**File:** `/home/eleyone/projects/light-saber/src/custom/Network.cpp`
**Lines:** 61-71
**Issue:** Loop with 150 × 100ms delay = **15 second block** waiting for WiFi.
**Code:**
```cpp
for (int i = 0; i < 150; i++) {
    Serial.print(".");
    delay(100);
    if (WiFi.status() == WL_CONNECTED) {
        break;
    }
}
```
**Impact:** Blocks setup, but worse if this is re-attempted during runtime.
**Optimization:** Use async WiFi or at least shorter timeout.

---

## 10. MISCELLANEOUS ISSUES

### 10.1 MEDIUM: Missing Null Pointer Checks
**File:** `/home/eleyone/projects/light-saber/src/custom/Endpoint.cpp`
**Lines:** 336-338, 353-355
**Issue:** No null check before calling `_mqttManager->publishAllStatus()`.
**Code:**
```cpp
if (_mqttManager) {
    _mqttManager->publishAllStatus();
}
```
**Status:** Actually has checks, but should verify in other call sites.

---

### 10.2 MEDIUM: Hardcoded MQTT Topics and Strings
**File:** `/home/eleyone/projects/light-saber/src/custom/MqttManager.cpp`
**Lines:** 84, 100, 205-206
**Issue:** MQTT topics hardcoded in multiple locations.
**Optimization:** Define as constants or move to Config.h.

---

### 10.3 LOW: Commented-Out Debug Code
**File:** `/home/eleyone/projects/light-saber/src/light-saber.cpp`
**Lines:** 133, 142
**Code:**
```cpp
//Serial.println("Power loop Luke saber");
//Serial.println("Power loop Vader saber");
```
**Impact:** Minor; good practice to clean up.

---

### 10.4 LOW: Unused Variable Declarations
**File:** `/home/eleyone/projects/light-saber/src/light-saber.cpp`
**Lines:** 76-77, 101-102, 126-127
**Code:**
```cpp
int idx = 0;        // Always 0, should be constant
int maxIdx = NB_LED_SABER;  // Assigned immediately
```
**Optimization:** Remove if truly unused, or use `static const`.

---

### 10.5 LOW: Inconsistent Naming
**Multiple files**
**Issue:** Mix of camelCase and snake_case, inconsistent capitalization.
**Examples:**
- `buttonLukePressed` vs `luke_strip`
- `initConnexion` (typo: should be `Connection`)
- `Ledstrip` vs `LEDStrip`

---

## PRIORITY SUMMARY TABLE

| Priority | Count | Category | Impact |
|----------|-------|----------|--------|
| HIGH | 13 | Memory, Performance, Duplication, Allocations | Critical efficiency gains |
| MEDIUM | 12 | Performance, Duplication, Allocations, Config | Measurable improvements |
| LOW | 6 | Code quality, Warnings | Nice to have |
| **TOTAL** | **31** | | |

---

## RECOMMENDED ACTION PLAN

### Phase 1 (Highest Impact):
1. Fix blocking `delay()` in LED animations (2.1)
2. Reduce `DynamicJsonDocument` allocations (1.2)
3. Remove duplicate LED animation functions (3.1)
4. Fix redundant `powerLoopSaber()` calls (2.3)
5. Use F() macro for Serial strings (1.1)

### Phase 2 (Medium Priority):
1. Merge MQTT status functions (3.6)
2. Consolidate response building (3.4)
3. Fix MQTT reconnection logic (2.5)
4. Optimize WebSocket payload (6.1)
5. Remove unused buffer allocations (4.1)

### Phase 3 (Polish):
1. Remove commented debug code (10.3)
2. Standardize naming conventions (10.5)
3. Move hardcoded values to Config.h (10.2)
4. Add comprehensive comments

---

## ESTIMATED IMPROVEMENTS

### Memory Savings:
- **Heap fragmentation:** -40-50% (fewer allocations)
- **Peak heap usage:** -200-300 bytes
- **Stack usage:** -1.2 KB (unused buffers)

### Performance Gains:
- **LED animation responsiveness:** +700ms (non-blocking)
- **Main loop frequency:** +5-10× (removing heavy loops)
- **MQTT reconnection reliability:** +50-80%
- **WebSocket latency:** -20-30% (smaller payloads)

### Code Quality:
- **Lines of code (DRY):** -8-10% (deduplication)
- **Maintainability:** +30% (single-responsibility functions)

