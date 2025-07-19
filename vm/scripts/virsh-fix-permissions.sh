#!/bin/bash
set -e

# Load configuration
source "$(dirname "$0")/../config.sh"

echo "Fixing permissions for libvirt access..."

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "This script needs sudo to copy files to /var/lib/libvirt/images/"
    echo "Please run: sudo $0"
    exit 1
fi

# Libvirt images directory
LIBVIRT_IMAGES="/var/lib/libvirt/images"

# Create directory if it doesn't exist
mkdir -p "$LIBVIRT_IMAGES"

# Copy images to libvirt directory
echo "Copying VM disk..."
cp -v "$VM_DISK" "$LIBVIRT_IMAGES/"
chown libvirt-qemu:libvirt-qemu "$LIBVIRT_IMAGES/$(basename $VM_DISK)"

echo "Copying cloud-init seed..."
cp -v "$VM_SEED" "$LIBVIRT_IMAGES/"
chown libvirt-qemu:libvirt-qemu "$LIBVIRT_IMAGES/$(basename $VM_SEED)"

echo "Copying base image..."
cp -v "$VM_CLOUD_IMG" "$LIBVIRT_IMAGES/"
chown libvirt-qemu:libvirt-qemu "$LIBVIRT_IMAGES/$(basename $VM_CLOUD_IMG)"

# Update the VM definition with new paths
echo "Updating VM definition..."
virsh dumpxml "$VM_NAME" > /tmp/space-captain-dev-temp.xml
sed -i "s|$VM_DISK|$LIBVIRT_IMAGES/$(basename $VM_DISK)|g" /tmp/space-captain-dev-temp.xml
sed -i "s|$VM_SEED|$LIBVIRT_IMAGES/$(basename $VM_SEED)|g" /tmp/space-captain-dev-temp.xml
sed -i "s|$PROJECT_ROOT|$PROJECT_ROOT|g" /tmp/space-captain-dev-temp.xml

# Redefine the VM
virsh undefine "$VM_NAME"
virsh define /tmp/space-captain-dev-temp.xml
rm /tmp/space-captain-dev-temp.xml

echo "Done! The VM images are now in $LIBVIRT_IMAGES/"
echo "The VM should now start without permission issues."
echo ""
echo "Note: Future updates to the VM disk will need to be copied again."