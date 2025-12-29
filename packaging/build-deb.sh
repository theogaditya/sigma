#!/bin/bash
#
# Build .deb package for Sigma Language
#

set -e

VERSION="${1:-1.1.0}"
ARCH="${2:-amd64}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/build"
PKG_DIR="${BUILD_DIR}/sigma-lang_${VERSION}_${ARCH}"

echo "Building .deb package for sigma-lang v${VERSION} (${ARCH})"

# Ensure binary exists
if [ ! -f "${BUILD_DIR}/bin/sigma" ]; then
    echo "Error: sigma binary not found. Build the project first."
    exit 1
fi

# Create package directory structure
rm -rf "$PKG_DIR"
mkdir -p "${PKG_DIR}/DEBIAN"
mkdir -p "${PKG_DIR}/usr/bin"
mkdir -p "${PKG_DIR}/usr/share/doc/sigma-lang"

# Copy binary
cp "${BUILD_DIR}/bin/sigma" "${PKG_DIR}/usr/bin/sigma"
chmod 755 "${PKG_DIR}/usr/bin/sigma"

# Copy documentation
cp "${PROJECT_ROOT}/README.md" "${PKG_DIR}/usr/share/doc/sigma-lang/"
cp "${PROJECT_ROOT}/LICENSE" "${PKG_DIR}/usr/share/doc/sigma-lang/" 2>/dev/null || true

# Copy control file and update version
sed "s/^Version:.*/Version: ${VERSION}/" "${SCRIPT_DIR}/debian/control" > "${PKG_DIR}/DEBIAN/control"
sed -i "s/^Architecture:.*/Architecture: ${ARCH}/" "${PKG_DIR}/DEBIAN/control"

# Copy post-install script
cp "${SCRIPT_DIR}/debian/postinst" "${PKG_DIR}/DEBIAN/postinst"
chmod 755 "${PKG_DIR}/DEBIAN/postinst"

# Build the package
dpkg-deb --build "$PKG_DIR"

echo ""
echo "Package created: ${PKG_DIR}.deb"
echo ""
echo "Install with: sudo dpkg -i ${PKG_DIR}.deb"
