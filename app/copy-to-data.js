#!/usr/bin/env node

/**
 * Copy built files from build/ to ../data/ directory for LittleFS embedding
 * This runs after the Parcel build completes
 */

import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const buildDir = path.join(__dirname, 'build');
const dataDir = path.join(__dirname, '..', 'data');

// Ensure data directory exists
if (!fs.existsSync(dataDir)) {
    fs.mkdirSync(dataDir, { recursive: true });
}

// Copy all files from build/ to data/
const files = fs.readdirSync(buildDir);
files.forEach(file => {
    const src = path.join(buildDir, file);
    const dest = path.join(dataDir, file);

    if (fs.statSync(src).isFile()) {
        fs.copyFileSync(src, dest);
        console.log(`✓ Copied ${file} to ../data/`);
    }
});

console.log(`\n✓ All files copied to LittleFS data directory`);
