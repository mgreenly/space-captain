#!/bin/bash
set -e

# Build Debian package for Space Captain
# Usage: build-deb-package.sh <version> <arch> <bin_dir> [git_sha] [os_dir]

VERSION=$1
ARCH=$2
BIN_DIR=$3
GIT_SHA=$4
OS_DIR=${5:-debian}  # Default to debian for backward compatibility

if [ -z "$VERSION" ] || [ -z "$ARCH" ] || [ -z "$BIN_DIR" ]; then
    echo "Usage: $0 <version> <arch> <bin_dir> [git_sha] [os_dir]"
    exit 1
fi

# If git SHA wasn't provided, it will be included in VERSION already
# This maintains backward compatibility

RELEASE=$(cat .vREL)
PKG_NAME="space-captain-server_${VERSION}-${RELEASE}_${ARCH}"
PKG_DIR="/tmp/space-captain-server-${VERSION}-${RELEASE}-${ARCH}"

# Create output directory if it doesn't exist
mkdir -p pkg/${OS_DIR}/out

echo "Creating package: pkg/${OS_DIR}/out/${PKG_NAME}.deb"

# Clean up any previous build
rm -rf "$PKG_DIR"
mkdir -p "$PKG_DIR/DEBIAN"
mkdir -p "$PKG_DIR/usr/bin"
mkdir -p "$PKG_DIR/usr/lib/systemd/system"
mkdir -p "$PKG_DIR/usr/lib/space-captain"
mkdir -p "$PKG_DIR/etc/space-captain"

# Find and copy the versioned server binary
SERVER_BINARY=$(readlink -f "${BIN_DIR}/${OS_DIR}/sc-server-release")
if [ ! -f "$SERVER_BINARY" ]; then
    echo "Error: Server binary not found. Run 'make release' first."
    exit 1
fi
cp "$SERVER_BINARY" "$PKG_DIR/usr/lib/space-captain/"
chmod 755 "$PKG_DIR/usr/lib/space-captain/$(basename "$SERVER_BINARY")"

# Strip RPATH from the binary for security and portability
BINARY_PATH="$PKG_DIR/usr/lib/space-captain/$(basename "$SERVER_BINARY")"
echo "Checking RPATH in binary..."
if chrpath -l "$BINARY_PATH" 2>/dev/null | grep -q "RPATH\|RUNPATH"; then
    echo "Found RPATH/RUNPATH, stripping it..."
    chrpath -d "$BINARY_PATH"
    echo "RPATH stripped successfully"
else
    echo "No RPATH/RUNPATH found in binary"
fi

# Copy mbedTLS libraries if they exist (for custom builds)
if [ -d "deps/build/${OS_DIR}/lib" ]; then
    cp deps/build/${OS_DIR}/lib/*.so* "$PKG_DIR/usr/lib/space-captain/" 2>/dev/null || true
fi

# Copy certificate files
if [ -f ".secrets/certs/server.crt" ] && [ -f ".secrets/certs/server.key" ]; then
    cp .secrets/certs/server.crt "$PKG_DIR/etc/space-captain/"
    cp .secrets/certs/server.key "$PKG_DIR/etc/space-captain/"
    chmod 644 "$PKG_DIR/etc/space-captain/server.crt"
    chmod 600 "$PKG_DIR/etc/space-captain/server.key"
else
    echo "Warning: Certificate files not found in .secrets/certs/ directory"
fi

# Create wrapper script
cat > "$PKG_DIR/usr/bin/space-captain-server" << 'EOF'
#!/bin/bash
export LD_LIBRARY_PATH=/usr/lib/space-captain:$LD_LIBRARY_PATH
exec /usr/lib/space-captain/sc-server-* "$@"
EOF
chmod 755 "$PKG_DIR/usr/bin/space-captain-server"

# Generate control file from template
RELEASE=$(cat .vREL)
sed -e "s/@VERSION@/$VERSION/" \
    -e "s/@ARCH@/$ARCH/" \
    -e "s/@RELEASE@/$RELEASE/" \
    pkg/${OS_DIR}/templates/control.template > "$PKG_DIR/DEBIAN/control"

# Copy post-install script
cp pkg/${OS_DIR}/templates/postinst "$PKG_DIR/DEBIAN/" 2>/dev/null || true
chmod 755 "$PKG_DIR/DEBIAN/postinst"

# Copy pre-removal script
cp pkg/${OS_DIR}/templates/prerm "$PKG_DIR/DEBIAN/" 2>/dev/null || true
chmod 755 "$PKG_DIR/DEBIAN/prerm"

# Copy conffiles list
cp pkg/${OS_DIR}/templates/conffiles "$PKG_DIR/DEBIAN/" 2>/dev/null || true

# Copy systemd service file
cp pkg/${OS_DIR}/templates/space-captain-server.service "$PKG_DIR/usr/lib/systemd/system/" 2>/dev/null || true

# Build the package
dpkg-deb --build "$PKG_DIR" "pkg/${OS_DIR}/out/${PKG_NAME}.deb"

# Clean up
rm -rf "$PKG_DIR"

echo "Package created: pkg/${OS_DIR}/out/${PKG_NAME}.deb"