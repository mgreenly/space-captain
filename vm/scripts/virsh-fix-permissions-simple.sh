#!/bin/bash
set -e

# Load configuration
source "$(dirname "$0")/../config.sh"

echo "Fixing permissions for libvirt access (simple method)..."

# Option 1: Add ACL permissions for libvirt-qemu user
echo "Adding ACL permissions for libvirt-qemu user..."
setfacl -R -m u:libvirt-qemu:rx "$VM_DIR" 2>/dev/null || {
    echo "setfacl failed. You may need to install 'acl' package: sudo apt-get install acl"
    echo "Or run the sudo version: sudo ./virsh-fix-permissions.sh"
    exit 1
}

# Make sure the images are readable
chmod 644 "$VM_DISK" "$VM_SEED" "$VM_CLOUD_IMG" 2>/dev/null || true

echo "Permissions updated!"
echo ""
echo "If the VM still won't start, you have two options:"
echo "1. Run: sudo ./virsh-fix-permissions.sh (copies files to /var/lib/libvirt/images/)"
echo "2. Configure libvirt to run as your user (advanced)"