#!/bin/bash

# VM Configuration
VM_NAME="space-captain-dev"
VM_CPUS=4
VM_MEMORY=4096  # 4GB in MB (reduced for better emulation performance)
VM_DISK_SIZE="20G"
VM_ARCH="x86_64"

# Paths
# Get the vm directory (parent of scripts directory when called from scripts/)
SCRIPT_DIR="$(dirname "$(realpath "$0")")"
if [[ "$SCRIPT_DIR" == */scripts ]]; then
    VM_DIR="$(dirname "$SCRIPT_DIR")"
else
    VM_DIR="$SCRIPT_DIR"
fi
PROJECT_ROOT="$(dirname "$VM_DIR")"
IMAGES_DIR="$VM_DIR/images"
CLOUD_INIT_DIR="$VM_DIR/cloud-init"

# VM Files
VM_DISK="$IMAGES_DIR/${VM_NAME}-disk.qcow2"
VM_SEED="$IMAGES_DIR/${VM_NAME}-seed.iso"
VM_CLOUD_IMG="$IMAGES_DIR/debian-cloud-amd64.qcow2"
VM_PIDFILE="$VM_DIR/${VM_NAME}.pid"
VM_MONITOR="$VM_DIR/${VM_NAME}.monitor"
VM_SERIAL="$VM_DIR/${VM_NAME}.serial"

# Network
VM_MAC="52:54:00:$(openssl rand -hex 3 | sed 's/\(..\)/\1:/g; s/:$//')"
VM_NET_DEVICE="virtio-net-pci"

# Debian Cloud Image URL (latest stable)
DEBIAN_CLOUD_URL="https://cloud.debian.org/images/cloud/bookworm/latest/debian-12-genericcloud-amd64.qcow2"

# QEMU Binary
QEMU_BIN="qemu-system-x86_64"

# Acceleration options
if [ -e /dev/kvm ]; then
    # KVM available - use hardware acceleration
    ACCEL_OPTS="-enable-kvm -cpu host"
else
    # No KVM - use software emulation
    ACCEL_OPTS="-cpu qemu64"
fi