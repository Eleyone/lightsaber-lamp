#!/bin/bash

# Configuration
DEVICE_IP="192.168.3.51"
DEVICE_PORT="3232"

echo "=========================================="
echo "Light Saber - Mise à jour OTA complète"
echo "=========================================="
echo "IP: $DEVICE_IP:$DEVICE_PORT"
echo ""

# Étape 1: Compiler la web app
echo "1️⃣  Compilation de la web app..."
cd app
npm run build
if [ $? -ne 0 ]; then
    echo "❌ Erreur lors de la compilation de l'app"
    exit 1
fi
cd ..
echo "✅ Web app compilée"
echo ""

# Étape 2: Copier les fichiers compilés vers data/
echo "2️⃣  Copie des fichiers compilés..."
mkdir -p data
rm -f data/app.css data/app.js data/fa.icon.png data/hotspot.html data/index.html
cp dist/app.css data/
cp dist/app.js data/
cp dist/fa.icon.png data/
cp dist/hotspot.html data/
cp dist/index.html data/
echo "✅ Fichiers copiés vers data/"
echo ""

# Étape 3: Rebuilder le filesystem LittleFS
echo "3️⃣  Construction du filesystem LittleFS..."
/home/eleyone/.platformio/penv/bin/platformio run --target buildfs --environment light-saber
if [ $? -ne 0 ]; then
    echo "❌ Erreur lors de la construction du filesystem"
    exit 1
fi
echo "✅ Filesystem construit"
echo ""

# Étape 4: Upload du firmware + app via OTA
echo "4️⃣  Upload firmware + app via OTA..."
echo "   (L'appareil va redémarrer automatiquement)"

# Calculer le hash MD5
MD5=$(md5sum .pio/build/light-saber/firmware.bin | cut -d' ' -f1)

# Uploader le firmware via HTTP POST
curl --silent --data-binary "@.pio/build/light-saber/firmware.bin" \
    -H "Content-Type: application/octet-stream" \
    -d "MD5=$MD5" \
    http://$DEVICE_IP/update

if [ $? -eq 0 ]; then
    echo ""
    echo "=========================================="
    echo "✅ Upload complété, attente du redémarrage..."
    echo "=========================================="
    echo ""

    # Attendre le redémarrage
    echo "Attente de 15 secondes pour le redémarrage..."
    sleep 15

    # Vérifier que l'appareil est revenu
    if ping -c 1 $DEVICE_IP > /dev/null 2>&1; then
        echo "✅ L'appareil est de retour en ligne!"
        echo "✅ Mise à jour complétée avec succès !"
    else
        echo "⚠️  L'appareil n'a pas répondu au ping. Attendez quelques secondes et vérifiez la connexion."
    fi
else
    echo ""
    echo "=========================================="
    echo "❌ Erreur lors de l'upload OTA"
    echo "=========================================="
    echo ""
    echo "Vérifiez:"
    echo "  - L'appareil est connecté au WiFi"
    echo "  - L'IP $DEVICE_IP est correcte"
    echo "  - La connexion réseau est stable"
    exit 1
fi
