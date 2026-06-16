#include "VirtualMouse.hpp"

#include <cstring>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/ioctl.h>

namespace rawaccel_linux {

VirtualMouse::VirtualMouse(const std::string& name) {
    fd_ = open("/dev/uinput", O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd_ < 0)
        throw std::runtime_error("Cannot open /dev/uinput — is the uinput kernel module loaded?");

    auto require = [this](int ret, const char* msg) {
        if (ret < 0) throw std::runtime_error(std::string(msg) + ": ioctl failed");
    };

    // Enable event types
    require(ioctl(fd_, UI_SET_EVBIT, EV_REL), "EV_REL");
    require(ioctl(fd_, UI_SET_EVBIT, EV_KEY), "EV_KEY");
    require(ioctl(fd_, UI_SET_EVBIT, EV_SYN), "EV_SYN");

    // Relative axes
    for (int code : {REL_X, REL_Y, REL_WHEEL, REL_HWHEEL, REL_WHEEL_HI_RES, REL_HWHEEL_HI_RES})
        require(ioctl(fd_, UI_SET_RELBIT, code), "REL bit");

    // Mouse buttons
    for (int code : {BTN_LEFT, BTN_RIGHT, BTN_MIDDLE, BTN_SIDE, BTN_EXTRA, BTN_FORWARD, BTN_BACK})
        require(ioctl(fd_, UI_SET_KEYBIT, code), "KEY bit");

    struct uinput_setup setup{};
    setup.id.bustype = BUS_VIRTUAL;
    setup.id.vendor  = VENDOR_ID;
    setup.id.product = PRODUCT_ID;
    setup.id.version = 1;
    strncpy(setup.name, name.c_str(), UINPUT_MAX_NAME_SIZE - 1);

    require(ioctl(fd_, UI_DEV_SETUP, &setup), "UI_DEV_SETUP");
    require(ioctl(fd_, UI_DEV_CREATE), "UI_DEV_CREATE");
}

VirtualMouse::~VirtualMouse() {
    if (fd_ >= 0) {
        ioctl(fd_, UI_DEV_DESTROY);
        close(fd_);
    }
}

void VirtualMouse::emit_raw(unsigned short type, unsigned short code, int value) {
    struct input_event ev{};
    ev.type  = type;
    ev.code  = code;
    ev.value = value;
    write(fd_, &ev, sizeof(ev));
}

void VirtualMouse::emit_motion(int dx, int dy) {
    if (dx) emit_raw(EV_REL, REL_X, dx);
    if (dy) emit_raw(EV_REL, REL_Y, dy);
    sync();
}

void VirtualMouse::emit_event(unsigned short type, unsigned short code, int value) {
    emit_raw(type, code, value);
    if (type == EV_SYN) return; // don't double-sync
}

void VirtualMouse::sync() {
    emit_raw(EV_SYN, SYN_REPORT, 0);
}

} // namespace rawaccel_linux
