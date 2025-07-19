#!/bin/bash

# Load configuration
source "$(dirname "$0")/../config.sh"

echo "Note: When using libvirt user session, SSH port forwarding is not available."
echo "Please use one of these methods to access the VM:"
echo ""
echo "1. Use virt-manager console:"
echo "   virt-manager -c qemu:///session"
echo "   Then open the console for space-captain-dev"
echo ""
echo "2. Use virsh console:"
echo "   virsh -c qemu:///session console space-captain-dev"
echo "   (Press Ctrl+] to exit)"
echo ""
echo "3. Once the VM boots, find its IP address from the console and SSH directly:"
echo "   ssh debian@<vm-ip-address>"