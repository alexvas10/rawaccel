# Raw Accel - linux port

Raw Accel is a mouse acceleration tool that allows for the acceleration of mouse input in the raw input stream. Originally a Windows-only driver, it has now been ported to Linux with full feature parity.

## 🐧 Linux Support

A feature-complete Linux port is available in the `linux/` directory. It consists of a C++ daemon (`rawacceld`) that runs at boot via systemd, and a Qt6 GUI for configuration.

### Features
- **Identical Math:** Uses the exact same C++ acceleration logic as the Windows version.
- **Settings Compatible:** Your `settings.json` from Windows works 100% unchanged on Linux.
- **Low Latency:** Daemon runs with real-time CPU scheduling (`SCHED_FIFO`) using `libevdev` and `uinput`.
- **Full GUI:** Qt6-based grapher with live charts and profiles.
- **Auto-start:** Daemon starts at boot automatically via systemd. GUI is run manually when needed.

---

### Installation

There are two install paths depending on your distro.

#### Option A — Pre-built Binary (Arch Linux / Manjaro only)

Download `rawaccel-linux-x64.tar.gz` from the [Releases](../../releases) page, extract it, and run:

```bash
tar -xzvf rawaccel-linux-x64.tar.gz
cd rawaccel-linux-x64
sudo ./install-binary.sh
```

> **Note:** This binary was compiled on Arch Linux. If it fails with library errors on your system, use Option B instead.

---

#### Option B — Build from Source (All Distros)

**Step 1 — Install build dependencies for your distro:**

| Distro | Command |
|---|---|
| **Ubuntu / Debian / Pop!\_OS / Mint** | `sudo apt install cmake build-essential libevdev-dev qt6-base-dev qt6-charts-dev` |
| **Fedora / Nobara** | `sudo dnf install cmake gcc-c++ libevdev-devel qt6-qtbase-devel qt6-qtcharts-devel` |
| **Arch / Manjaro / EndeavourOS** | `sudo pacman -S cmake libevdev qt6-base qt6-charts` |
| **openSUSE** | `sudo zypper install cmake gcc-c++ libevdev-devel qt6-base-devel qt6-charts-devel` |

**Step 2 — Clone and install:**

```bash
git clone https://github.com/alexvas10/rawaccel
cd rawaccel/linux
sudo ./install/install.sh
```

The script builds, installs the daemon and GUI to `/usr/local/bin`, sets up the systemd service, and loads the `uinput` kernel module.

---

#### Post-install (both options)

Add your user to the `input` group so the GUI can connect to the daemon:

```bash
sudo usermod -aG input $USER
```

Then **log out and back in**, and launch the GUI:

```bash
rawaccel-gui
```

---

### Important: Disable Compositor Acceleration

If your mouse feels faster than expected after installing, your desktop environment is applying its own acceleration curve **on top of** rawaccel's output. You must disable it.

**Hyprland** — add to `~/.config/hypr/hyprland.conf`:
```
input {
    accel_profile = flat
}
device {
    name = rawaccel
    accel_profile = flat
    sensitivity = 0
}
```

**KDE Plasma** — System Settings → Input Devices → Mouse → Pointer Acceleration → set to **None**

**GNOME** — run in terminal:
```bash
gsettings set org.gnome.desktop.peripherals.mouse accel-profile flat
```

**X11 (any desktop)** — add to `~/.xinitrc` or your X11 autostart:
```bash
xinput set-prop "rawaccel" "libinput Accel Profile Enabled" 0 1
```

---

### Useful Commands

```bash
# Check daemon status
systemctl status rawaccel

# View daemon logs live
journalctl -u rawaccel -f

# Restart daemon (e.g. after changing settings.json manually)
sudo systemctl restart rawaccel

# Settings file location
/etc/rawaccel/settings.json
```

---

## 🪟 Windows Support

Raw Accel is a Windows 10 & Windows 11 x86-64 driver which allows for the acceleration of mouse input in the raw input stream. It started as a replacement for [InterAccel](https://github.com/KovaaK/InterAccel) and has been extended with more acceleration types, charts, and other features.

## Anti-Cheat Friendly

[Releases](https://github.com/a1xd/rawaccel/releases/latest) of the Raw Accel driver are signed and run in system space. Raw Accel only modifies mouse input by a constant set of formulas, and adds a one-second delay when changing settings in order to mitigate its abuse.

## Getting Help

For an overview of everything Raw Accel has to offer, please see the [guide](doc/Guide.md). For questions, see the [FAQ](doc/FAQ.md) first.

## Development

Development of Raw Accel is ongoing. See "User Interface 2.0" below for work on next release. Bug reports and pull requests are always welcome.  Join [our Discord server](https://discord.gg/7pQh8zH) if you want to stay updated with releases or say hello.

### User Interface 2.0
The next release of Raw Accel is planned to have a new User Interface. Work for this is ongoing at https://github.com/RawAccelOfficial/rawaccel/tree/userinterface. Check out the ReadMe in that branch if interested in contributing.

## External Websites
Raw Accel has moved from its old home at https://github.com/a1xd/rawaccel to https://github.com/RawAccelOfficial/rawaccel. (If you are unsure, you can check that the original link redirects to the new one.)  
The latest version of Raw Accel is always hosted [here on github](https://github.com/RawAccelOfficial/rawaccel). There is no other site or mirror where you can be sure get official versions of Raw Accel.   
Raw Accel is not affiliated with any external websites, such as rawaccel.net or rawaccel.com. If Raw Accel ever were to be affiliated with an external site, it would be mentioned here on github first.

## Credits
simon - Driver & Acceleration Logic  
\_m00se\_ - GUI, Gain features, Acceleration types  
Sidiouth  - Primary tester and sounding board  
TauntyArmordillo - Originator of the ideas behind Synchronous and Natural curve types
Kovaak - Brought us together
