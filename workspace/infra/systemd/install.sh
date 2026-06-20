#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SERVICE_DIR="$HOME/.config/systemd/user"

UNITS=(
    quantithaca.service
    rclone-onedrive.service
)

mkdir -p "$SERVICE_DIR"

for UNIT in "${UNITS[@]}"; do
    cp "$SCRIPT_DIR/$UNIT" "$SERVICE_DIR/$UNIT"
    echo "Installed $UNIT"
done

systemctl --user daemon-reload

for UNIT in "${UNITS[@]}"; do
    systemctl --user enable "$UNIT"
    systemctl --user start "$UNIT"
    echo "Enabled and started $UNIT"
done

loginctl enable-linger "$USER"

echo ""
echo "Done. Check logs with:"
for UNIT in "${UNITS[@]}"; do
    echo "  journalctl --user -u $UNIT -f"
done
