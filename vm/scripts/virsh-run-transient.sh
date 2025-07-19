#!/bin/bash
set -e

# Load configuration
source "$(dirname "$0")/../config.sh"

echo "Running VM in transient mode (no changes will be saved)..."

# Check if VM is defined in libvirt
if ! virsh -c qemu:///session list --all | grep -q "$VM_NAME"; then
    echo "VM not found in libvirt. Import it first with:"
    echo "  ./virsh-import.sh"
    exit 1
fi

# Check if VM is already running
if virsh -c qemu:///session list | grep -q "$VM_NAME"; then
    echo "VM is already running. Stop it first with:"
    echo "  virsh -c qemu:///session destroy $VM_NAME"
    exit 1
fi

# Start VM in transient mode
echo "Starting transient VM..."
echo "Note: First boot will take time to install packages"
virsh -c qemu:///session start "$VM_NAME"

echo ""
echo "VM started in normal mode (changes WILL be saved)"
echo "For true transient mode with overlay disk, use: ./virsh-start-transient.sh"
echo ""
echo "To connect:"
echo "  virsh -c qemu:///session console $VM_NAME"
echo ""
echo "To make it transient, you can:"
echo "1. Take a snapshot now: virsh -c qemu:///session snapshot-create-as $VM_NAME clean"
echo "2. Always revert before use: virsh -c qemu:///session snapshot-revert $VM_NAME clean"