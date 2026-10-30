#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
command -v rpmbuild >/dev/null || { echo 'Install rpm-build and required Qt6 build dependencies first.' >&2; exit 1; }
TOP="${HOME}/rpmbuild"
mkdir -p "$TOP"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
cp "$ROOT/Vistara-1.1.0.tar.gz" "$TOP/SOURCES/"
cp "$ROOT/packaging/vistara.spec" "$TOP/SPECS/"
rpmbuild -ba "$TOP/SPECS/vistara.spec"
mkdir -p "$ROOT/dist"
find "$TOP/RPMS" "$TOP/SRPMS" -type f -name '*vistara*1.1.0*.rpm' -exec cp -v {} "$ROOT/dist/" \;
echo "RPM packages copied to $ROOT/dist"
