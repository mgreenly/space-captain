#!/bin/bash

# Load configuration  
source "$(dirname "$0")/../config.sh"

echo "VM: $VM_NAME"
echo "Architecture: $VM_ARCH"
echo ""

if [ -f "$VM_PIDFILE" ] && kill -0 $(cat "$VM_PIDFILE") 2>/dev/null; then
    PID=$(cat "$VM_PIDFILE")
    echo "Status: RUNNING (PID: $PID)"
    echo ""
    echo "Resources:"
    echo "  CPUs: $VM_CPUS"
    echo "  Memory: $VM_MEMORY MB"
    echo "  Disk: $VM_DISK_SIZE"
    echo ""
    echo "Network:"
    echo "  SSH Port: 2222 (forwarded to VM port 22)"
    echo "  MAC Address: $VM_MAC"
    echo ""
    echo "Files:"
    echo "  Disk: $VM_DISK"
    echo "  Monitor: $VM_MONITOR"
    echo "  Serial: $VM_SERIAL"
    echo ""
    echo "Connect with:"
    echo "  ssh -p 2222 debian@localhost"
else
    echo "Status: STOPPED"
    echo ""
    if [ -f "$VM_DISK" ]; then
        echo "VM disk exists. Run './launch.sh' to start."
    else
        echo "No VM disk found. Run './launch.sh' to create and start."
    fi
fi