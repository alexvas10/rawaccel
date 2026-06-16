#pragma once

#include "shared/compat.hpp"
#include "rawaccel.hpp"

#include <functional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace rawaccel_linux {

struct LoadedSettings {
    rawaccel::modifier_settings modifier;
    rawaccel::device_config default_device_cfg;
    std::vector<rawaccel::device_settings> devices;
    std::string version;
};

// Load settings from a JSON string (Windows-compatible format).
// On error, calls err_cb with a message and throws std::runtime_error.
LoadedSettings settings_from_json(const std::string& json_str,
                                  std::function<void(const std::string&)> err_cb = nullptr);

// Serialize settings back to Windows-compatible JSON string.
std::string settings_to_json(const LoadedSettings& s);

// Load from file path; returns default settings if file doesn't exist.
LoadedSettings load_settings_file(const std::string& path);

// Save to file path; creates directory if needed.
void save_settings_file(const std::string& path, const LoadedSettings& s);

} // namespace rawaccel_linux
