#pragma once
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

inline std::vector<Decoded> decode(const dbc::Body& body,
                                   const uint8_t* data, size_t size) {
    std::vector<Decoded> out;
    out.reserve(body.signals.size());
    for (const auto& s : body.signals) {
        const uint64_t raw = raw_of(s, data, size);
        out.push_back({&s, raw, physical_of(s, raw)});
    }
    return out;
}

} // namespace can
