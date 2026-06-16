#include "SettingsLoader.hpp"

#include <cstring>
#include <cwchar>
#include <fstream>
#include <stdexcept>
#include <sys/stat.h>

namespace rawaccel_linux {

static void utf8_to_wchar(wchar_t* dst, size_t n, const std::string& src) {
    std::mbstowcs(dst, src.c_str(), n - 1);
    dst[n - 1] = L'\0';
}

static rawaccel::accel_mode mode_from_string(const std::string& s) {
    if (s == "classic")     return rawaccel::accel_mode::classic;
    if (s == "jump")        return rawaccel::accel_mode::jump;
    if (s == "natural")     return rawaccel::accel_mode::natural;
    if (s == "synchronous") return rawaccel::accel_mode::synchronous;
    if (s == "power")       return rawaccel::accel_mode::power;
    if (s == "lut")         return rawaccel::accel_mode::lookup;
    if (s == "lookup")      return rawaccel::accel_mode::lookup;
    return rawaccel::accel_mode::noaccel;
}

static std::string mode_to_string(rawaccel::accel_mode m) {
    switch (m) {
        case rawaccel::accel_mode::classic:     return "classic";
        case rawaccel::accel_mode::jump:        return "jump";
        case rawaccel::accel_mode::natural:     return "natural";
        case rawaccel::accel_mode::synchronous: return "synchronous";
        case rawaccel::accel_mode::power:       return "power";
        case rawaccel::accel_mode::lookup:      return "lut";
        default:                                return "noaccel";
    }
}

static rawaccel::cap_mode cap_mode_from_string(const std::string& s) {
    if (s == "in_out") return rawaccel::cap_mode::io;
    if (s == "input")  return rawaccel::cap_mode::in;
    return rawaccel::cap_mode::out;
}

static std::string cap_mode_to_string(rawaccel::cap_mode m) {
    switch (m) {
        case rawaccel::cap_mode::io:  return "in_out";
        case rawaccel::cap_mode::in:  return "input";
        default:                      return "output";
    }
}

static rawaccel::accel_args accel_args_from_json(const nlohmann::json& j) {
    rawaccel::accel_args a{};

    if (j.contains("mode"))         a.mode = mode_from_string(j["mode"].get<std::string>());
    if (j.contains("Gain / Velocity")) a.gain = j["Gain / Velocity"].get<bool>();
    if (j.contains("inputOffset"))  a.input_offset = j["inputOffset"].get<double>();
    if (j.contains("outputOffset")) a.output_offset = j["outputOffset"].get<double>();
    if (j.contains("acceleration")) a.acceleration = j["acceleration"].get<double>();
    if (j.contains("decayRate"))    a.decay_rate = j["decayRate"].get<double>();
    if (j.contains("gamma"))        a.gamma = j["gamma"].get<double>();
    if (j.contains("motivity"))     a.motivity = j["motivity"].get<double>();
    if (j.contains("exponentClassic")) a.exponent_classic = j["exponentClassic"].get<double>();
    if (j.contains("scale"))        a.scale = j["scale"].get<double>();
    if (j.contains("exponentPower")) a.exponent_power = j["exponentPower"].get<double>();
    if (j.contains("limit"))        a.limit = j["limit"].get<double>();
    if (j.contains("syncSpeed"))    a.sync_speed = j["syncSpeed"].get<double>();
    if (j.contains("smooth"))       a.smooth = j["smooth"].get<double>();

    if (j.contains("Cap / Jump")) {
        auto& cap = j["Cap / Jump"];
        a.cap.x = cap.value("x", 15.0);
        a.cap.y = cap.value("y", 1.5);
    }

    if (j.contains("Cap mode"))
        a.cap_mode = cap_mode_from_string(j["Cap mode"].get<std::string>());

    if (j.contains("data") && j["data"].is_array()) {
        auto& arr = j["data"];
        int count = static_cast<int>(arr.size());
        if (count > static_cast<int>(rawaccel::LUT_RAW_DATA_CAPACITY))
            count = static_cast<int>(rawaccel::LUT_RAW_DATA_CAPACITY);
        for (int i = 0; i < count; ++i)
            a.data[i] = arr[i].get<float>();
        a.length = count;
    }

    return a;
}

static rawaccel::speed_args speed_args_from_json(const nlohmann::json& j) {
    rawaccel::speed_args s{};
    const char* whole_key = "Whole/combined accel (set false for 'by component' mode)";
    if (j.contains(whole_key))  s.whole = j[whole_key].get<bool>();
    if (j.contains("lpNorm"))   s.lp_norm = j["lpNorm"].get<double>();

    const char* in_key  = "Time in ms after which an input is weighted at half its original value.";
    const char* sc_key  = "Time in ms after which scale is weighted at half its original value.";
    const char* out_key = "Time in ms after which an output is weighted at half its original value.";
    if (j.contains(in_key))  s.input_speed_smooth_halflife = j[in_key].get<double>();
    if (j.contains(sc_key))  s.scale_smooth_halflife = j[sc_key].get<double>();
    if (j.contains(out_key)) s.output_speed_smooth_halflife = j[out_key].get<double>();
    return s;
}

static rawaccel::device_config device_config_from_json(const nlohmann::json& j) {
    rawaccel::device_config c{};
    if (j.contains("disable"))      c.disable = j["disable"].get<bool>();
    if (j.contains("setExtraInfo")) c.set_extra_info = j["setExtraInfo"].get<bool>();

    const char* ptl_key = "Use constant time interval based on polling rate";
    if (j.contains(ptl_key)) c.poll_time_lock = j[ptl_key].get<bool>();

    const char* dpi_key = "DPI (normalizes input speed unit: counts/ms -> in/s)";
    const char* pr_key  = "Polling rate Hz (keep at 0 for automatic adjustment)";
    if (j.contains(dpi_key)) c.dpi = j[dpi_key].get<int>();
    if (j.contains(pr_key))  c.polling_rate = j[pr_key].get<int>();
    // legacy short keys
    if (j.contains("dpi"))         c.dpi = j["dpi"].get<int>();
    if (j.contains("pollingRate")) c.polling_rate = j["pollingRate"].get<int>();

    if (j.contains("minimumTime")) c.clamp.min = j["minimumTime"].get<double>();
    if (j.contains("maximumTime")) c.clamp.max = j["maximumTime"].get<double>();
    return c;
}

static nlohmann::json accel_args_to_json(const rawaccel::accel_args& a) {
    nlohmann::json j;
    j["mode"] = mode_to_string(a.mode);
    j["Gain / Velocity"] = a.gain;
    j["inputOffset"] = a.input_offset;
    j["outputOffset"] = a.output_offset;
    j["acceleration"] = a.acceleration;
    j["decayRate"] = a.decay_rate;
    j["gamma"] = a.gamma;
    j["motivity"] = a.motivity;
    j["exponentClassic"] = a.exponent_classic;
    j["scale"] = a.scale;
    j["exponentPower"] = a.exponent_power;
    j["limit"] = a.limit;
    j["syncSpeed"] = a.sync_speed;
    j["smooth"] = a.smooth;
    j["Cap / Jump"] = {{"x", a.cap.x}, {"y", a.cap.y}};
    j["Cap mode"] = cap_mode_to_string(a.cap_mode);

    if (a.mode == rawaccel::accel_mode::lookup && a.length > 0) {
        nlohmann::json arr = nlohmann::json::array();
        for (int i = 0; i < a.length; ++i)
            arr.push_back(a.data[i]);
        j["data"] = arr;
    } else {
        j["data"] = nlohmann::json::array();
    }
    return j;
}

static nlohmann::json speed_args_to_json(const rawaccel::speed_args& s) {
    nlohmann::json j;
    j["Whole/combined accel (set false for 'by component' mode)"] = s.whole;
    j["lpNorm"] = s.lp_norm;
    j["Time in ms after which an input is weighted at half its original value."] = s.input_speed_smooth_halflife;
    j["Time in ms after which scale is weighted at half its original value."]    = s.scale_smooth_halflife;
    j["Time in ms after which an output is weighted at half its original value."] = s.output_speed_smooth_halflife;
    return j;
}

static nlohmann::json device_config_to_json(const rawaccel::device_config& c) {
    nlohmann::json j;
    j["disable"] = c.disable;
    j["setExtraInfo"] = c.set_extra_info;
    j["Use constant time interval based on polling rate"] = c.poll_time_lock;
    j["DPI (normalizes input speed unit: counts/ms -> in/s)"] = c.dpi;
    j["Polling rate Hz (keep at 0 for automatic adjustment)"] = c.polling_rate;
    if (c.clamp.min != rawaccel::DEFAULT_TIME_MIN) j["minimumTime"] = c.clamp.min;
    if (c.clamp.max != rawaccel::DEFAULT_TIME_MAX) j["maximumTime"] = c.clamp.max;
    return j;
}

// ─── Public API ──────────────────────────────────────────────────────────────

LoadedSettings settings_from_json(const std::string& json_str,
                                  std::function<void(const std::string&)> err_cb)
{
    LoadedSettings out{};
    nlohmann::json root = nlohmann::json::parse(json_str);

    out.version = root.value("version", "1.7.0");

    // Handle both flat (Linux native) and legacy "Driver settings" wrapper
    const nlohmann::json* cfg = &root;
    nlohmann::json driver_settings;
    if (root.contains("Driver settings")) {
        driver_settings = root["Driver settings"];
        cfg = &driver_settings;
    }

    if (cfg->contains("defaultDeviceConfig"))
        out.default_device_cfg = device_config_from_json((*cfg)["defaultDeviceConfig"]);

    // Load first profile (we handle single-profile for now, like Windows default)
    rawaccel::profile& prof = out.modifier.prof;

    if (cfg->contains("profiles") && (*cfg)["profiles"].is_array() && !(*cfg)["profiles"].empty()) {
        const auto& jp = (*cfg)["profiles"][0];

        if (jp.contains("name")) {
            std::string n = jp["name"].get<std::string>();
            utf8_to_wchar(prof.name, rawaccel::MAX_NAME_LEN, n);
        }

        const char* dom_key = "Stretches domain for horizontal vs vertical inputs";
        const char* rng_key = "Stretches accel range for horizontal vs vertical inputs";
        if (jp.contains(dom_key)) {
            prof.domain_weights.x = jp[dom_key].value("x", 1.0);
            prof.domain_weights.y = jp[dom_key].value("y", 1.0);
        }
        if (jp.contains(rng_key)) {
            prof.range_weights.x = jp[rng_key].value("x", 1.0);
            prof.range_weights.y = jp[rng_key].value("y", 1.0);
        }
        // Also support short form "domainXY" / "rangeXY"
        if (jp.contains("domainXY")) {
            prof.domain_weights.x = jp["domainXY"].value("x", 1.0);
            prof.domain_weights.y = jp["domainXY"].value("y", 1.0);
        }
        if (jp.contains("rangeXY")) {
            prof.range_weights.x = jp["rangeXY"].value("x", 1.0);
            prof.range_weights.y = jp["rangeXY"].value("y", 1.0);
        }

        const char* ax_key = "Whole or horizontal accel parameters";
        const char* ay_key = "Vertical accel parameters";
        const char* sp_key = "Input speed calculation parameters";
        if (jp.contains(ax_key)) prof.accel_x = accel_args_from_json(jp[ax_key]);
        if (jp.contains(ay_key)) prof.accel_y = accel_args_from_json(jp[ay_key]);
        if (jp.contains(sp_key)) prof.speed_processor_args = speed_args_from_json(jp[sp_key]);

        prof.output_dpi           = jp.value("Output DPI", rawaccel::NORMALIZED_DPI);
        prof.yx_output_dpi_ratio  = jp.value("Y/X output DPI ratio (vertical sens multiplier)", 1.0);
        prof.lr_output_dpi_ratio  = jp.value("L/R output DPI ratio (left sens multiplier)", 1.0);
        prof.ud_output_dpi_ratio  = jp.value("U/D output DPI ratio (up sens multiplier)", 1.0);
        prof.degrees_rotation     = jp.value("Degrees of rotation", 0.0);
        prof.degrees_snap         = jp.value("Degrees of angle snapping", 0.0);
        prof.speed_max            = jp.value("Input Speed Cap", 0.0);
    }

    if (cfg->contains("devices") && (*cfg)["devices"].is_array()) {
        for (const auto& jd : (*cfg)["devices"]) {
            rawaccel::device_settings ds{};
            if (jd.contains("name")) {
                auto n = jd["name"].get<std::string>();
                utf8_to_wchar(ds.name, rawaccel::MAX_NAME_LEN, n);
            }
            if (jd.contains("profile")) {
                auto p = jd["profile"].get<std::string>();
                utf8_to_wchar(ds.profile, rawaccel::MAX_NAME_LEN, p);
            }
            if (jd.contains("id")) {
                auto id = jd["id"].get<std::string>();
                utf8_to_wchar(ds.id, rawaccel::MAX_DEV_ID_LEN, id);
            }
            if (jd.contains("config"))
                ds.config = device_config_from_json(jd["config"]);
            out.devices.push_back(ds);
        }
    }

    rawaccel::init_data(out.modifier);

    (void)err_cb;
    return out;
}

std::string settings_to_json(const LoadedSettings& s) {
    const auto& prof = s.modifier.prof;

    // Reconstruct profile name from wchar_t
    char name_buf[rawaccel::MAX_NAME_LEN * 4] = {};
    std::wcstombs(name_buf, prof.name, sizeof(name_buf) - 1);

    nlohmann::json profile;
    profile["name"] = std::string(name_buf);
    profile["Stretches domain for horizontal vs vertical inputs"] = {
        {"x", prof.domain_weights.x}, {"y", prof.domain_weights.y}
    };
    profile["Stretches accel range for horizontal vs vertical inputs"] = {
        {"x", prof.range_weights.x}, {"y", prof.range_weights.y}
    };
    profile["Whole or horizontal accel parameters"] = accel_args_to_json(prof.accel_x);
    profile["Vertical accel parameters"]            = accel_args_to_json(prof.accel_y);
    profile["Input speed calculation parameters"]   = speed_args_to_json(prof.speed_processor_args);
    profile["Output DPI"]                                          = prof.output_dpi;
    profile["Y/X output DPI ratio (vertical sens multiplier)"]    = prof.yx_output_dpi_ratio;
    profile["L/R output DPI ratio (left sens multiplier)"]        = prof.lr_output_dpi_ratio;
    profile["U/D output DPI ratio (up sens multiplier)"]          = prof.ud_output_dpi_ratio;
    profile["Degrees of rotation"]                                 = prof.degrees_rotation;
    profile["Degrees of angle snapping"]                           = prof.degrees_snap;
    profile["Input Speed Cap"]                                     = prof.speed_max;

    nlohmann::json devices = nlohmann::json::array();
    for (const auto& ds : s.devices) {
        char nbuf[rawaccel::MAX_NAME_LEN * 4] = {};
        char pbuf[rawaccel::MAX_NAME_LEN * 4] = {};
        char ibuf[rawaccel::MAX_DEV_ID_LEN * 4] = {};
        std::wcstombs(nbuf, ds.name,    sizeof(nbuf) - 1);
        std::wcstombs(pbuf, ds.profile, sizeof(pbuf) - 1);
        std::wcstombs(ibuf, ds.id,      sizeof(ibuf) - 1);
        nlohmann::json jd;
        jd["name"]    = std::string(nbuf);
        jd["profile"] = std::string(pbuf);
        jd["id"]      = std::string(ibuf);
        jd["config"]  = device_config_to_json(ds.config);
        devices.push_back(jd);
    }

    nlohmann::json root;
    root["### Accel modes ###"] = "classic | jump | natural | synchronous | power | lut | noaccel";
    root["### Cap modes ###"]   = "in_out | input | output";
    root["version"]             = s.version.empty() ? "1.7.0" : s.version;
    root["defaultDeviceConfig"] = device_config_to_json(s.default_device_cfg);
    root["profiles"]            = nlohmann::json::array({profile});
    root["devices"]             = devices;

    return root.dump(2);
}

LoadedSettings load_settings_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        // Return defaults
        LoadedSettings s{};
        s.version = "1.7.0";
        rawaccel::init_data(s.modifier);
        return s;
    }
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    return settings_from_json(content);
}

void save_settings_file(const std::string& path, const LoadedSettings& s) {
    // Create parent directory
    auto slash = path.rfind('/');
    if (slash != std::string::npos) {
        std::string dir = path.substr(0, slash);
        mkdir(dir.c_str(), 0755);
    }
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write settings file: " + path);
    f << settings_to_json(s);
}

} // namespace rawaccel_linux
