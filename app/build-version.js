#!/usr/bin/env node

/**
 * Build Version Generator
 * - Generates build metadata (hash, timestamp)
 * - Captures Git commit hash
 * - Increments web app patch version
 * This script runs during npm build process
 */

import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import { execSync } from 'child_process';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// Get directories
const projectRoot = path.join(__dirname, '..');
const packageJsonPath = path.join(__dirname, 'package.json');
const dataDir = path.join(projectRoot, 'data');

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
  buildDate: new Date().toISOString(),
  buildTime: new Date().toLocaleString(),
  commit_hash: commitHash,
};

// Generate version string for display
buildInfo.versionDisplay = `${buildInfo.buildTime}`;

// Output build info as JSON for reference
console.log('\n✓ Generated version information:');
console.log(JSON.stringify(buildInfo, null, 2));

// Save version.json to data directory (for distribution)
const versionJsonDataPath = path.join(dataDir, 'version.json');

try {
  fs.mkdirSync(dataDir, { recursive: true });
} catch (err) {
  // Directory might already exist, that's fine
}

try {
  fs.writeFileSync(versionJsonDataPath, JSON.stringify(buildInfo, null, 2));
  console.log(`✓ Saved version.json to ${versionJsonDataPath}`);
} catch (err) {
  console.warn('Warning: Could not save version.json to data:', err.message);
}
