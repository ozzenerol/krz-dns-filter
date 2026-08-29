#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

SERVICE_NAME="krz-dns-filter"
BLACKLIST="${1:-$SCRIPT_DIR/blacklist.txt}"
PORT="${2:-53}"
RUN_USER="${SUDO_USER:-$USER}"

if ! [[ "$PORT" =~ ^[0-9]+$ ]]; then
    echo "Invalid port: $PORT" >&2
    exit 1
fi

if [ ! -f "$BLACKLIST" ]; then
    echo "Blacklist file not found: $BLACKLIST" >&2
    exit 1
fi

echo "Building the project..."
make

BINARY="$SCRIPT_DIR/bin/$SERVICE_NAME"
if [ ! -x "$BINARY" ]; then
    echo "Build did not produce $BINARY" >&2
    exit 1
fi

UNIT_FILE="/etc/systemd/system/${SERVICE_NAME}.service"

echo "Writing systemd unit to $UNIT_FILE (needs sudo)..."
sudo tee "$UNIT_FILE" > /dev/null <<EOF
[Unit]
Description=krz-dns-filter DNS filtering server
After=network.target

[Service]
Type=simple
ExecStart=$BINARY $BLACKLIST $PORT
WorkingDirectory=$SCRIPT_DIR
Restart=on-failure
RestartSec=2
User=$RUN_USER
AmbientCapabilities=CAP_NET_BIND_SERVICE
CapabilityBoundingSet=CAP_NET_BIND_SERVICE
NoNewPrivileges=true
PrivateTmp=true

[Install]
WantedBy=multi-user.target
EOF

echo "Reloading systemd and starting the service..."
sudo systemctl daemon-reload
sudo systemctl enable --now "${SERVICE_NAME}.service"

echo "Done. Service is running. Check status with:"
echo "  systemctl status $SERVICE_NAME"
echo "  journalctl -u $SERVICE_NAME -f"
