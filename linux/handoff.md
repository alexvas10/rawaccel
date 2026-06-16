# rawaccel Linux Port — Handoff Document

## Goal

Port rawaccel (a Windows-only mouse acceleration tool) to Linux. The Linux version must:

- Apply identical mouse acceleration math to the Windows version
- Use a settings file format that is 100% compatible with Windows — you can copy `settings.json` from Windows to Linux and it works unchanged
- Live in the `linux/` subdirectory of the existing rawaccel repo at `/home/alex/Documents/Code/rawaccel/`

The Windows version is a kernel-mode driver + C# WinForms GUI. The Linux version replaces this with:

- A C++ daemon (`rawacceld`) running as root via systemd, using libevdev to grab mouse input and uinput to emit accelerated output
- A C++ Qt6 GUI (`rawaccel-gui`) with the same controls and acceleration curve charts as the Windows grapher

The acceleration math lives in `common/` (pure C++ headers shared between Windows and Linux, zero platform dependencies). The Linux port reuses these headers directly.

---

## Architecture

```
Physical Mouse
     |  (raw EV_REL events via libevdev, exclusive grab)
     v
rawacceld  (root systemd service)
     |  applies modifier::modify() from common/ headers
     |  writes to /dev/uinput (virtual mouse device named "rawaccel")
     |  listens on /var/run/rawaccel.sock (IPC)
     v
Virtual Mouse  ->  X11 / Wayland / games see accelerated input

rawaccel-gui  (Qt6, user process, must be in 'input' group)
     |  reads/writes /etc/rawaccel/settings.json
     |  connects to /var/run/rawaccel.sock for READ/WRITE/SUBSCRIBE
     v
Acceleration curve charts + settings controls
```

IPC wire format: `[uint32 LE length][UTF-8 JSON body]` over Unix domain socket.

Commands (GUI -> daemon): `{"cmd":"READ"}`, `{"cmd":"WRITE","settings":{...}}`, `{"cmd":"SUBSCRIBE"}`

Responses (daemon -> GUI): `{"type":"SETTINGS",...}`, `{"type":"ERROR",...}`, `{"type":"EVENT","x":5,"y":-2,"time_ms":1.25}`

---

## Current State

**The Linux port is feature-complete and verified end-to-end against the user's real Windows settings, with the daemon installed as a boot-time systemd service.**

- Daemon (`rawacceld`) and GUI (`rawaccel-gui`) both build cleanly via `cmake --build build -j$(nproc)` from `linux/`.
- The user's actual Windows `settings.json` (not just auto-generated defaults) has been copied to `/etc/rawaccel/settings.json`, loaded by the daemon with **no errors**, and the GUI confirmed the displayed profile/curve/values match what the user had on Windows. The `SettingsLoader.cpp` Windows-JSON field-mapping is now considered validated against a real-world export.
- The daemon is installed via `linux/install/install.sh` as the systemd service `rawaccel.service` — confirmed `enabled` and `active (running)` via `systemctl status rawaccel`, so it starts at boot without the user needing to run anything manually. The GUI is intentionally **not** part of any autostart — the user only wants the daemon to start automatically, and runs `rawaccel-gui` manually when needed.
- Mouse motion has been confirmed by the user to feel "identical to Windows" — this took two fixes this session, both described below (libinput double-acceleration, and a click-registration bug).
- Outstanding design question from Session 2, still holding: "if someone turns off the daemon, mouse movement should go back to normal" — true because `EVIOCGRAB` is tied to the fd, released automatically by the kernel on process exit regardless of signal. No code needed.
- Known, deliberately-deferred minor issue: `rawacceld` has no `SIGTERM` handler, so `systemctl stop`/`restart` takes ~90s while systemd waits out the default timeout before SIGKILLing it. User said to leave this for now but keep it as a standing suggestion.
- Minor daemon UX gap (carried over from Session 2, still not fixed): `rawacceld` ignores unrecognized CLI args (e.g. `--version`, `--help`) and just starts running normally instead of printing info and exiting.

---

## All Source Files

