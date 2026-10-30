#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VER="1.2.0"
command -v rpmbuild >/dev/null || { echo 'Missing rpmbuild' >&2; exit 1; }
TOP="${HOME}/rpmbuild"
mkdir -p "$TOP"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
cp "$ROOT/dist/Vistara-${VER}.tar.gz" "$TOP/SOURCES/"
cp "$ROOT/packaging/vistara.spec" "$TOP/SPECS/"
rpmbuild -ba "$TOP/SPECS/vistara.spec"
mkdir -p "$ROOT/dist"
find "$TOP/RPMS" -type f -name "vistara-${VER}-*.x86_64.rpm" -exec cp -v {} "$ROOT/dist/" \;
echo "RPM packages copied to $ROOT/dist"
