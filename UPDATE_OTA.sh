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
cp app/build/app.css data/
cp app/build/app.js data/
cp app/build/fa.icon.png data/
cp app/build/hotspot.html data/
cp app/build/index.html data/
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

# ElegantOTA : ouvrir la zone flash (avec le MD5 attendu), puis envoyer le binaire en multipart
curl --silent --fail "http://$DEVICE_IP/ota/start?mode=fr&hash=$MD5" > /dev/null && \
curl --silent --fail -F "file=@.pio/build/light-saber/firmware.bin" \
    http://$DEVICE_IP/ota/upload

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
        echo ""

        # Attendre un peu que l'appareil soit complètement prêt
        sleep 2

        # Récupérer les versions
        echo "📋 Version Information:"
        echo "   Récupération des versions depuis le device..."

        FW_INFO=$(curl --silent http://$DEVICE_IP/api/version)
        if [ ! -z "$FW_INFO" ]; then
            FW_VERSION=$(echo $FW_INFO | grep -o '"firmware":"[^"]*"' | cut -d'"' -f4)
            BUILD_DATE=$(echo $FW_INFO | grep -o '"build_date":"[^"]*"' | cut -d'"' -f4)
            echo "   ✅ Firmware: $FW_VERSION"
            echo "   ✅ Build Date: $BUILD_DATE"
        fi

        APP_INFO=$(curl --silent http://$DEVICE_IP/version.json)
        if [ ! -z "$APP_INFO" ]; then
            APP_VERSION=$(echo $APP_INFO | grep -o '"app":"[^"]*"' | cut -d'"' -f4)
            APP_BUILD=$(echo $APP_INFO | grep -o '"buildTime":"[^"]*"' | cut -d'"' -f4)
            echo "   ✅ App: $APP_VERSION"
            echo "   ✅ App Build: $APP_BUILD"
        fi

        echo ""
        echo "✅ Mise à jour complétée avec succès !"
        echo "   Accédez à http://$DEVICE_IP pour utiliser votre appareil"
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
