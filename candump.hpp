#pragma once
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

namespace can {

struct Frame {
    double timestamp = 0.0;
    std::string interface;
    uint32_t id = 0;
    bool extended = false;
    bool remote = false;
    uint8_t data[8] = {};
    size_t length = 0;
};

inline std::optional<Frame> parse_line(std::string_view line) {
    const size_t open  = line.find('(');
    const size_t close = line.find(')');
    const size_t hash  = line.find('#');
    if (open == std::string_view::npos || close == std::string_view::npos ||
        hash == std::string_view::npos)
        return std::nullopt;

    Frame f;

    f.timestamp = std::strtod(std::string(line.substr(open + 1, close - open - 1)).c_str(),
                              nullptr);

    // between ')' and '#' is "  can0 123"
    std::string_view mid = line.substr(close + 1, hash - close - 1);
    const size_t sp = mid.find_last_of(" \t");
    if (sp == std::string_view::npos) return std::nullopt;

    std::string_view interface = mid.substr(0, sp);
    while (!interface.empty() && (interface.front() == ' ' || interface.front() == '\t'))
        interface.remove_prefix(1);
    f.interface = std::string(interface);

    const std::string_view id_str = mid.substr(sp + 1);
    const uint32_t raw_id = static_cast<uint32_t>(
        std::strtoul(std::string(id_str).c_str(), nullptr, 16));
    f.extended = raw_id > 0x7FFu;
    f.id = f.extended ? (raw_id & 0x1FFFFFFFu) : raw_id;

    std::string_view p = line.substr(hash + 1);
    if (!p.empty() && (p.front() == 'R' || p.front() == 'r')) {
        f.remote = true;
        return f;
    }

    const std::string bytes(p);
    for (size_t i = 0; i + 1 < bytes.size() && f.length < 8; i += 2) {
        f.data[f.length++] =
            static_cast<uint8_t>(std::strtoul(bytes.substr(i, 2).c_str(), nullptr, 16));
    }
    return (bytes.size() % 2 == 0) ? std::optional<Frame>(f) : std::nullopt;
}

} // namespace can