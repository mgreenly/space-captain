#!/bin/bash
set -e

# Load configuration
source "$(dirname "$0")/../config.sh"

# Check if QEMU is installed
if ! command -v "$QEMU_BIN" &> /dev/null; then
    echo "Error: $QEMU_BIN is not installed"
    echo "Install with: sudo apt-get install qemu-system-arm"
    exit 1
fi

# Check if VM is already running
if [ -f "$VM_PIDFILE" ] && kill -0 $(cat "$VM_PIDFILE") 2>/dev/null; then
    echo "VM '$VM_NAME' is already running (PID: $(cat $VM_PIDFILE))"
    exit 1
fi

# Download Debian cloud image if not present
if [ ! -f "$VM_CLOUD_IMG" ]; then
    echo "Downloading Debian cloud image..."
    mkdir -p "$IMAGES_DIR"
    curl -L -o "$VM_CLOUD_IMG" "$DEBIAN_CLOUD_URL"
fi

# Create VM disk from cloud image
if [ ! -f "$VM_DISK" ]; then
    echo "Creating VM disk from cloud image..."
    qemu-img create -f qcow2 -F qcow2 -b "$VM_CLOUD_IMG" "$VM_DISK" "$VM_DISK_SIZE"
fi

# Update cloud-init with current SSH key
echo "Updating cloud-init with current SSH key..."
SSH_PUB_KEY="$PROJECT_ROOT/.secrets/ssh/space-captain.pub"
if [ -f "$SSH_PUB_KEY" ]; then
    # Read the SSH public key
    SSH_KEY_CONTENT=$(cat "$SSH_PUB_KEY")
    
    # Create temporary cloud-init directory
    TEMP_CLOUD_INIT="$VM_DIR/temp-cloud-init"
    rm -rf "$TEMP_CLOUD_INIT"
    mkdir -p "$TEMP_CLOUD_INIT"
    
    # Copy meta-data
    cp "$CLOUD_INIT_DIR/meta-data" "$TEMP_CLOUD_INIT/"
    
    # Generate user-data from template
    cp "$CLOUD_INIT_DIR/user-data.template" "$TEMP_CLOUD_INIT/user-data"
    
    # Replace placeholder with actual SSH key (escape special characters)
    SSH_KEY_ESCAPED=$(echo "$SSH_KEY_CONTENT" | sed 's/[[\.*^$()+?{|]/\\&/g')
    sed -i "s|SSH_KEY_PLACEHOLDER|$SSH_KEY_ESCAPED|g" "$TEMP_CLOUD_INIT/user-data"
    
    # Create cloud-init ISO from temp directory
    echo "Creating cloud-init seed ISO..."
    cd "$TEMP_CLOUD_INIT"
    genisoimage -output "$VM_SEED" -volid cidata -joliet -rock user-data meta-data
    
    # Clean up temp directory
    rm -rf "$TEMP_CLOUD_INIT"
else
    echo "Error: SSH public key not found at $SSH_PUB_KEY"
    echo "Run 'make vm-ssh-keys' to generate SSH keys"
    exit 1
fi

# Clean up old socket/files
rm -f "$VM_MONITOR" "$VM_SERIAL"

echo "Launching VM '$VM_NAME'..."
echo "  CPUs: $VM_CPUS"
echo "  Memory: $VM_MEMORY MB"
echo "  Disk: $VM_DISK"
echo "  Architecture: $VM_ARCH"

# Launch VM
$QEMU_BIN \
    -name "$VM_NAME" \
    -machine q35 \
    $ACCEL_OPTS \
    -smp "$VM_CPUS" \
    -m "$VM_MEMORY" \
    -drive file="$VM_DISK",if=virtio,format=qcow2 \
    -drive file="$VM_SEED",if=virtio,format=raw \
    -netdev user,id=net0,hostfwd=tcp::2222-:22 \
    -device "$VM_NET_DEVICE",netdev=net0,mac="$VM_MAC" \
    -virtfs local,path="$PROJECT_ROOT",mount_tag=workspace,security_model=none,id=workspace \
    -serial file:"$VM_SERIAL" \
    -monitor unix:"$VM_MONITOR",server,nowait \
    -pidfile "$VM_PIDFILE" \
    -daemonize \
    -display none

# Wait for VM to start
sleep 2

if [ -f "$VM_PIDFILE" ] && kill -0 $(cat "$VM_PIDFILE") 2>/dev/null; then
    echo "VM started successfully (PID: $(cat $VM_PIDFILE))"
    echo ""
    if [ ! -e /dev/kvm ]; then
        echo "WARNING: KVM not available - using software emulation (slower)"
        echo ""
    fi
    echo "To connect:"
    echo "  SSH: ssh -p 2222 debian@localhost"
    echo "  Serial: tail -f $VM_SERIAL"
    echo "  Monitor: socat - UNIX-CONNECT:$VM_MONITOR"
    echo ""
    echo "The project directory is mounted at /workspace inside the VM"
    echo ""
    echo "If SSH hangs, the VM is still booting. Check progress with:"
    echo "  tail -f $VM_SERIAL"
else
    echo "Failed to start VM"
    exit 1
fi