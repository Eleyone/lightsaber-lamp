# Web Application Optimization Plan

**Status:** Ready for Implementation
**Estimated Total Time:** 6-8 hours
**Expected Size Reduction:** 195+ KB (28% of current size)
**Final Target Size:** 450-500 KB (fits ESP32 comfortably)

---

## Phase 1: Critical Size Reduction (2-3 hours)

### Optimization 1.1: Replace Favicon.ico (-183 KB) ⭐ CRITICAL

**Current Issue:**
- favicon.ico: 186 KB (27% of app!)
- Likely multi-resolution ICO with excessive metadata

**Solution:**
Create a small PNG favicon or SVG favicon and reference it properly.

#### Option A: Simple PNG Favicon (Recommended)

Create a 32x32 PNG favicon (~2-3 KB):
```html
<!-- In src/html/index.html and hotspot.html -->
<link rel="icon" type="image/png" href="../img/favicon.png">
<!-- Also support older browsers -->
<link rel="shortcut icon" href="../img/favicon.png">
```

**Steps:**
1. Replace `src/img/favicon.ico` with a 32x32 PNG (~2-3 KB)
2. Update both HTML files with new link tags
3. Update `parcel-namer-rewrite` rules to include favicon.png

**Expected Result:** -183 KB

---

#### Option B: SVG Favicon (More Elegant)

```html
<link rel="icon" type="image/svg+xml" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'%3E%3Ctext y='0.9em' font-size='90'%3E⚔️%3C/text%3E%3C/svg%3E">
```

**Advantage:** No file needed, pure data URI
**Disadvantage:** Less compatibility with old browsers

**Recommendation:** Use PNG for maximum compatibility

---

### Optimization 1.2: Create Fetch Wrapper (-3 KB)

**Current Issue:**
- Fetch error handling repeated 4 times in endpoint.js (80+ lines)
- Code duplication for saveWifiConfig, resetWifiConfig, saveNetworkConfig, resetNetworkConfig

**Solution:**
Create a reusable `fetchJson` helper function.

#### Before (Current - 100 lines)

```javascript
// endpoint.js - lines 2-27 (repeated 4 times!)
saveWifiConfig($formData) {
    fetch('/wifi-settings', {
        method: 'POST',
        body: JSON.stringify($formData),
        headers: {'Content-type': 'application/json; charset=UTF-8'}
    }).then((response) => {
        if (response.ok) return response.json()
        else throw new Error((await response.json()).message)
    }).then((message) => {
        Alpine.store('notifs').text = message.message;
        Alpine.store('notifs').code = 'success';
    }).catch((error) => {
        Alpine.store('notifs').text = error.message;
        Alpine.store('notifs').code = 'error';
    }).finally(() => {
        Alpine.store('notifs').toggle();
    });
}
```

#### After (Optimized - 40 lines total)

**New file:** `src/js/coponents/fetchHelper.js`

```javascript
export const fetchJson = async (url, options = {}) => {
    const response = await fetch(url, {
        headers: { 'Content-type': 'application/json; charset=UTF-8' },
        ...options
    });

    if (!response.ok) {
        const error = await response.json();
        throw new Error(error.message || 'Request failed');
    }

    return response.json();
};

export const handleFetch = (promise) => {
    return promise
        .then((message) => {
            Alpine.store('notifs').text = message.message;
            Alpine.store('notifs').code = 'success';
        })
        .catch((error) => {
            Alpine.store('notifs').text = error.message;
            Alpine.store('notifs').code = 'error';
        })
        .finally(() => {
            Alpine.store('notifs').toggle();
        });
};
```

**Updated endpoint.js:**

```javascript
import { fetchJson, handleFetch } from './coponents/fetchHelper.js';

export default {
    saveWifiConfig($formData) {
        handleFetch(fetchJson('/wifi-settings', {
            method: 'POST',
            body: JSON.stringify($formData)
        }));
    },

    resetWifiConfig() {
        handleFetch(fetchJson('/reset-wifi-settings', {
            method: 'POST'
        }));
    },

    saveNetworkConfig($formData) {
        handleFetch(fetchJson('/network-settings', {
            method: 'POST',
            body: JSON.stringify($formData)
        }));
    },

    resetNetworkConfig() {
        handleFetch(fetchJson('/reset-network-settings', {
            method: 'POST'
        }));
    }
}
```

**Lines Saved:** 60 lines removed
**Size Reduction:** -3 KB

