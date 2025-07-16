#!/bin/bash
set -e

# Install Space Captain
# Usage: install.sh <prefix> <bin_dir> <os_dir>

PREFIX=$1
BIN_DIR=$2
OS_DIR=$3

if [ -z "$PREFIX" ] || [ -z "$BIN_DIR" ] || [ -z "$OS_DIR" ]; then
    echo "Usage: $0 <prefix> <bin_dir> <os_dir>"
    exit 1
fi

echo "Installing Space Captain to $PREFIX"
echo "======================================="

# Create directory structure
install -d "$PREFIX/bin"
install -d "$PREFIX/lib/space-captain"
install -d "$PREFIX/etc/space-captain"

# Find the versioned binaries
SERVER_VERSIONED=$(basename $(readlink -f "$BIN_DIR/sc-server-release"))
CLIENT_VERSIONED=$(basename $(readlink -f "$BIN_DIR/sc-client-release"))

if [ ! -f "$BIN_DIR/$SERVER_VERSIONED" ]; then
    echo "Error: Server binary not found. Run 'make release' first."
    exit 1
fi

if [ ! -f "$BIN_DIR/$CLIENT_VERSIONED" ]; then
    echo "Error: Client binary not found. Run 'make release' first."
    exit 1
fi

echo "Installing server binary: $SERVER_VERSIONED"
install -m 755 "$BIN_DIR/$SERVER_VERSIONED" "$PREFIX/lib/space-captain/$SERVER_VERSIONED"

echo "Installing client binary: $CLIENT_VERSIONED"
install -m 755 "$BIN_DIR/$CLIENT_VERSIONED" "$PREFIX/lib/space-captain/$CLIENT_VERSIONED"

# Install mbedTLS libraries
echo "Installing mbedTLS libraries..."
if [ -d "deps/build/$OS_DIR/lib" ]; then
    cp -P deps/build/$OS_DIR/lib/*.so* "$PREFIX/lib/space-captain/" 2>/dev/null || true
fi

# Install certificates
echo "Installing certificates..."
if [ -f ".secrets/certs/server.crt" ] && [ -f ".secrets/certs/server.key" ]; then
    install -m 644 .secrets/certs/server.crt "$PREFIX/etc/space-captain/"
    install -m 600 .secrets/certs/server.key "$PREFIX/etc/space-captain/"
else
    echo "Warning: Certificate files not found. Run 'make certs' first."
fi

# Create wrapper scripts
echo "Creating wrapper scripts..."

# Server wrapper
cat > "$PREFIX/bin/space-captain-server" << EOF
#!/bin/bash
export LD_LIBRARY_PATH=$PREFIX/lib/space-captain:\$LD_LIBRARY_PATH
exec $PREFIX/lib/space-captain/sc-server-* "\$@"
EOF
chmod 755 "$PREFIX/bin/space-captain-server"

# Client wrapper
cat > "$PREFIX/bin/space-captain-client" << EOF
#!/bin/bash
export LD_LIBRARY_PATH=$PREFIX/lib/space-captain:\$LD_LIBRARY_PATH
exec $PREFIX/lib/space-captain/sc-client-* "\$@"
EOF
chmod 755 "$PREFIX/bin/space-captain-client"

echo ""
echo "Installation complete!"
echo "  Binaries:      $PREFIX/lib/space-captain/"
echo "  Wrappers:      $PREFIX/bin/space-captain-{server,client}"
echo "  Certificates:  $PREFIX/etc/space-captain/"
echo "  Libraries:     $PREFIX/lib/space-captain/"
echo ""
echo "Add $PREFIX/bin to your PATH to use the commands."