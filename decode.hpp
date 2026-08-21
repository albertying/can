#pragma once
#include <charconv>
#include <optional>
#include <vector>

#include "bits.hpp"
#include "dbc.hpp"

namespace can {

struct Decoded {
    const dbc::Signal* signal = nullptr;
    uint64_t raw = 0;
    double value = 0.0;
};

inline uint64_t raw_of(const dbc::Signal& s, const uint8_t* data, size_t size) {
    return s.l_endian ? extract_le(data, size, s.start_bit, s.length)
                      : extract_be(data, size, s.start_bit, s.length);
}

inline double physical_of(const dbc::Signal& s, uint64_t raw) {
    const double v = s.is_signed
                         ? static_cast<double>(sign_extend(raw, s.length))
                         : static_cast<double>(raw);
    return v * s.scale + s.offset;
}

inline bool is_selector(const dbc::Signal& s) {
    return s.multiplexer == "M" ||
           (s.multiplexer.size() > 1 && s.multiplexer.back() == 'M' &&
            s.multiplexer.front() == 'm');
}

inline std::optional<unsigned> mux_group(const dbc::Signal& s) {
    if (s.multiplexer.size() < 2 || s.multiplexer.front() != 'm') return std::nullopt;
    const char* b = s.multiplexer.data() + 1;
    const char* e = s.multiplexer.data() + s.multiplexer.size();
    if (e[-1] == 'M') --e;
    unsigned out = 0;
    if (std::from_chars(b, e, out).ec != std::errc{}) return std::nullopt;
    return out;
}

inline std::vector<Decoded> decode(const dbc::Body& body,
                                   const uint8_t* data, size_t size) {
    std::optional<unsigned> selected;
    for (const auto& s : body.signals) {
        if (is_selector(s) && !mux_group(s)) {
            selected = static_cast<unsigned>(raw_of(s, data, size));
            break;
        }
    }

    std::vector<Decoded> out;
    out.reserve(body.signals.size());
    for (const auto& s : body.signals) {
        if (const auto g = mux_group(s); g && (!selected || *g != *selected)) {
            continue;
        }
        const uint64_t raw = raw_of(s, data, size);
        out.push_back({&s, raw, physical_of(s, raw)});
    }
    return out;
}

} // namespace can