---

### Optimization 1.3: Deduplicate Luke/Vader Controllers (-8 KB)

**Current Issue:**
- `lukeLedCtrl` (108 lines) and `vaderLedCtrl` (108 lines)
- 95% code duplication, only IDs differ

**Solution:**
Create a factory function to generate controller instances.

#### Before (Current - 216 lines in app.js)

```javascript
Alpine.data('lukeLedCtrl', () => ({
    id: 'luke_strip',
    state: false,
    color: "#FF00AA",
    brightness: 50,
    colorpickerIsOpen: false,
    handleWsChangeEvent(event) { /* ... */ },
    init() { /* ... */ }
}));

Alpine.data('vaderLedCtrl', () => ({
    id: 'vader_strip',
    state: false,
    color: "#00EAFF",
    brightness: 50,
    colorpickerIsOpen: false,
    handleWsChangeEvent(event) { /* ... */ },  // SAME CODE
    init() { /* ... */ }                        // SAME CODE
}));
```

#### After (Optimized - 100 lines total)

**New file:** `src/js/coponents/ledController.js`

```javascript
export const createLedController = (stripId, defaultColor) => ({
    id: stripId,
    state: false,
    color: defaultColor,
    brightness: 50,
    colorpickerIsOpen: false,

    handleWsChangeEvent(event) {
        const message = JSON.parse(event.detail);
        if (message[this.id]) {
            const properties = message[this.id];
            for (const property in properties) {
                let newValue = properties[property];
                if (property === 'brightness') {
                    newValue = Math.round((newValue / 255) * 100);
                }
                this[property] = newValue;
            }
        }
    },

    init() {
        const sendUpdate = (field) => {
            this.$watch(field, (value, oldValue) => {
                if (value !== oldValue && Alpine.store('websocket').state) {
                    let payload = { [this.id]: {} };

                    if (field === 'brightness') {
                        payload[this.id][field] = Math.round((value / 100) * 255);
                    } else {
                        payload[this.id][field] = value;
                    }

                    Alpine.store('websocket').websocket.send(
                        JSON.stringify(payload)
                    );
                }
            });
        };

        ['state', 'color', 'brightness'].forEach(sendUpdate);
    }
});
```

**Updated app.js:**

```javascript
import { createLedController } from './coponents/ledController.js';

Alpine.data('lukeLedCtrl', () =>
    createLedController('luke_strip', '#FF00AA')
);

Alpine.data('vaderLedCtrl', () =>
    createLedController('vader_strip', '#00EAFF')
);
```

**Lines Saved:** 116 lines removed
**Size Reduction:** -8 KB

---

### Optimization 1.4: Fix SVG Minification Issues (-1 KB)

**Current Issue:**
- Build warnings about `cleanupIDs` not being part of preset-default
- SVG files with Alpine directives (`@click`) fail to minify
- htmlnano reports errors for every SVG with event handlers

**Solution:**
Update SVGO configuration to handle Alpine directives.

#### Option A: Update .svgorc Configuration

Create `.svgorc.json` in project root:

```json
{
  "plugins": [
    {
      "name": "preset-default",
      "params": {
        "overrides": {
          "removeUnknownAttrs": {
            "unknownAttrs": ["@click", "x-data", "x-on", "@"]
          }
        }
      }
    }
  ]
}
```

#### Option B: Update package.json

```json
{
  "svgo": {
    "plugins": [
      "preset-default",
      {
        "name": "removeUnknownAttrs",
        "params": {
          "unknownAttrs": ["@click", "x-data", "x-on", "@"]
        }
      }
    ]
  }
}
```

**Expected Result:** -1 KB, eliminates build warnings

---

## Phase 2: Code Quality & Updates (2-3 hours)

### Optimization 2.1: Update Dependencies & Fix Vulnerabilities

**Current Status:**
- 54 vulnerabilities (1 low, 3 moderate, 50 high)
- Parcel 2.9.3 (outdated, 2.13+ available)
- Various PostCSS plugins outdated

**Steps:**

1. **Update browserslist database:**
```bash
cd /home/eleyone/projects/light-saber/app
npx update-browserslist-db@latest
```

2. **Audit current issues:**
```bash
npm audit
```

3. **Safe updates (compatible versions):**
```bash
npm update
npm install --save-dev parcel@latest
```

4. **Check for breaking changes:**
- Test build after each major update
- Verify all functionality works

