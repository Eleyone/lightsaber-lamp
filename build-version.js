#!/usr/bin/env node

/**
 * Build Version Generator
 * - Generates build metadata (hash, timestamp)
 * - Captures Git commit hash
 * - Increments web app patch version
 * This script runs during npm build process
 */

const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const { execSync } = require('child_process');

// Get directories
const buildDir = process.env.BUILD_DIR || '.pio/build/light-saber';
const appSrcDir = path.join(__dirname, 'app', 'src');
const packageJsonPath = path.join(__dirname, 'app', 'package.json');
const dataDir = path.join(__dirname, 'data');

/**
 * Get Git commit hash
 */
function getGitCommitHash() {
  try {
    const hash = execSync('git rev-parse --short HEAD', { encoding: 'utf-8' }).trim();
    return hash;
  } catch (err) {
    console.warn('Warning: Could not get Git commit hash:', err.message);
    return 'unknown';
  }
}

/**
 * Increment patch version in package.json
 */
function incrementPatchVersion() {
  try {
    const packageJson = JSON.parse(fs.readFileSync(packageJsonPath, 'utf-8'));
    const [major, minor, patch] = packageJson.version.split('.').map(Number);
    const newPatch = patch + 1;
    const newVersion = `${major}.${minor}.${newPatch}`;

    packageJson.version = newVersion;
    fs.writeFileSync(packageJsonPath, JSON.stringify(packageJson, null, 2) + '\n');

    console.log(`[Build] Incremented web app version: ${major}.${minor}.${patch} -> ${newVersion}`);
    return newVersion;
  } catch (err) {
    console.warn('Warning: Could not increment patch version:', err.message);
    return null;
  }
}

// Get Git commit hash
const commitHash = getGitCommitHash();
console.log(`[Build] Current commit hash: ${commitHash}`);

// Increment patch version
const newVersion = incrementPatchVersion();

// Build info object
const buildInfo = {
  app: newVersion || JSON.parse(fs.readFileSync(packageJsonPath, 'utf-8')).version,
  timestamp: new Date().toISOString(),
  buildDate: new Date().toISOString(),
  date: new Date().toLocaleDateString('en-US', { year: 'numeric', month: 'short', day: '2-digit' }),
  time: new Date().toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit', second: '2-digit', hour12: false }),
  commit_hash: commitHash,
  unixTimestamp: Math.floor(Date.now() / 1000),
};

// Generate version string for display
buildInfo.versionDisplay = `${buildInfo.date} ${buildInfo.time}`;

// Output build info as JSON for reference
console.log('\n✓ Generated version information:');
console.log(JSON.stringify(buildInfo, null, 2));

// Save version.json to both src and data directories (for distribution)
const versionJsonPath = path.join(appSrcDir, 'version.json');
const versionJsonDataPath = path.join(dataDir, 'version.json');

try {
  fs.writeFileSync(versionJsonPath, JSON.stringify(buildInfo, null, 2));
  console.log(`✓ Saved version.json to ${versionJsonPath}`);
} catch (err) {
  console.warn('Warning: Could not save version.json:', err.message);
}

try {
  fs.writeFileSync(versionJsonDataPath, JSON.stringify(buildInfo, null, 2));
  console.log(`✓ Saved version.json to ${versionJsonDataPath}`);
} catch (err) {
  console.warn('Warning: Could not save version.json to data:', err.message);
}
