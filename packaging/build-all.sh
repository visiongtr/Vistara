#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION=1.2.0
DIST="$ROOT/dist"
mkdir -p "$DIST"
if [[ ! -f "$ROOT/CMakeLists.txt" || ! -f "$ROOT/icons/vistara_icon_256.png" ]]; then
  echo "Run from the actual Vistara source tree with CMakeLists.txt and icons/." >&2; exit 1
fi
# Create a clean snapshot of the current source (never include build/ or dist/).
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/Vistara-$VERSION"
tar -C "$ROOT" --exclude='./build' --exclude='./dist' --exclude='./.git' --exclude='./packaging' -cf - . | tar -C "$TMP/Vistara-$VERSION" -xf -
# Ensure desktop launcher is in the exported source, as the RPM spec uses it.
cat > "$TMP/Vistara-$VERSION/vistara.desktop" <<'DESKTOP'
[Desktop Entry]
Type=Application
Name=Vistara
Comment=Lightweight image viewer
Exec=Vistara %f
Icon=vistara
Terminal=false
Categories=Graphics;Viewer;
MimeType=image/jpeg;image/png;image/webp;image/tiff;image/bmp;image/gif;
StartupNotify=true
DESKTOP
# The source archive explicitly contains version 1.2.0.
sed -i -E 's/(project\(VistaraViewer VERSION )[0-9]+\.[0-9]+\.[0-9]+/\11.2.0/' "$TMP/Vistara-$VERSION/CMakeLists.txt"
tar -C "$TMP" -czf "$DIST/Vistara-$VERSION.tar.gz" "Vistara-$VERSION"
(cd "$TMP" && zip -qr "$DIST/Vistara-$VERSION-source.zip" "Vistara-$VERSION")
echo 'Source archives created in dist/'
# Make sure local CMake version matches packaged version before building binary.
if ! grep -qE 'project\(VistaraViewer VERSION 1\.2\.0' "$ROOT/CMakeLists.txt"; then
  echo 'Update project(VistaraViewer VERSION ...) in CMakeLists.txt to 1.2.0 first.' >&2
  exit 1
fi
bash "$ROOT/packaging/build-deb.sh" "$ROOT" "$VERSION"
if command -v podman >/dev/null; then
  podman run --rm -v "$ROOT:/work:Z" -w /work fedora:44 bash -c '
    set -e
    dnf install -y -q rpm-build cmake cmake-rpm-macros gcc-c++ qt6-qtbase-devel desktop-file-utils zip
    bash packaging/build-rpm.sh
  '
else
  echo 'Podman unavailable: RPM not built. Install podman and rerun this script.' >&2
fi
printf '\nFinal release assets:\n'
find "$DIST" -maxdepth 1 -type f \( -name '*.deb' -o -name '*.rpm' -o -name '*.zip' -o -name '*.tar.gz' \) -printf '%f\n'