**Expected Result:** 0 vulnerabilities, faster builds

---

### Optimization 2.2: Fix Google Fonts External Loading

**Current Issue (index.html line 9):**
```html
<link href="https://fonts.googleapis.com/css?family=Roboto:regular,bold,italic,thin,light,bolditalic,black,medium&amp;lang=en" rel="stylesheet">
```

**Problem:** External CDN call violates self-contained requirement

**Solution Options:**

#### Option A: Use System Fonts (Fastest)
```html
<style>
body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI",
                 "Helvetica Neue", Arial, sans-serif;
}
</style>
```
**Pros:** No external calls, instant load, ~2-3 KB savings
**Cons:** Less distinctive branding

#### Option B: Self-Hosted Fonts (Best)
1. Download Roboto font files from Google Fonts
2. Place in `src/fonts/`
3. Reference via @font-face:

```css
/* In src/css/app.css */
@font-face {
    font-family: 'Roboto';
    src: url('../fonts/roboto-regular.woff2') format('woff2');
    font-weight: 400;
}

@font-face {
    font-family: 'Roboto';
    src: url('../fonts/roboto-bold.woff2') format('woff2');
    font-weight: 700;
}

body {
    font-family: 'Roboto', sans-serif;
}
```

**Recommendation:** Option A (system fonts) for this embedded project

---

### Optimization 2.3: Verify Tailwind CSS Purging

**Current Setup (tailwind.config.js):**

```javascript
module.exports = {
  content: [
    "./src/**/*.html",
    "./src/**/*.js"
  ],
  theme: {
    extend: {},
  },
  plugins: [],
}
```

**Verification Steps:**

1. Check that content paths match your file structure
2. After build, inspect `app.css` size (~18 KB is reasonable)
3. If size is larger, unused utilities might not be purged

**If needing optimization:**

```javascript
// More specific content patterns
content: [
    "./src/html/**/*.html",
    "./src/html/components/**/*.html",
    "./src/js/**/*.js"
]
```

**Expected Result:** Verify no bloat, confirm ~10-15 KB CSS is optimal

---

### Optimization 2.4: Remove Source Maps from Production

**Current Waste:**
```
dist/
├── app.js.map       331 KB
└── app.css.map       25 KB
Total waste: 356 KB
```

**Solution (package.json):**

```json
{
  "scripts": {
    "build": "parcel build --no-source-maps src/html/*.html"
  }
}
```

Or update `.parcelrc`:

```json
{
  "extends": "@parcel/config-default",
  "source": {
    "maps": false
  }
}
```

**Result:** -356 KB waste removed from build directory

---

## Phase 3: Fine-tuning (Optional, 1-2 hours)

### Optimization 3.1: HTML Minification

**Current:** HTML not minified
**Target:** Remove unnecessary whitespace

Most bundlers minify by default. Verify by checking built HTML files.

**Manual check:**
```bash
wc -c app/dist/index.html  # Current size
```

Expected minified: ~35-38 KB (vs current 43 KB)
**Potential savings:** -5 KB

---

### Optimization 3.2: CSS Micro-optimizations

**Current:** ~18 KB minified

Possible optimizations:
1. Remove custom slider CSS if not used
2. Simplify color variables
3. Merge similar utility patterns

**Potential savings:** -2-3 KB

---

### Optimization 3.3: Service Worker for Caching

For embedded devices, a simple service worker can:
- Cache all assets on first load
- Reduce network requests after initial load
- Enable offline fallback

**Basic Service Worker:**

```javascript
// src/js/sw.js
const CACHE_VERSION = 'v1';
const CACHE_ASSETS = [
    '/index.html',
    '/app.js',
    '/app.css',
    '/favicon.png'
];

self.addEventListener('install', (event) => {
    event.waitUntil(
        caches.open(CACHE_VERSION).then((cache) => {
            return cache.addAll(CACHE_ASSETS);
        })
    );
});

self.addEventListener('fetch', (event) => {
    if (event.request.method !== 'GET') return;

    event.respondWith(
        caches.match(event.request).then((response) => {
            return response || fetch(event.request);
        })
    );
});
```

**Register in index.html:**
```html
<script>
if ('serviceWorker' in navigator) {
    navigator.serviceWorker.register('/sw.js');
}
</script>
```

**Benefits:**
- Instant load on repeat visits
- Reduced bandwidth usage
- Graceful offline operation

