#pragma once

#include "shared/compat.hpp"
#include "rawaccel.hpp"
#include <nlohmann/json.hpp>
#include <string>

// Serializes/deserializes GUI-side settings (Windows-compatible JSON).
// Separate from the daemon's SettingsLoader (avoids linking daemon into GUI).
namespace SettingsManager {

static constexpr const char* SYSTEM_SETTINGS = "/etc/rawaccel/settings.json";
static constexpr const char* USER_CACHE_PATH = nullptr; // resolved at runtime

// Returns the user cache path (~/.config/rawaccel/settings.json)
std::string userCachePath();
std::string guiPrefsPath();

// Convert C++ profile → Windows-compatible JSON object
nlohmann::json profileToJson(const rawaccel::modifier_settings& settings);

// Convert Windows-compatible JSON → C++ profile
rawaccel::modifier_settings profileFromJson(const nlohmann::json& j);

// Load from file (tries system path first, falls back to user cache)
nlohmann::json loadJson(const std::string& path);

// Save to file
void saveJson(const std::string& path, const nlohmann::json& j);

} // namespace SettingsManager
