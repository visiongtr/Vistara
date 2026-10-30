#!/usr/bin/env bash
set -euo pipefail

PROJECT="${1:-/mnt/Projects/Vistara}"
VERSION="${2:-1.2.0}"
OUTPUT="$PROJECT/dist"
BUILD="$PROJECT/build/Package-Release"

if [[ ! -f "$PROJECT/CMakeLists.txt" ]]; then
  echo "Error: CMakeLists.txt not found at $PROJECT" >&2
  exit 1
fi
for command in cmake dpkg-deb dpkg-shlibdeps; do
  command -v "$command" >/dev/null || { echo "Missing command: $command" >&2; exit 1; }
done

mkdir -p "$BUILD" "$OUTPUT"
printf '\n==> Configuring release build\n'
cmake -S "$PROJECT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
printf '\n==> Compiling Vistara\n'
cmake --build "$BUILD" --parallel "$(nproc)"

EXE="$BUILD/Vistara"
if [[ ! -x "$EXE" ]]; then
  echo "Could not find executable at $EXE" >&2
  exit 1
fi

STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT
mkdir -p "$STAGE/DEBIAN" "$STAGE/usr/bin" "$STAGE/usr/share/applications" "$STAGE/usr/share/icons/hicolor/256x256/apps"
install -m755 "$EXE" "$STAGE/usr/bin/vistara"
install -m644 "$PROJECT/icons/vistara_icon_256.png" "$STAGE/usr/share/icons/hicolor/256x256/apps/vistara.png"
cat > "$STAGE/usr/share/applications/vistara.desktop" <<'DESKTOP'
[Desktop Entry]
Type=Application
Name=Vistara
Comment=Lightweight GPU-assisted image viewer
Exec=vistara %f
Icon=vistara
Terminal=false
Categories=Graphics;Viewer;
MimeType=image/jpeg;image/png;image/webp;image/tiff;image/bmp;image/gif;
StartupNotify=true
DESKTOP

# Ask Debian's installed libraries for the right runtime dependencies.
# dpkg-shlibdeps expects a debian/control file in its working directory.
mkdir -p "$STAGE/debian"
cat > "$STAGE/debian/control" <<'CONTROL'
Source: vistara
Section: graphics
Priority: optional
Maintainer: Vistara Project <noreply@example.invalid>
Standards-Version: 4.6.2

Package: vistara
Architecture: any
Description: Lightweight image viewer
CONTROL
pushd "$STAGE" >/dev/null
SHLIBS="$(dpkg-shlibdeps -O -eusr/bin/vistara)" || { echo "dpkg-shlibdeps failed" >&2; exit 1; }
popd >/dev/null
DEPENDS="${SHLIBS#shlibs:Depends=}"
if [[ "$DEPENDS" == "$SHLIBS" || -z "$DEPENDS" ]]; then
  echo "Could not determine runtime dependencies: $SHLIBS" >&2
  exit 1
fi
# Qt may load these dynamically; they are not necessarily ELF-linked.
DEPENDS="$DEPENDS, libqt6openglwidgets6"
cat > "$STAGE/DEBIAN/control" <<CONTROL
Package: vistara
Version: $VERSION
Section: graphics
Priority: optional
Architecture: $(dpkg --print-architecture)
Maintainer: Vistara Project <noreply@example.invalid>
Depends: $DEPENDS
Description: Vistara lightweight image viewer
 A simple Qt image viewer with GPU-assisted rendering,
 fast navigation, zooming and non-destructive rotation saving.
CONTROL
rm -rf "$STAGE/debian"
PACKAGE="$OUTPUT/vistara_${VERSION}_$(dpkg --print-architecture).deb"
dpkg-deb --build --root-owner-group "$STAGE" "$PACKAGE"
printf '\nCreated: %s\n' "$PACKAGE"
printf 'Install on another compatible LMDE machine with:\n  sudo apt install ./vistara_%s_%s.deb\n' "$VERSION" "$(dpkg --print-architecture)"
