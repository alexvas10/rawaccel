#!/bin/bash
set -e

if [ "$(id -u)" -ne 0 ]; then
    echo "install.sh: must run as root (sudo ./install.sh)"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../build"

echo "==> Building rawaccel..."
mkdir -p "$BUILD_DIR"
cmake -B "$BUILD_DIR" -S "$SCRIPT_DIR/.." -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo "==> Installing binaries..."
install -m 755 "$BUILD_DIR/daemon/rawacceld"    /usr/local/bin/rawacceld
install -m 755 "$BUILD_DIR/gui/rawaccel-gui"    /usr/local/bin/rawaccel-gui

echo "==> Installing systemd service..."
install -m 644 "$SCRIPT_DIR/rawaccel.service"   /etc/systemd/system/rawaccel.service

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
echo "  sudo usermod -aG input \$SUDO_USER"
echo "  (then log out and back in)"
echo ""
echo "Run the GUI with:  rawaccel-gui"
echo "View daemon logs:  journalctl -u rawaccel -f"
