#pragma once

#include <string>
#include <linux/uinput.h>

namespace rawaccel_linux {

class VirtualMouse {
public:
    // Identifies our own emitted device so DeviceManager can exclude it from input sources
    // (otherwise it would grab and read back the events it just wrote, looping forever).
    static constexpr unsigned short VENDOR_ID  = 0x1234;
    static constexpr unsigned short PRODUCT_ID = 0x5678;

    explicit VirtualMouse(const std::string& name = "rawaccel");
    ~VirtualMouse();

    VirtualMouse(const VirtualMouse&) = delete;
    VirtualMouse& operator=(const VirtualMouse&) = delete;

    // Emit relative motion (post-acceleration).
    void emit_motion(int dx, int dy);

    // Pass-through: forward a raw input_event verbatim.
    void emit_event(unsigned short type, unsigned short code, int value);

    void sync();

private:
    int fd_ = -1;

    void emit_raw(unsigned short type, unsigned short code, int value);
};

} // namespace rawaccel_linux
