#!/bin/bash
set -e

# Load configuration
source "$(dirname "$0")/../config.sh"

if [ ! -f "$VM_PIDFILE" ]; then
    echo "VM '$VM_NAME' is not running (no PID file found)"
    exit 0
fi

PID=$(cat "$VM_PIDFILE")

if ! kill -0 "$PID" 2>/dev/null; then
    echo "VM '$VM_NAME' is not running (stale PID file)"
    rm -f "$VM_PIDFILE"
    exit 0
fi

echo "Stopping VM '$VM_NAME' (PID: $PID)..."

# Send powerdown command via monitor
if [ -S "$VM_MONITOR" ]; then
    echo "system_powerdown" | socat - UNIX-CONNECT:"$VM_MONITOR" >/dev/null 2>&1 || true
    
    # Wait for graceful shutdown (max 30 seconds)
    for i in {1..30}; do
        if ! kill -0 "$PID" 2>/dev/null; then
            echo "VM stopped gracefully"
            rm -f "$VM_PIDFILE"
            exit 0
        fi
        sleep 1
    done
fi

# Force kill if still running
echo "Forcing VM termination..."
kill -TERM "$PID" 2>/dev/null || true
sleep 2

if kill -0 "$PID" 2>/dev/null; then
    kill -KILL "$PID" 2>/dev/null || true
fi

rm -f "$VM_PIDFILE"
echo "VM stopped"