# Guide de Mise à Jour OTA - Light Saber Lamp

La lampe se met à jour par le WiFi avec ElegantOTA : page web `http://<ip>/update`, ou en
ligne de commande avec curl. Deux images à envoyer, dans cet ordre :
1. **LittleFS** (`littlefs.bin`) : l'app web ;
2. **Firmware** (`firmware.bin`).

L'appareil redémarre après chacune.

---

## Étape 1 : Récupérer les images

### Depuis la CI (recommandé)

Chaque push sur `main` produit l'artefact `light-saber-<commit>` dans Gitea Actions :
`light-saber-firmware-<commit>.bin`, `light-saber-littlefs-<commit>.bin` et un fichier
`.sha256`. Pour un tag `v*`, ces fichiers sont aussi attachés à la release.

Le build est reproductible : pour vérifier qu'une image CI correspond bien aux sources,

```bash
ci/verify.sh <commit>   # firmware identique à l'octet près, LittleFS : mêmes fichiers
```

(il lit `GITEA_URL` et `GITEA_TOKEN` dans `.env`).

### En local

```bash
cd app && npm install && npm run build && cd ..
pio run --target buildfs --environment light-saber
pio run --environment light-saber
```

Les images sont dans `.pio/build/light-saber/`.

---

## Étape 2 : Envoyer les images

ElegantOTA ouvre d'abord la zone flash avec le MD5 attendu (`/ota/start`), puis reçoit
l'image (`/ota/upload`). `mode=fs` pour LittleFS, `mode=fr` pour le firmware.

```bash
IP=192.168.3.139   # IP de la lampe
upload() {
  curl -sf "http://$IP/ota/start?mode=$1&hash=$(md5sum "$2" | cut -d' ' -f1)" \
    && curl -sf -F "file=@$2" "http://$IP/ota/upload"
}
upload fs .pio/build/light-saber/littlefs.bin
sleep 15   # attendre le redémarrage
upload fr .pio/build/light-saber/firmware.bin
```

Chaque appel doit répondre `OK`.

---

## Étape 3 : Vérifier la mise à jour

```bash
curl http://$IP/api/version   # firmware : {"firmware":"…","build_date":"…","commit_hash":"…"}
curl http://$IP/version.json  # app web
```

Le `commit_hash` doit être celui des images envoyées. La date de build est celle du commit
(en UTC pour le firmware).

---

## Dépannage

### ❌ "Connection refused" ou "Timeout"
- Vérifier que l'IP est correcte (regarde le routeur)
- Vérifier que l'appareil est allumé et connecté au WiFi

### ❌ "500 Internal Server Error"
- L'appareil redémarre peut-être encore
- Attendre 30 secondes et réessayer

### ❌ "Upload stuck ou interrupted"
1. Appuyer sur le bouton RESET de l'appareil
2. Attendre que le WiFi se reconnecte
3. Réessayer

### ❌ "Appareil ne répond plus après upload"
1. Éteindre/rallumer l'appareil
2. Vérifier les logs série (si possible) : `pio device monitor -p /dev/ttyUSB0 -b 115200`

### ⚠️ Table de partitions
Un changement de `partitions.csv` ne passe pas par l'OTA : il faut flasher une fois en USB
(`pio run --target upload --environment light-saber`, puis `--target uploadfs`).

---

## Questions/Problèmes?

- Voir `README.md` pour plus de détails
- Voir `CLAUDE.md` pour l'architecture du code
