#!/bin/bash
set -e

# Load configuration
source "$(dirname "$0")/../config.sh"

echo "Importing VM into libvirt/virt-manager..."

# Check if VM already exists in libvirt (user session)
if virsh -c qemu:///session list --all | grep -q "$VM_NAME"; then
    echo "VM '$VM_NAME' already exists in libvirt user session"
    echo "To remove it: virsh -c qemu:///session undefine $VM_NAME"
    exit 1
fi

# Make sure the VM is not running via QEMU direct
if [ -f "$VM_PIDFILE" ] && kill -0 $(cat "$VM_PIDFILE") 2>/dev/null; then
    echo "VM is running via QEMU. Stop it first with: make x86_64-stop"
    exit 1
fi

# Check if disk exists
if [ ! -f "$VM_DISK" ]; then
    echo "VM disk not found. Run 'make x86_64-start' at least once to create it."
    exit 1
fi

# Define the VM in libvirt (user session)
echo "Defining VM in libvirt user session..."
# Use x86_64 XML if it exists, otherwise fall back to generic
if [ -f "$VM_DIR/space-captain-dev-x86_64.xml" ]; then
    virsh -c qemu:///session define "$VM_DIR/space-captain-dev-x86_64.xml"
else
    virsh -c qemu:///session define "$VM_DIR/space-captain-dev.xml"
fi

echo "VM imported successfully!"
echo ""
echo "You can now:"
echo "  - Open virt-manager with user session: virt-manager -c qemu:///session"
echo "  - Start with: virsh -c qemu:///session start $VM_NAME"
echo "  - Connect with: virsh -c qemu:///session console $VM_NAME"
echo ""
echo "Note: This VM is in your user session, not the system session."
echo "Always use '-c qemu:///session' with virsh commands."