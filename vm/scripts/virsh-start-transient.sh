#!/bin/bash
set -e

# Load configuration
source "$(dirname "$0")/../config.sh"

echo "Starting transient VM (all changes will be discarded on shutdown)..."

# Check if VM is defined in libvirt
if ! virsh -c qemu:///session list --all | grep -q "$VM_NAME"; then
    echo "VM not found in libvirt. Import it first with:"
    echo "  ./virsh-import.sh"
    exit 1
fi

# Check if VM is already running
if virsh -c qemu:///session list | grep -q "$VM_NAME"; then
    echo "VM is already running"
    exit 1
fi

# Create a temporary overlay disk for this session
TEMP_DISK="/tmp/${VM_NAME}-transient-$$.qcow2"
echo "Creating temporary overlay disk: $TEMP_DISK"
qemu-img create -f qcow2 -F qcow2 -b "$VM_DISK" "$TEMP_DISK"

# Create temporary XML with modified disk path
TEMP_XML="/tmp/${VM_NAME}-transient-$$.xml"
virsh -c qemu:///session dumpxml "$VM_NAME" > "$TEMP_XML"
sed -i "s|$VM_DISK|$TEMP_DISK|g" "$TEMP_XML"

# Start the VM transiently with the temporary disk
echo "Starting transient VM..."
virsh -c qemu:///session create "$TEMP_XML" --autodestroy

# Clean up temporary XML
rm -f "$TEMP_XML"

echo ""
echo "Transient VM started!"
echo "- All changes will be lost when the VM shuts down"
echo "- The temporary disk will be automatically deleted"
echo ""
echo "To connect:"
echo "  virsh -c qemu:///session console $VM_NAME"
echo "  Or use virt-manager: virt-manager -c qemu:///session"
echo ""
echo "To stop:"
echo "  virsh -c qemu:///session destroy $VM_NAME"