#!/bin/bash
# Checks that the images built by the CI for a commit match a local build of the same commit:
# the firmware must be byte for byte identical, the LittleFS image must hold the same files
# (mklittlefs stores the clock in the image, so its bytes always differ).
# Needs GITEA_URL and GITEA_TOKEN, read from .env when it exists.
# Usage: ci/verify.sh [COMMIT]   (default: HEAD, which must be pushed and built by the CI)
set -euo pipefail

cd "$(dirname "$0")/.."
[ -f .env ] && { set -a; . ./.env; set +a; }
: "${GITEA_URL:?GITEA_URL is not set}" "${GITEA_TOKEN:?GITEA_TOKEN is not set}"
pio=$(command -v pio || echo ~/.platformio/penv/bin/pio)
mklittlefs=~/.platformio/packages/tool-mklittlefs/mklittlefs

commit=$(git rev-parse --short=7 "${1:-HEAD}")
name="light-saber-${commit}"
work=$(mktemp -d)
trap 'git worktree remove --force "$work/src" 2>/dev/null; rm -rf "$work"' EXIT

echo "1️⃣  Téléchargement de l'artefact CI $name..."
api="$GITEA_URL/api/v1/repos/Eleyone/LightSaber/actions/artifacts"
id=$(curl -sf -H "Authorization: token $GITEA_TOKEN" "$api?name=$name" \
    | python3 -I -c 'import json,sys; a=json.load(sys.stdin)["artifacts"]; print(a[0]["id"] if a else "")')
[ -n "$id" ] || { echo "❌ Pas d'artefact $name : le commit est-il poussé et la CI terminée ?"; exit 1; }
mkdir "$work/ci"
curl -sfL -H "Authorization: token $GITEA_TOKEN" -o "$work/ci.zip" "$api/$id/zip"
unzip -q "$work/ci.zip" -d "$work/ci"

echo "2️⃣  Build local de $commit dans un worktree propre..."
git worktree add -q --detach "$work/src" "$commit"
(cd "$work/src/app" && npm ci --silent && npm run build) > "$work/build.log" 2>&1
(cd "$work/src" && "$pio" run --target buildfs --environment light-saber \
    && "$pio" run --environment light-saber) >> "$work/build.log" 2>&1 \
    || { tail -20 "$work/build.log"; echo "❌ Échec du build local"; exit 1; }
local_bin="$work/src/.pio/build/light-saber"

echo "3️⃣  Comparaison..."
status=0
ci_sha=$(sha256sum "$work/ci/light-saber-firmware-${commit}.bin" | cut -d' ' -f1)
local_sha=$(sha256sum "$local_bin/firmware.bin" | cut -d' ' -f1)
if [ "$ci_sha" = "$local_sha" ]; then
    echo "✅ Firmware identique : $ci_sha"
else
    echo "❌ Firmware différent : CI $ci_sha, local $local_sha"
    status=1
fi

unpack() {
    "$mklittlefs" -u "$2" -b 4096 -p 256 -s "$(stat -c %s "$1")" "$1" > /dev/null
}
unpack "$work/ci/light-saber-littlefs-${commit}.bin" "$work/fs-ci"
unpack "$local_bin/littlefs.bin" "$work/fs-local"
if diff -r "$work/fs-ci" "$work/fs-local" > /dev/null; then
    echo "✅ LittleFS : mêmes fichiers ($(ls "$work/fs-ci" | wc -l))"
else
    diff -rq "$work/fs-ci" "$work/fs-local" || true
    echo "❌ LittleFS : contenu différent"
    status=1
fi
exit $status
