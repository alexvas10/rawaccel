#!/bin/bash
set -e

if [ "$(id -u)" -ne 0 ]; then
    echo "install-binary.sh: must run as root (sudo ./install-binary.sh)"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "==> Installing binaries..."
install -m 755 "$SCRIPT_DIR/rawacceld"        /usr/local/bin/rawacceld
install -m 755 "$SCRIPT_DIR/rawaccel-gui"     /usr/local/bin/rawaccel-gui

echo "==> Installing systemd service..."
install -m 644 "$SCRIPT_DIR/rawaccel.service" /etc/systemd/system/rawaccel.service

echo "==> Creating settings directory..."
mkdir -p /etc/rawaccel

echo "==> Enabling and starting daemon..."
systemctl daemon-reload
systemctl enable rawaccel
systemctl start rawaccel

echo "==> Loading uinput module..."
modprobe uinput
echo "uinput" > /etc/modules-load.d/rawaccel.conf

echo ""
echo "==> Installation complete!"
echo ""
echo "IMPORTANT: Add your user to the 'input' group to use the GUI:"
echo "  sudo usermod -aG input $SUDO_USER"
echo "  (then log out and back in)"
echo ""
echo "Run the GUI with:  rawaccel-gui"
echo "View daemon logs:  journalctl -u rawaccel -f"
