#!/usr/bin/env node

/**
 * Build Version Generator
 * Generates build metadata (hash, timestamp) to be embedded in the firmware
 * This script runs during PlatformIO build process
 */

const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

// Get build directory from environment or use default
const buildDir = process.env.BUILD_DIR || '.pio/build/light-saber';
const srcDir = path.join(__dirname, 'src');

// Build info object
const buildInfo = {
  timestamp: new Date().toISOString(),
  date: new Date().toLocaleDateString('en-US', { year: 'numeric', month: 'short', day: '2-digit' }),
  time: new Date().toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit', second: '2-digit', hour12: false }),
  unixTimestamp: Math.floor(Date.now() / 1000),
};

// Try to compute firmware hash if binary exists
const firmwarePath = path.join(buildDir, 'firmware.bin');
if (fs.existsSync(firmwarePath)) {
  try {
    const fileBuffer = fs.readFileSync(firmwarePath);
    buildInfo.hash = crypto.createHash('md5').update(fileBuffer).digest('hex');
  } catch (err) {
    console.warn('Warning: Could not compute firmware hash:', err.message);
    buildInfo.hash = '0000000000000000';
  }
} else {
  // Hash not available yet (firmware not built)
  buildInfo.hash = '0000000000000000';
}

// Generate version string for display
buildInfo.versionDisplay = `${buildInfo.date} ${buildInfo.time}`;

// Output build info as JSON for reference
console.log('Build Information:');
console.log(JSON.stringify(buildInfo, null, 2));

// Save to file that can be used by firmware if needed
const buildInfoPath = path.join(srcDir, 'BuildInfo.json');
try {
  fs.writeFileSync(buildInfoPath, JSON.stringify(buildInfo, null, 2));
  console.log(`Build info saved to ${buildInfoPath}`);
} catch (err) {
  console.warn('Warning: Could not save BuildInfo.json:', err.message);
}
