#!/bin/bash
# Script to install full kernel with 9p support in the VM
# This replaces the cloud kernel with a full kernel that includes 9p modules

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Space Captain VM Kernel Upgrade${NC}"
echo "=================================="
echo ""
echo "This script will install the full Debian kernel to enable workspace mounting."
echo ""

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}Please run as root (use sudo)${NC}"
    exit 1
fi

# Check current kernel
CURRENT_KERNEL=$(uname -r)
echo "Current kernel: $CURRENT_KERNEL"

# Check if already running full kernel
if [[ ! "$CURRENT_KERNEL" =~ cloud ]]; then
    echo -e "${GREEN}Already running a full kernel!${NC}"
    
    # Try to load 9p modules
    if modprobe -n 9p 2>/dev/null; then
        echo "9p modules are available"
        exit 0
    else
        echo -e "${YELLOW}Warning: 9p modules not found in current kernel${NC}"
    fi
fi

# Install full kernel
echo ""
echo "Installing full kernel package..."
apt-get update
apt-get install -y linux-image-amd64

# Get the newly installed kernel version
NEW_KERNEL=$(dpkg -l | grep linux-image | grep -v cloud | grep -v meta | awk '{print $2}' | sed 's/linux-image-//' | sort -V | tail -1)

echo ""
echo -e "${GREEN}Full kernel installed: linux-image-$NEW_KERNEL${NC}"

# Update GRUB to prefer the new kernel
echo ""
echo "Updating boot configuration..."

# Create GRUB config to set default kernel
cat > /etc/default/grub.d/50-cloudimg-settings.cfg << EOF
# Prefer full kernel over cloud kernel
GRUB_DEFAULT="Advanced options for Debian GNU/Linux>Debian GNU/Linux, with Linux $NEW_KERNEL"
GRUB_DISABLE_SUBMENU=y
EOF

# Update GRUB
update-grub

echo ""
echo -e "${GREEN}Boot configuration updated${NC}"
echo ""
echo "The VM needs to be rebooted to use the new kernel."
echo ""
echo -e "${YELLOW}After reboot, the workspace will be automatically mounted at /workspace${NC}"
echo ""
echo "Reboot now? (y/n)"
read -r response

if [[ "$response" =~ ^[Yy]$ ]]; then
    echo "Rebooting..."
    reboot
else
    echo "Please reboot manually when ready: sudo reboot"
fi