#!/bin/bash
set -e

# Build RPM package for Space Captain
# Usage: build-rpm-package.sh <version> <arch> <bin_dir> [git_sha]

VERSION=$1
ARCH=$2
BIN_DIR=$3
GIT_SHA=$4

if [ -z "$VERSION" ] || [ -z "$ARCH" ] || [ -z "$BIN_DIR" ]; then
    echo "Usage: $0 <version> <arch> <bin_dir> [git_sha]"
    exit 1
fi

# If git SHA wasn't provided, it will be included in VERSION already
# This maintains backward compatibility

# Create output directory if it doesn't exist
mkdir -p pkg/out

echo "Creating RPM package version $VERSION for $ARCH"

# Use a temporary directory for rpmbuild
RPMBUILD_DIR=$(mktemp -d)
mkdir -p "$RPMBUILD_DIR"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# Create tarball of the release binaries
SERVER_BINARY=$(readlink -f "${BIN_DIR}/sc-server-release")
if [ ! -f "$SERVER_BINARY" ]; then
    echo "Error: Server binary not found. Run 'make release' first."
    exit 1
fi

# Create a temporary directory for the tarball
TMPDIR=$(mktemp -d)
mkdir -p "$TMPDIR/space-captain-$VERSION/usr/bin"
mkdir -p "$TMPDIR/space-captain-$VERSION/usr/lib"
mkdir -p "$TMPDIR/space-captain-$VERSION/etc/space-captain"
cp "$SERVER_BINARY" "$TMPDIR/space-captain-$VERSION/usr/bin/"

# Copy mbedTLS libraries
cp deps/build/amazon/lib/*.so* "$TMPDIR/space-captain-$VERSION/usr/lib/" 2>/dev/null || true

# Copy certificate files
if [ -f ".secrets/certs/server.crt" ] && [ -f ".secrets/certs/server.key" ]; then
    cp .secrets/certs/server.crt "$TMPDIR/space-captain-$VERSION/etc/space-captain/"
    cp .secrets/certs/server.key "$TMPDIR/space-captain-$VERSION/etc/space-captain/"
else
    echo "Warning: Certificate files not found in .secrets/certs/ directory"
fi

# Copy systemd service file to tarball
cp pkg/rpm/space-captain-server.service "$TMPDIR/space-captain-$VERSION/"

# Create the source tarball
cd "$TMPDIR" && tar czf "$RPMBUILD_DIR/SOURCES/space-captain-$VERSION.tar.gz" "space-captain-$VERSION"
cd - >/dev/null

# Generate spec file from template
DATE=$(date +'%a %b %d %Y')
RELEASE=$(cat .vRELEASE)
sed -e "s/@VERSION@/$VERSION/g" \
    -e "s/@DATE@/$DATE/g" \
    -e "s/@RELEASE@/$RELEASE/g" \
    pkg/rpm/space-captain.spec.template > "$RPMBUILD_DIR/SPECS/space-captain.spec"

# Build the RPM
rpmbuild --define "_topdir $RPMBUILD_DIR" -bb "$RPMBUILD_DIR/SPECS/space-captain.spec"

# Copy the built RPM to pkg/out
cp "$RPMBUILD_DIR"/RPMS/$ARCH/space-captain-server-$VERSION-$RELEASE*.rpm pkg/out/

# Clean up
rm -rf "$TMPDIR"
rm -rf "$RPMBUILD_DIR"

echo "RPM package created in pkg/out/"