```
linux/
├── CMakeLists.txt                      — top-level build, optional GUI
├── shared/
│   ├── compat.hpp                      — MSVC shims: _copysign, __forceinline
│   └── ipc_protocol.hpp                — 4-byte length-prefix helpers (ipc_send/ipc_recv)
├── daemon/
│   ├── CMakeLists.txt
│   ├── main.cpp                        — entry point, inotify settings watch, signal handling
│   ├── SettingsLoader.hpp/cpp          — JSON <-> C++ struct conversion (Windows-compatible)
│   ├── VirtualMouse.hpp/cpp            — /dev/uinput writer
│   ├── MouseDevice.hpp/cpp             — libevdev reader, per-device thread, accel pipeline
│   ├── DeviceManager.hpp/cpp           — enumerates /dev/input/event*, inotify hotplug
│   └── IpcServer.hpp/cpp               — Unix socket server, READ/WRITE/SUBSCRIBE handlers
├── gui/
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── MainWindow.hpp/cpp              — QMainWindow, left panel + right charts
│   ├── AccelGUI.hpp/cpp                — coordinator: loads settings, wires widgets, IPC
│   ├── Calculator.hpp/cpp              — simulates accel curve points (matches Windows math)
│   ├── SettingsManager.hpp/cpp         — GUI-side JSON read/write (no daemon dependency)
│   ├── IpcClient.hpp/cpp               — QLocalSocket client to daemon
│   ├── AccelTypePanel.hpp/cpp          — mode combo + all param spinboxes, show/hide per mode
│   ├── Options/
│   │   ├── OptionWidget.hpp/cpp        — labeled spinbox + current value label
│   │   ├── LockableOption.hpp/cpp      — OptionWidget + lock button (X=Y coupling)
│   │   └── LutEditor.hpp/cpp           — plain text editor for LUT point pairs
│   ├── Layouts/
│   │   └── LayoutBase.hpp              — ModeLayout: which controls show per accel mode
│   └── Charts/
│       ├── ChartXY.hpp/cpp             — QChart wrapper (combined or split X/Y series)
│       └── AccelCharts.hpp/cpp         — 3 chart rows in a splitter (Sens/Velocity/Gain)
└── install/
    ├── rawaccel.service                — systemd unit (Type=simple, Restart=always, root)
    └── install.sh                      — builds, installs, enables service, loads uinput module
```

The `common/` headers at `/home/alex/Documents/Code/rawaccel/common/` are included directly and not modified.

---

## Key Implementation Notes

**MSVC shims** (`shared/compat.hpp`): `common/rawaccel.hpp` uses `_copysign` (MSVC name) and `__forceinline`. Compat.hpp redefines both. This header must be included before any `common/` header — it is included in `MouseDevice.hpp` which is the entry point for the accel headers.

**`rawaccel::init_data()` call**: After filling `modifier_settings::prof` and before constructing `modifier`, you must call `rawaccel::init_data(settings)`. This populates the data union inside the accel_args. The daemon's `SettingsLoader` calls this at the end of `settings_from_json()`.

**`wchar_t` name fields**: `profile::name` and `device_settings::name/id/profile` are `wchar_t[256]`. SettingsLoader uses `mbstowcs`/`wcstombs` with `setlocale(LC_ALL,"")` (called in main.cpp before any JSON parsing).

