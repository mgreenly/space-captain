#!/bin/bash
set -e

# Load configuration
source "$(dirname "$0")/../config.sh"

echo "Removing VM from libvirt user session..."

# Check if VM exists
if ! virsh -c qemu:///session list --all | grep -q "$VM_NAME"; then
    echo "VM '$VM_NAME' not found in libvirt user session"
    exit 0
fi

# Stop if running
if virsh -c qemu:///session list | grep -q "$VM_NAME"; then
    echo "Stopping VM..."
    virsh -c qemu:///session destroy "$VM_NAME" || true
fi

# Undefine the VM
echo "Removing VM definition..."
virsh -c qemu:///session undefine "$VM_NAME"

echo "VM removed from libvirt"
echo ""
echo "Note: The disk image still exists at:"
echo "  $VM_DISK"
echo "To remove it, run: make aarch64-destroy"