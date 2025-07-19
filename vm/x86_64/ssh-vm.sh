#!/bin/bash
# SSH into the Space Captain VM

# SSH with key authentication
# UserKnownHostsFile=/dev/null prevents saving the host key
# StrictHostKeyChecking=no disables host key verification
# Get the script directory to find the project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

ssh -4 -o StrictHostKeyChecking=no \
    -o UserKnownHostsFile=/dev/null \
    -i "$PROJECT_ROOT/.secrets/ssh/space-captain" \
    -p 2222 \
    debian@localhost "$@"