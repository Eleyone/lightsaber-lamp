# Guide de Mise à Jour OTA - Light Saber Lamp

## Mise à Jour Rapide (Automatique - Recommandé)

```bash
./UPDATE_OTA.sh
```

C'est tout! Le script fait tout automatiquement.

---

## Mise à Jour Manuelle (Pas à Pas)

### Prérequis
- Appareil allumé et connecté au WiFi
- Adresse IP de l'appareil (ex: 192.168.3.51)
- Les modifications de code sont commitées

### Étape 1: Compiler le nouveau firmware

```bash
# 1. Aller au dossier du projet
cd /path/to/light-saber

# 2. Compiler l'app web
cd app && npm install && npm run build && cd ..

# 3. Compiler le filesystem LittleFS
pio run --target buildfs --environment light-saber

# 4. Compiler le firmware
pio run --environment light-saber
```

✅ Les fichiers compilés sont maintenant dans `.pio/build/light-saber/`

### Étape 2: Envoyer le firmware via OTA

```bash
# Remplace 192.168.3.51 par l'IP de ton appareil
timeout 60 curl -F "filedata=@.pio/build/light-saber/firmware.bin" \
  http://192.168.3.51/update
```

⏳ **Attends 10-15 secondes** que l'appareil redémarre

### Étape 3: Vérifier la mise à jour

```bash
# Vérifier la nouvelle version
curl http://192.168.3.51/api/version
```

Devrait afficher:
```json
{
  "firmware": "1.2.0",
  "build_date": "Nov 13 2025"
}
```

✅ **Terminé!** La mise à jour est complète.

---

## Dépannage

### ❌ "Connection refused" ou "Timeout"
- Vérifier que l'IP est correcte (regarde le routeur)
- Vérifier que l'appareil est allumé
- Vérifier que l'appareil est connecté au WiFi

### ❌ "500 Internal Server Error"
- L'appareil redémarre peut-être encore
- Attendre 30 secondes et réessayer

### ❌ "Upload stuck ou interrupted"
1. Appuyer sur le bouton RESET de l'appareil
2. Attendre que le WiFi se reconnecte (LEDs clignoteront)
3. Réessayer

### ❌ "Appareil ne répond plus après upload"
1. Éteindre/rallumer l'appareil
2. Vérifier les logs série (si possible): `pio device monitor -p /dev/ttyUSB0`

---

## Commandes Utiles

### Voir la version actuelle
```bash
curl http://192.168.3.51/api/version
```

### Voir la version du web app
```bash
curl http://192.168.3.51/version.json
```

### Redémarrer manuellement l'appareil
(Via la page web settings si disponible, ou débrancher/rebrancher l'alimentation)

### Vérifier la connectivité réseau
```bash
ping 192.168.3.51
```

---

## Script Automatique (UPDATE_OTA.sh)

Le script `UPDATE_OTA.sh` fait tout automatiquement:
1. ✅ Construit l'app web
2. ✅ Construit le filesystem LittleFS
3. ✅ Compile le firmware
4. ✅ Envoie via OTA
5. ✅ Attends le redémarrage
6. ✅ Affiche les nouvelles versions

**Utilisation:**
```bash
# Éditer le script pour changer l'IP si besoin
nano UPDATE_OTA.sh

# Lancer la mise à jour
./UPDATE_OTA.sh
```

---

## Questions/Problèmes?

- Voir `README.md` pour plus de détails
- Voir `CLAUDE.md` pour l'architecture du code
- Vérifier les logs avec: `pio device monitor -p /dev/ttyUSB0 -b 115200`
