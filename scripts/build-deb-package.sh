#!/bin/bash
set -e

# Build Debian package for Space Captain
# Usage: build-deb-package.sh <version> <arch> <bin_dir>

VERSION=$1
ARCH=$2
BIN_DIR=$3

if [ -z "$VERSION" ] || [ -z "$ARCH" ] || [ -z "$BIN_DIR" ]; then
    echo "Usage: $0 <version> <arch> <bin_dir>"
    exit 1
fi

RELEASE=$(cat .vRELEASE)
PKG_NAME="space-captain-server_${VERSION}-${RELEASE}_${ARCH}"
PKG_DIR="/tmp/space-captain-server-${VERSION}-${RELEASE}-${ARCH}"

# Create output directory if it doesn't exist
mkdir -p pkg/out

echo "Creating package: pkg/out/${PKG_NAME}.deb"

# Clean up any previous build
rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR/DEBIAN"
mkdir -p "$PKG_DIR/usr/bin"
mkdir -p "$PKG_DIR/usr/lib/systemd/system"
mkdir -p "$PKG_DIR/usr/lib/space-captain"
mkdir -p "$PKG_DIR/etc/space-captain"

# Find and copy the versioned server binary
SERVER_BINARY=$(readlink -f "${BIN_DIR}/sc-server-release")
if [ ! -f "$SERVER_BINARY" ]; then
    echo "Error: Server binary not found. Run 'make release' first."
    exit 1
fi
cp "$SERVER_BINARY" "$PKG_DIR/usr/lib/space-captain/"
chmod 755 "$PKG_DIR/usr/lib/space-captain/$(basename "$SERVER_BINARY")"

# Copy mbedTLS libraries if they exist (for custom builds)
if [ -d "deps/build/debian/lib" ]; then
    cp deps/build/debian/lib/*.so* "$PKG_DIR/usr/lib/space-captain/" 2>/dev/null || true
fi

# Copy certificate files
if [ -f "certs/server.crt" ] && [ -f "certs/server.key" ]; then
    cp certs/server.crt "$PKG_DIR/etc/space-captain/"
    cp certs/server.key "$PKG_DIR/etc/space-captain/"
    chmod 644 "$PKG_DIR/etc/space-captain/server.crt"
    chmod 600 "$PKG_DIR/etc/space-captain/server.key"
else
    echo "Warning: Certificate files not found in certs/ directory"
fi

# Create wrapper script
cat > "$PKG_DIR/usr/bin/space-captain-server" << 'EOF'
#!/bin/bash
export LD_LIBRARY_PATH=/usr/lib/space-captain:$LD_LIBRARY_PATH
exec /usr/lib/space-captain/sc-server-* "$@"
EOF
chmod 755 "$PKG_DIR/usr/bin/space-captain-server"

# Generate control file from template
RELEASE=$(cat .vRELEASE)
sed -e "s/@VERSION@/$VERSION/" \
    -e "s/@ARCH@/$ARCH/" \
    -e "s/@RELEASE@/$RELEASE/" \
    pkg/deb/control.template > "$PKG_DIR/DEBIAN/control"

# Copy post-install script
cp pkg/deb/postinst "$PKG_DIR/DEBIAN/"
chmod 755 "$PKG_DIR/DEBIAN/postinst"

# Copy pre-removal script
cp pkg/deb/prerm "$PKG_DIR/DEBIAN/"
chmod 755 "$PKG_DIR/DEBIAN/prerm"

# Copy conffiles list
cp pkg/deb/conffiles "$PKG_DIR/DEBIAN/"

# Copy systemd service file
cp pkg/deb/space-captain-server.service "$PKG_DIR/usr/lib/systemd/system/"

# Build the package
dpkg-deb --build "$PKG_DIR" "pkg/out/${PKG_NAME}.deb"

# Clean up
rm -rf "$PKG_DIR"

echo "Package created: pkg/out/${PKG_NAME}.deb"