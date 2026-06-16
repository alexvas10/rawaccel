#include "SettingsManager.hpp"

#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <clocale>
#include <fstream>
#include <stdexcept>
#include <sys/stat.h>

namespace SettingsManager {

std::string userCachePath() {
    const char* home = std::getenv("HOME");
    if (!home) home = "/tmp";
    return std::string(home) + "/.config/rawaccel/settings.json";
}

std::string guiPrefsPath() {
    const char* home = std::getenv("HOME");
    if (!home) home = "/tmp";
    return std::string(home) + "/.config/rawaccel/gui.json";
}

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

static nlohmann::json accel_args_to_json(const rawaccel::accel_args& a) {
    nlohmann::json j;
    j["mode"]            = mode_to_string(a.mode);
    j["Gain / Velocity"] = a.gain;
    j["inputOffset"]     = a.input_offset;
    j["outputOffset"]    = a.output_offset;
    j["acceleration"]    = a.acceleration;
    j["decayRate"]       = a.decay_rate;
    j["gamma"]           = a.gamma;
    j["motivity"]        = a.motivity;
    j["exponentClassic"] = a.exponent_classic;
    j["scale"]           = a.scale;
    j["exponentPower"]   = a.exponent_power;
    j["limit"]           = a.limit;
    j["syncSpeed"]       = a.sync_speed;
    j["smooth"]          = a.smooth;
    j["Cap / Jump"]      = {{"x", a.cap.x}, {"y", a.cap.y}};
    j["Cap mode"]        = cap_mode_to_string(a.cap_mode);

    if (a.mode == rawaccel::accel_mode::lookup && a.length > 0) {
        auto arr = nlohmann::json::array();
        for (int i = 0; i < a.length; ++i) arr.push_back(a.data[i]);
        j["data"] = arr;
    } else {
        j["data"] = nlohmann::json::array();
    }
    return j;
}

static rawaccel::accel_args accel_args_from_json(const nlohmann::json& j) {
    rawaccel::accel_args a{};
    if (j.contains("mode"))             a.mode = mode_from_string(j["mode"].get<std::string>());
    if (j.contains("Gain / Velocity"))  a.gain = j["Gain / Velocity"].get<bool>();
    if (j.contains("inputOffset"))      a.input_offset = j["inputOffset"].get<double>();
    if (j.contains("outputOffset"))     a.output_offset = j["outputOffset"].get<double>();
    if (j.contains("acceleration"))     a.acceleration = j["acceleration"].get<double>();
    if (j.contains("decayRate"))        a.decay_rate = j["decayRate"].get<double>();
    if (j.contains("gamma"))            a.gamma = j["gamma"].get<double>();
    if (j.contains("motivity"))         a.motivity = j["motivity"].get<double>();
    if (j.contains("exponentClassic"))  a.exponent_classic = j["exponentClassic"].get<double>();
    if (j.contains("scale"))            a.scale = j["scale"].get<double>();
    if (j.contains("exponentPower"))    a.exponent_power = j["exponentPower"].get<double>();
    if (j.contains("limit"))            a.limit = j["limit"].get<double>();
    if (j.contains("syncSpeed"))        a.sync_speed = j["syncSpeed"].get<double>();
    if (j.contains("smooth"))           a.smooth = j["smooth"].get<double>();
    if (j.contains("Cap / Jump")) {
        a.cap.x = j["Cap / Jump"].value("x", 15.0);
        a.cap.y = j["Cap / Jump"].value("y", 1.5);
    }
    if (j.contains("Cap mode"))
        a.cap_mode = cap_mode_from_string(j["Cap mode"].get<std::string>());
    if (j.contains("data") && j["data"].is_array()) {
        int cnt = std::min((int)j["data"].size(), (int)rawaccel::LUT_RAW_DATA_CAPACITY);
        for (int i = 0; i < cnt; ++i) a.data[i] = j["data"][i].get<float>();
        a.length = cnt;
    }
    return a;
}

nlohmann::json profileToJson(const rawaccel::modifier_settings& settings) {
    const auto& prof = settings.prof;
    char name_buf[rawaccel::MAX_NAME_LEN * 4] = {};
    std::wcstombs(name_buf, prof.name, sizeof(name_buf) - 1);

    nlohmann::json jp;
    jp["name"] = std::string(name_buf);
    jp["Stretches domain for horizontal vs vertical inputs"] = {{"x", prof.domain_weights.x}, {"y", prof.domain_weights.y}};
    jp["Stretches accel range for horizontal vs vertical inputs"] = {{"x", prof.range_weights.x}, {"y", prof.range_weights.y}};
    jp["Whole or horizontal accel parameters"] = accel_args_to_json(prof.accel_x);
    jp["Vertical accel parameters"]            = accel_args_to_json(prof.accel_y);
    jp["Input speed calculation parameters"]   = {
        {"Whole/combined accel (set false for 'by component' mode)", prof.speed_processor_args.whole},
        {"lpNorm", prof.speed_processor_args.lp_norm},
        {"Time in ms after which an input is weighted at half its original value.", prof.speed_processor_args.input_speed_smooth_halflife},
        {"Time in ms after which scale is weighted at half its original value.",    prof.speed_processor_args.scale_smooth_halflife},
        {"Time in ms after which an output is weighted at half its original value.", prof.speed_processor_args.output_speed_smooth_halflife},
    };
    jp["Output DPI"]                                        = prof.output_dpi;
    jp["Y/X output DPI ratio (vertical sens multiplier)"]  = prof.yx_output_dpi_ratio;
    jp["L/R output DPI ratio (left sens multiplier)"]      = prof.lr_output_dpi_ratio;
    jp["U/D output DPI ratio (up sens multiplier)"]        = prof.ud_output_dpi_ratio;
    jp["Degrees of rotation"]                               = prof.degrees_rotation;
    jp["Degrees of angle snapping"]                         = prof.degrees_snap;
    jp["Input Speed Cap"]                                   = prof.speed_max;

    nlohmann::json root;
    root["### Accel modes ###"] = "classic | jump | natural | synchronous | power | lut | noaccel";
    root["### Cap modes ###"]   = "in_out | input | output";
    root["version"]             = "1.7.0";
    root["defaultDeviceConfig"] = {{"disable",false},{"setExtraInfo",false},{"dpi",0},{"pollingRate",0}};
    root["profiles"]            = nlohmann::json::array({jp});
    root["devices"]             = nlohmann::json::array();
    return root;
}

rawaccel::modifier_settings profileFromJson(const nlohmann::json& j) {
    rawaccel::modifier_settings ms{};

    const nlohmann::json* cfg = &j;
    nlohmann::json driver_settings;
    if (j.contains("Driver settings")) {
        driver_settings = j["Driver settings"];
        cfg = &driver_settings;
    }

    if (!cfg->contains("profiles") || !(*cfg)["profiles"].is_array() || (*cfg)["profiles"].empty())
        return ms;

    const auto& jp = (*cfg)["profiles"][0];
    auto& prof = ms.prof;

    if (jp.contains("name")) utf8_to_wchar(prof.name, rawaccel::MAX_NAME_LEN, jp["name"].get<std::string>());

    const char* dom = "Stretches domain for horizontal vs vertical inputs";
    const char* rng = "Stretches accel range for horizontal vs vertical inputs";
    if (jp.contains(dom)) { prof.domain_weights.x = jp[dom].value("x",1.0); prof.domain_weights.y = jp[dom].value("y",1.0); }
    if (jp.contains(rng)) { prof.range_weights.x  = jp[rng].value("x",1.0); prof.range_weights.y  = jp[rng].value("y",1.0); }
    if (jp.contains("domainXY")) { prof.domain_weights.x = jp["domainXY"].value("x",1.0); prof.domain_weights.y = jp["domainXY"].value("y",1.0); }
    if (jp.contains("rangeXY"))  { prof.range_weights.x  = jp["rangeXY"].value("x",1.0);  prof.range_weights.y  = jp["rangeXY"].value("y",1.0); }

    if (jp.contains("Whole or horizontal accel parameters")) prof.accel_x = accel_args_from_json(jp["Whole or horizontal accel parameters"]);
    if (jp.contains("Vertical accel parameters"))            prof.accel_y = accel_args_from_json(jp["Vertical accel parameters"]);

    if (jp.contains("Input speed calculation parameters")) {
        const auto& sp = jp["Input speed calculation parameters"];
        prof.speed_processor_args.whole    = sp.value("Whole/combined accel (set false for 'by component' mode)", true);
        prof.speed_processor_args.lp_norm  = sp.value("lpNorm", 2.0);
    }

    prof.output_dpi          = jp.value("Output DPI", rawaccel::NORMALIZED_DPI);
    prof.yx_output_dpi_ratio = jp.value("Y/X output DPI ratio (vertical sens multiplier)", 1.0);
    prof.lr_output_dpi_ratio = jp.value("L/R output DPI ratio (left sens multiplier)", 1.0);
    prof.ud_output_dpi_ratio = jp.value("U/D output DPI ratio (up sens multiplier)", 1.0);
    prof.degrees_rotation    = jp.value("Degrees of rotation", 0.0);
    prof.degrees_snap        = jp.value("Degrees of angle snapping", 0.0);
    prof.speed_max           = jp.value("Input Speed Cap", 0.0);

    rawaccel::init_data(ms);
    return ms;
}

nlohmann::json loadJson(const std::string& path) {
    std::ifstream f(path);
    if (!f) return {};
    try {
        std::string content((std::istreambuf_iterator<char>(f)), {});
        return nlohmann::json::parse(content);
    } catch (...) {
        return {};
    }
}

void saveJson(const std::string& path, const nlohmann::json& j) {
    auto slash = path.rfind('/');
    if (slash != std::string::npos) mkdir(path.substr(0, slash).c_str(), 0755);
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write: " + path);
    f << j.dump(2);
}

} // namespace SettingsManager
