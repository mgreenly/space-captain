#!/bin/bash

# Load configuration
source "$(dirname "$0")/../config.sh"

# Check if VM is running
if [ ! -f "$VM_PIDFILE" ] || ! kill -0 $(cat "$VM_PIDFILE") 2>/dev/null; then
    echo "Error: VM '$VM_NAME' is not running"
    echo "Run './launch.sh' to start the VM"
    exit 1
fi

# SSH into VM
echo "Connecting to VM..."
ssh -4 -p 2222 -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -i "$PROJECT_ROOT/.secrets/ssh/space-captain" debian@localhost "$@"