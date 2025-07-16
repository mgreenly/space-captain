#!/bin/bash
set -e

# Uninstall Space Captain
# Usage: uninstall.sh <prefix>

PREFIX=$1

if [ -z "$PREFIX" ]; then
    echo "Usage: $0 <prefix>"
    exit 1
fi

echo "Uninstalling Space Captain from $PREFIX"
echo "========================================="
echo ""
echo "This will remove:"
[ -f "$PREFIX/bin/space-captain-server" ] && echo "  - $PREFIX/bin/space-captain-server" || true
[ -f "$PREFIX/bin/space-captain-client" ] && echo "  - $PREFIX/bin/space-captain-client" || true
[ -d "$PREFIX/lib/space-captain" ] && echo "  - $PREFIX/lib/space-captain/" || true
[ -d "$PREFIX/etc/space-captain" ] && echo "  - $PREFIX/etc/space-captain/" || true
echo ""

# Check if running in non-interactive mode (e.g., from CI/CD)
if [ -t 0 ]; then
    # Interactive mode - ask for confirmation
    echo -n "Continue with uninstall? [y/N] "
    read REPLY
    if [ "$REPLY" != "y" ] && [ "$REPLY" != "Y" ]; then
        echo ""
        echo "Uninstall cancelled."
        exit 0
    fi
else
    # Non-interactive mode - proceed without confirmation
    echo "Running in non-interactive mode, proceeding with uninstall..."
fi

echo ""

# Remove wrapper scripts
if [ -f "$PREFIX/bin/space-captain-server" ]; then
    echo "Removing wrapper script: $PREFIX/bin/space-captain-server"
    rm -f "$PREFIX/bin/space-captain-server"
fi

if [ -f "$PREFIX/bin/space-captain-client" ]; then
    echo "Removing wrapper script: $PREFIX/bin/space-captain-client"
    rm -f "$PREFIX/bin/space-captain-client"
fi

# Remove binaries and libraries
if [ -d "$PREFIX/lib/space-captain" ]; then
    echo "Removing binaries and libraries: $PREFIX/lib/space-captain/"
    rm -rf "$PREFIX/lib/space-captain"
fi

# Remove certificates
if [ -d "$PREFIX/etc/space-captain" ]; then
    echo "Removing certificates: $PREFIX/etc/space-captain/"
    rm -rf "$PREFIX/etc/space-captain"
fi

# Clean up empty directories if they exist
rmdir "$PREFIX/lib" 2>/dev/null || true
rmdir "$PREFIX/etc" 2>/dev/null || true
rmdir "$PREFIX/bin" 2>/dev/null || true

echo ""
echo "Uninstall complete!"