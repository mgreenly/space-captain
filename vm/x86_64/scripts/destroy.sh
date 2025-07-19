#!/bin/bash
set -e

# Load configuration
source "$(dirname "$0")/../config.sh"

# Stop VM if running
"$(dirname "$0")/stop.sh"

echo "Destroying VM '$VM_NAME'..."

# Remove VM disk
if [ -f "$VM_DISK" ]; then
    echo "Removing VM disk..."
    rm -f "$VM_DISK"
fi

# Remove cloud-init seed
if [ -f "$VM_SEED" ]; then
    echo "Removing cloud-init seed..."
    rm -f "$VM_SEED"
fi

# Remove runtime files
rm -f "$VM_MONITOR" "$VM_SERIAL" "$VM_PIDFILE"

# Remove MAC address file
if [ -f "$VM_MAC_FILE" ]; then
    echo "Removing MAC address file..."
    rm -f "$VM_MAC_FILE"
fi

echo "VM '$VM_NAME' destroyed"
echo ""
echo "Note: The base cloud image was preserved at:"
echo "  $VM_CLOUD_IMG"
echo "To remove it, run: rm -f \"$VM_CLOUD_IMG\""