**JSON key name overrides** (Windows C# uses `[JsonProperty]` attributes, so field names differ from C++ struct member names). Key mappings in SettingsLoader:
- `"Whole or horizontal accel parameters"` → `profile.accel_x`
- `"Vertical accel parameters"` → `profile.accel_y`
- `"Gain / Velocity"` → `accel_args.gain` (bool)
- `"Cap / Jump"` → `accel_args.cap` (vec2d)
- `"Cap mode"` → `accel_args.cap_mode` (`"io"`→`cap_mode::io`, `"input"`→`cap_mode::in`, `"output"`→`cap_mode::out`)
- `"Whole/combined accel (set false for 'by component' mode)"` → `speed_args.whole`
- LUT mode JSON key in `"Acceleration mode"` is `"lut"` but C++ enum is `accel_mode::lookup`

**LUT data format**: JSON stores `[[x1,y1],[x2,y2],...]` as a flat float array in `data[]` with a `length` field equal to `2 * num_points`.

**Fractional carry-over**: `MouseDevice` accumulates sub-integer output in `frac_x_`, `frac_y_` so motion isn't lost at low speeds (matches Windows driver behavior).

**GCC `-Wchanges-meaning` suppression**: `common/rawaccel-base.hpp:62` has `cap_mode cap_mode = cap_mode::out` (member same name as type). GCC 13+ rejects this. Both CMakeLists pass `-Wno-changes-meaning` to suppress it. Do not remove this flag — the upstream header is not ours to modify.

**`vec2d` namespace**: `vec2d` is declared at file scope in `common/math-vec2.hpp` (`using vec2d = vec2<double>`), NOT inside the `rawaccel` namespace. Use `vec2d`, not `rawaccel::vec2d`.

**nlohmann-json**: Not installed as a system package. CMakeLists uses FetchContent to download it automatically from GitHub during configure. The downloaded copy lives in `build/_deps/`.

---

## What Changed — Session 1 (Initial Port, From Scratch)

This session created the entire Linux port from scratch. All source files in `linux/` were written new — nothing existed before.

Fixes applied during the build attempt:

1. **Wrong `vec2d` namespace** — `MouseDevice.cpp` used `rawaccel::vec2d` but `vec2d` is not in that namespace. Changed to just `vec2d`.

2. **Missing include in `main.cpp`** — `main.cpp` referenced `rawaccel_ipc::SOCKET_PATH` without including `ipc_protocol.hpp`. Added the include.

3. **`-Wchanges-meaning` compile error** — GCC 13 rejects the `cap_mode cap_mode` member in the upstream `rawaccel-base.hpp`. Added `-Wno-changes-meaning` to both daemon and GUI CMakeLists.

4. **CMake include path bug** — The daemon and GUI CMakeLists initially only added `linux/shared/` to include dirs, which broke `#include "shared/compat.hpp"`. Fixed by also adding `${CMAKE_CURRENT_SOURCE_DIR}/..` (the `linux/` directory) so both `#include "shared/compat.hpp"` and `#include "compat.hpp"` resolve correctly.

5. **FetchContent timestamp warning** — Added `DOWNLOAD_EXTRACT_TIMESTAMP TRUE` to the nlohmann-json FetchContent declaration.

6. **GUI made optional** — Initially `find_package(Qt6 REQUIRED ...)` failed because `qt6-charts` isn't installed. Changed to optional: GUI only builds if Qt6Charts is found.

Failed attempts in Session 1: **none that required architectural reversal.** The design chosen at the start held throughout; all failures were compile-time errors caught and fixed in the same session.

---

## What Changed — Session 2 (Build, Run, Debug, Verify)

Picked up with `qt6-charts` now installed but the CMake cache still stale (`Qt6Charts_DIR-NOTFOUND`), and the GUI never compiled even once. Goal of the session: get the daemon actually running, build the GUI for the first time, and verify the whole pipeline end-to-end with real hardware — then move on to importing the user's real Windows settings.

**Files modified this session:**
- `CMakeLists.txt` (top level) — added `Network` to the `find_package(Qt6 ...)` components and to the GUI-enable condition.
- `gui/CMakeLists.txt` — added `Qt6::Network` to `target_link_libraries`.
- `gui/Calculator.cpp` — fixed `rawaccel::vec2d` → `vec2d` at two call sites (same class of bug as the one already fixed in `MouseDevice.cpp` in Session 1; `vec2d` is file-scope, not inside `namespace rawaccel`).
- `daemon/MouseDevice.cpp` — two real runtime bugs fixed, see below.
- `daemon/VirtualMouse.hpp` / `daemon/VirtualMouse.cpp` — added `VENDOR_ID`/`PRODUCT_ID` constants used to self-identify the virtual device (needed by the `MouseDevice.cpp` fix below).
- `install/rawaccel.service` — changed boot ordering so the daemon starts before the SDDM login screen instead of waiting for `multi-user.target` (see below). **Not yet installed/enabled** — left for the user to do explicitly since it's a root/boot-affecting change.

**Bugs found and fixed:**

1. **CPU busy-loop (300–400% CPU) in `MouseDevice.cpp`.** The device fd was opened with `O_NONBLOCK`, but `run_loop()` calls `libevdev_next_event(..., LIBEVDEV_READ_FLAG_BLOCKING, ...)` expecting a real blocking read. With `O_NONBLOCK` set, every read returned `EAGAIN` immediately and the loop's `if (rc == -EAGAIN) continue;` spun as fast as possible. Confirmed via the local libevdev header docs that `LIBEVDEV_READ_FLAG_BLOCKING` is just a descriptive flag — actual blocking depends entirely on the fd's `O_NONBLOCK` status, which the flag does not override. **Fix:** removed `O_NONBLOCK` from the `open()` call in the `MouseDevice` constructor.

2. **Self-feedback infinite loop (sustained ~84-90% CPU after the first mouse movement, never idling).** `DeviceManager` enumerates every `/dev/input/event*` node, including the daemon's own uinput-created "rawaccel" output device (it advertises `REL_X`/`REL_Y` so it looks like a legitimate mouse). The daemon was grabbing and reading back the events it had just written to itself, forever. **Fix:** tagged the virtual device with a vendor/product ID (`VirtualMouse::VENDOR_ID = 0x1234`, `PRODUCT_ID = 0x5678`, bus type `BUS_VIRTUAL`), and added a check in `MouseDevice`'s constructor that excludes any device matching those IDs from being treated as an input source.

**Verified working end-to-end:**
- Daemon stays alive, CPU usage is normal (no busy loop) after both fixes above.
- Kernel/udev registers the virtual device correctly: `/proc/bus/input/devices` shows `Handlers=event19 mouse2`, correct `EV`/`REL`/`KEY` bitmaps matching what `VirtualMouse.cpp` sets up; udev tags it `ID_INPUT=1`, `ID_INPUT_MOUSE=1`.
- `loginctl seat-status seat0` lists the virtual device (`input30 "rawaccel"`) as assigned to the active seat — ruled out a seat-assignment theory that was being investigated mid-session.
- `hyprctl devices` lists "rawaccel" as a recognized mouse.
- **The cursor visibly moves on screen** under Hyprland/Wayland when the physical mouse is moved — confirmed directly by the user, not just by inspecting raw evdev bytes (this distinction mattered, see Failed Attempts below).
- GUI builds and runs; status bar connects to the daemon over `/var/run/rawaccel.sock`; clicking Apply in the GUI was confirmed to reach the daemon and write through to `/etc/rawaccel/settings.json` (md5sum of the file changed before/after Apply).
- Confirmed the daemon doesn't need any special "graceful shutdown" handling for the user's "give my mouse back when the daemon stops" requirement: `EVIOCGRAB` is tied to the fd, so the kernel releases the grab automatically on process exit for *any* reason (including `kill -9`), restoring normal mouse behavior immediately. Tested directly via `sudo pkill -9 rawacceld` while the daemon was mid-grab.

**Failed attempts / dead ends this session:**

- **Mid-session false alarm: cursor frozen, clicks not registering, while the daemon was confirmed running and emitting well-formed events.** Spent significant time chasing a udev/seat-assignment theory (virtual device lacking `ID_PATH`/`ID_PATH_TAG` compared to the real device) — this theory was disproven (`loginctl seat-status` showed the virtual device WAS correctly assigned to seat0). Also checked Hyprland's live log for a `libinput: New device rawaccel` line and didn't find one, which looked suspicious, but turned out to be because the log file had stopped being written to ~14 minutes *before* the daemon was even started that time around — i.e. it predated the device's existence and was not evidence of anything. By the time a live-log-tailing re-test was about to start, **the user reported the cursor had started moving again on its own** (likely transient — possibly a stale/leftover daemon instance from earlier testing was the one actually holding the grab, or a Hyprland-side hiccup; root cause of the transient freeze was never conclusively identified). Net effect: don't assume "raw evdev bytes look correct" means "cursor moves" — confirm the latter directly. If this recurs, check `ps aux | grep rawacceld` for duplicate/stale daemon instances first (this session accidentally launched two at once earlier by passing `--version`/`--help` flags, which the daemon ignores and just runs normally instead of printing info and exiting — that's a minor daemon UX gap, not yet fixed, see Next Step).
- **`sudo` in the agent's shell has no interactive password support.** Worked around with a scoped `NOPASSWD` sudoers entry for the exact `rawacceld` binary path with no argument restriction. This caused the duplicate-daemon accident above (any args to that exact path run passwordless and the binary ignores unrecognized args). Commands like `pkill`, `mkdir`, `cat` on root-owned device nodes are NOT covered by this rule — the user has to run those themselves.
- **`libinput list-devices` / `libinput debug-events`** (suggested in the Session 1 "Next Step") **don't exist on this machine** — only the `libinput` *library* is installed, not `libinput-tools` (separate package, not installed). Used `hyprctl devices`, `/proc/bus/input/devices`, and `udevadm info` instead, which don't require it.

---

## What Changed — Session 3 (Real Settings Import, systemd Install, libinput Double-Accel, Click Bug)

Picked up with the daemon/GUI built and verified against auto-generated defaults only, systemd service written but not installed, and the user still waiting on access to their real Windows `settings.json`. Goal of the session: get the user's actual Windows settings imported and verified, install the daemon as a real boot service, and chase down two behavioral bugs the user noticed only after testing with real settings on real hardware (movement felt faster than Windows, then clicks not registering while stationary).

**Files modified this session:**
- `daemon/MouseDevice.cpp` — one real bug fixed, see below. No other source files touched.
- `~/.config/hypr/hyprland.conf` (outside the repo, user's Hyprland config) — added `accel_profile = flat` to the global `input {}` block, and a new `device { name = rawaccel; accel_profile = flat; sensitivity = 0; }` block. Not part of the rawaccel repo, but required for correct behavior on this user's machine and worth knowing about in any future session.
- `/etc/rawaccel/settings.json` (outside the repo) — replaced with the user's real Windows export (downloaded to `~/Downloads/settings.json`). The user explicitly declined a backup of the previous (default-generated) file, so the old contents are gone.
- `/etc/systemd/system/rawaccel.service`, `/usr/local/bin/rawacceld`, `/usr/local/bin/rawaccel-gui` — installed for the first time via `sudo ./install/install.sh`, then `rawacceld` reinstalled a second time after the click-bug fix (`sudo install -m 755 build/daemon/rawacceld /usr/local/bin/rawacceld` + `sudo systemctl restart rawaccel`).

**Bugs found and fixed:**

1. **Mouse felt faster than Windows (not a `settings.json`/math bug).** Root cause was outside the C++ code entirely: Hyprland's `input:accel_profile` was unset, so libinput applied its own default `adaptive` pointer-acceleration curve on top of the daemon's already-accelerated output from the virtual `rawaccel` uinput device — i.e. two acceleration curves stacked. This is the Linux equivalent of Windows' "Enhance pointer precision" / pointer-speed slider needing to be left neutral so the OS doesn't add a second curve on top of the driver's output. **Fix:** added `accel_profile = flat` (global, in `input {}`) and a per-device override (`device { name = rawaccel; accel_profile = flat; sensitivity = 0; }`) in `hyprland.conf`, confirmed via `hyprctl getoption input:accel_profile` → `flat`. User confirmed movement now feels identical to Windows.

2. **Clicks not registering while the mouse was stationary (`daemon/MouseDevice.cpp:126-130`).** In `run_loop()`, on `SYN_REPORT` the code checked `if (acc_x == 0 && acc_y == 0) { has_last = false; continue; }` — i.e. when a frame had no motion (e.g. a click with no mouse movement), it skipped forwarding *anything* to the virtual device for that frame, including a `SYN_REPORT`. Button events earlier in the same frame were already written raw via `VirtualMouse::emit_event()`, but `emit_event()` for `EV_KEY` never calls `sync()` — only `emit_motion()` does. So a button-down event sat unflushed in the uinput write buffer until some *later* frame happened to contain motion and call `emit_motion()`, which is why clicks only seemed to register while moving. **Fix:** call `virtual_mouse_.sync()` before the early `continue` so every original-device `SYN_REPORT` is mirrored to the virtual device regardless of whether there was motion that frame. Rebuilt (`cmake --build build -j$(nproc) --target rawacceld`), reinstalled the binary, restarted the service, user confirmed clicks now register reliably while stationary.

**Verified working end-to-end this session:**
- Daemon loads the user's real Windows `settings.json` with no errors (`rawacceld: loaded settings from /etc/rawaccel/settings.json`), managing 2 mouse devices.
- GUI displays the imported profile/curve/values matching what the user had on Windows.
- Mouse movement feels "identical to Windows" per the user, after the libinput flat-profile fix.
- Daemon installed via `install.sh`, confirmed `systemctl is-enabled rawaccel` → `enabled` and `systemctl status` → `active (running)`, survives without the user manually starting anything.
- User is in the `input` group already (`groups alex` → includes `input`), so no group/logout step was needed for the GUI.
- Clicking while the mouse is completely stationary now works reliably, confirmed by the user after the `MouseDevice.cpp` fix + binary reinstall + service restart.

**Failed attempts / dead ends this session:**

- **None that required reversal.** Both bugs (libinput double-accel, click-sync) were diagnosed correctly on the first hypothesis and fixed in one pass each. The only friction was operational, not a wrong technical turn: `sudo systemctl restart rawaccel` appeared to hang — this was not a bug, just `rawacceld` having no `SIGTERM` handler, so systemd waited its full default ~90s stop-timeout before SIGKILLing the old process. Confirmed via `journalctl -u rawaccel` showing `State 'stop-sigterm' timed out. Killing.` This is the same known gap noted in Current State (deferred per user's instruction, kept as a standing suggestion — would need a signal handler in `main.cpp` that calls `exit()` or unwinds cleanly to make stop/restart fast).
- **Two near-misses on operational sequencing, both resolved by re-explaining rather than redoing technical work:** (a) the user initially stopped only the manually-run daemon, not realizing the manually-run GUI didn't matter for the systemd install — clarified the GUI was never a conflict, only the manual daemon process was; (b) the user ran `install.sh` and reported success, but a `systemctl status` check afterward showed the unit didn't exist — turned out the user had only *talked about* running it and hadn't actually executed it yet in that exact moment; once they did, it installed cleanly on the first real attempt.

---

## Next Step

There is no blocking technical work left — the port is functionally complete and verified against the user's real settings and hardware. The single next thing to consider, at the user's discretion (not yet requested, offered and explicitly deferred):

**Add a `SIGTERM`/`SIGINT` handler to `daemon/main.cpp` for fast, clean shutdown.** Currently `rawacceld` has no signal handler, so any `systemctl stop`/`restart rawaccel` blocks for ~90 seconds while systemd waits out its default stop-timeout before SIGKILLing the process (confirmed in Session 3 via `journalctl` showing `State 'stop-sigterm' timed out. Killing.`). This doesn't affect normal operation (the kernel still releases the `EVIOCGRAB` and restores normal mouse behavior immediately on SIGKILL, per the Session 2 finding) — it's purely an operational annoyance when the user wants to reinstall/restart the daemon after a code change. Fix would be a handler that sets `running_ = false` (or calls `exit(0)` directly) so the process exits promptly instead of ignoring `SIGTERM` and forcing a timeout.

**Other deferred, non-blocking item carried over from Session 2:** `rawacceld` ignores unrecognized CLI args (e.g. `--version`, `--help`) and just starts running normally instead of printing info and exiting — caused an accidental duplicate-daemon-instance bug once in Session 2. Not a blocker, worth a guard if revisited.
