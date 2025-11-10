#!/usr/bin/env node

/**
 * Build script to generate version.json with app version
 * This file is generated during npm run build
 */

import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// Read package.json to get version
const packageJsonPath = path.join(__dirname, 'package.json');
const packageJson = JSON.parse(fs.readFileSync(packageJsonPath, 'utf8'));

// Create version object
const versionInfo = {
    app: packageJson.version,
    buildDate: new Date().toISOString(),
    buildTime: new Date().toLocaleString()
};

// Write to data directory (will be embedded in LittleFS)
const dataDir = path.join(__dirname, '..', 'data');
if (!fs.existsSync(dataDir)) {
    fs.mkdirSync(dataDir, { recursive: true });
}

const versionJsonPath = path.join(dataDir, 'version.json');
fs.writeFileSync(versionJsonPath, JSON.stringify(versionInfo, null, 2), 'utf8');

console.log(`✓ Generated version.json`);
console.log(`  App version: ${versionInfo.app}`);
console.log(`  Build date: ${versionInfo.buildTime}`);
