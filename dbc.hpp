#pragma once
#include <cstdint>
#include <fstream>
#include <map>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

namespace dbc {

struct Signal {
    std::string name;
    std::string multiplexer;
    unsigned start_bit = 0;
    unsigned length = 0;
    bool l_endian = true;
    bool is_signed = false;
    double scale = 1.0;
    double offset = 0.0;
    std::string unit;
};

struct Body {
    uint32_t frame_id = 0; // 11 or 29
    bool extended = false; // if frame_id is extended or not
    std::string name;
    unsigned length = 0;
    std::vector<Signal> signals;
};

// A standard 0x123 and an extended 0x123 are different frames on the wire,
// so the map key has to carry the flag too.
inline uint64_t key(uint32_t frame_id, bool extended) {
    return (static_cast<uint64_t>(extended) << 32) | frame_id;
}

static const std::regex kBo{R"(^BO_\s+(\d+)\s+(\w+)\s*:\s*(\d+))"};

static const std::regex kSg{
    R"RX(^SG_\s+(\w+)\s*(M|m\d+M?)?\s*:\s*)RX"   // 1 name, 2 mux
    R"RX((\d+)\|(\d+)@([01])([+-])\s*)RX"        // 3 start, 4 len, 5 order, 6 sign
    R"RX(\(([^,]+),([^)]+)\)\s*)RX"              // 7 scale, 8 offset
    R"RX(\[([^|]*)\|([^\]]*)\]\s*)RX"            // 9,10 min/max - ignored
    R"RX("([^"]*)")RX"                           // 11 unit
};

inline std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \n\r\t\f\v");
    if (b == std::string::npos) return {};
    const auto e = s.find_last_not_of(" \n\r\t\f\v");
    return s.substr(b, e - b + 1);
}

inline std::map<uint64_t, Body> parse(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("can't open " + path);
    }

    std::map<uint64_t, Body> db;

    Body* curr = nullptr;
    std::string line;
    size_t line_num = 0;

    while (std::getline(in, line)) {
        ++line_num;
        line = trim(line);
        std::smatch m;

        if (line.starts_with("BO_")) {
            if (!std::regex_search(line, m, kBo)) {
                throw std::runtime_error("bad body at line " + std::to_string(line_num));
            }

            const auto raw = static_cast<uint32_t>(std::stoul(m[1].str()));

            Body bd;

            bd.extended = (raw & 0x80000000u) != 0;
            bd.frame_id = bd.extended ? (raw & 0x1FFFFFFFu) : raw;
            bd.name = m[2].str();
            bd.length = static_cast<unsigned>(std::stoul(m[3].str()));

            const auto k = key(bd.frame_id, bd.extended);
            auto [it, inserted] = db.emplace(k, std::move(bd));
            if (!inserted) {
                throw std::runtime_error("duplicate id at line " + std::to_string(line_num));
            }

            curr = &(it->second);
        } else if (line.starts_with("SG_")) {
            if (!std::regex_search(line, m, kSg)) {
                throw std::runtime_error("bad signal at line " + std::to_string(line_num));
            }

            if (!curr) {
                throw std::runtime_error("signal before body at line " + std::to_string(line_num));
            }

            Signal sg;

            sg.name = m[1].str();
            sg.multiplexer = m[2].str();
            sg.start_bit = static_cast<unsigned>(std::stoul(m[3].str()));
            sg.length = static_cast<unsigned>(std::stoul(m[4].str()));
            sg.l_endian = m[5].str() == "1";
            sg.is_signed = m[6].str() == "-";
            sg.scale = std::stod(m[7].str());
            sg.offset = std::stod(m[8].str());
            sg.unit = m[11].str();

            unsigned byte_position = sg.start_bit / 8;
            unsigned bit_position = sg.start_bit % 8;
            unsigned room = 0;
            if (byte_position < curr->length) {
                room = sg.l_endian ? curr->length * 8 - sg.start_bit //
                                   : bit_position + 1 + (curr->length - byte_position - 1) * 8;
            }

            if (sg.length == 0 || sg.length > room) {
                throw std::runtime_error("signal overruns at line " + std::to_string(line_num));
            }

            curr->signals.push_back(std::move(sg));
        }
    }
    return db;
}

} // namespace dbc