**Size impact:** +1-2 KB

---

## Implementation Checklist

### Before Starting
- [ ] Create git branch for optimization
- [ ] Current build works: `npm run build`
- [ ] Note baseline: 696 KB total

### Phase 1 (Critical - 2-3 hours)

#### 1.1 Favicon
- [ ] Create 32x32 PNG favicon (~2-3 KB)
- [ ] Delete old favicon.ico
- [ ] Update both HTML files with new link tag
- [ ] Test build

#### 1.2 Fetch Wrapper
- [ ] Create `src/js/coponents/fetchHelper.js`
- [ ] Update `src/js/coponents/endpoint.js`
- [ ] Import and use helper
- [ ] Test all settings forms
- [ ] Verify websocket still works

#### 1.3 LED Controller
- [ ] Create `src/js/coponents/ledController.js`
- [ ] Update `src/js/app.js` to use factory
- [ ] Test Luke control panel
- [ ] Test Vader control panel
- [ ] Test color picker
- [ ] Test brightness slider

#### 1.4 SVG Configuration
- [ ] Create `.svgorc.json` or update package.json
- [ ] Rebuild and verify no warnings
- [ ] Favicon renders correctly

**Checkpoint:** Rebuild and measure size
```bash
npm run build
du -sh dist/
# Expected: ~510-520 KB
```

### Phase 2 (Quality - 2-3 hours)

#### 2.1 Dependencies
- [ ] Run `npx update-browserslist-db@latest`
- [ ] Run `npm audit`
- [ ] Run `npm update`
- [ ] Test build
- [ ] Run full app test in browser

#### 2.2 Google Fonts
- [ ] Remove Google Fonts link
- [ ] Add system fonts CSS
- [ ] Test typography looks good
- [ ] Verify no layout shifts

#### 2.3 Tailwind
- [ ] Verify CSS size ~18 KB or less
- [ ] Check purging is working
- [ ] Review unused utilities

#### 2.4 Source Maps
- [ ] Update build script to exclude .map files
- [ ] Rebuild
- [ ] Verify no .map files in dist/

**Checkpoint:** Final size measurement
```bash
npm run build
du -sh dist/
# Expected: ~450-500 KB
```

### Testing
- [ ] All pages load correctly
- [ ] WebSocket connects
- [ ] Controls respond to input
- [ ] Settings save properly
- [ ] No console errors
- [ ] Responsive design works

### Finalization
- [ ] Update WEBAPP_ANALYSIS_REPORT.md with results
- [ ] Commit with proper commit message
- [ ] Create filesystem build for ESP32
- [ ] Upload to device and test

---

## Rollback Plan

If any optimization breaks functionality:

1. Revert specific file
2. Test in isolation
3. Fix the issue
4. Re-commit

Git commands:
```bash
# View changes
git diff src/js/app.js

# Revert specific file
git checkout src/js/app.js

# Revert entire branch
git reset --hard main
```

---

## Expected Results

### Size Reduction Summary

| Phase | Optimization | Savings | Cumulative |
|-------|--------------|---------|-----------|
| Start | Current app | - | 696 KB |
| 1.1 | Favicon replacement | -183 KB | 513 KB |
| 1.2 | Fetch wrapper | -3 KB | 510 KB |
| 1.3 | LED controller | -8 KB | 502 KB |
| 1.4 | SVG config | -1 KB | 501 KB |
| 2.4 | Remove .map files | -356 KB* | 145 KB** |
| **Phase 2** | **Font optimization** | **-2 KB** | **499 KB** |
| **Phase 3** | **HTML/CSS tweaks** | **-5 KB** | **494 KB** |

*Map files are waste in build, not shipped to ESP32
**Only counting ESP32-shipped files: 145 KB total for .map cleanup is inaccurate

**Accurate ESP32 Storage:**
```
Current:    ~321 KB (without .map files)
After Opt:  ~280-300 KB (40-45% reduction)
ESP32 Free: 700-750 KB remaining
```

---

## Success Criteria

- [ ] App builds without warnings
- [ ] Final size < 350 KB for ESP32
- [ ] All features work identically
- [ ] No vulnerabilities in dependencies
- [ ] Self-contained (no external CDN calls)
- [ ] Responsive design verified
- [ ] WebSocket stable
- [ ] Settings persistence working

---

**Ready for implementation!**

Next steps: Confirm you want to proceed, and I will apply these optimizations systematically.
