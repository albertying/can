#pragma once
#include <cstdint>
#include <cstddef>

namespace can {

inline uint64_t load_le64(const uint8_t* data, size_t size) {
    uint64_t value = 0;
    for (size_t i = 0; i < size && i < 8; ++i)
        value |= static_cast<uint64_t>(data[i]) << (i * 8);
    return value;
}

inline uint64_t load_be64(const uint8_t* data, size_t size) {
    uint64_t value = 0;
    for (size_t i = 0; i < size && i < 8; ++i)
        value |= static_cast<uint64_t>(data[i]) << (8 * (7 - i));
    return value;
}

inline uint64_t extract_le(const uint8_t* data, size_t size,
                            unsigned start, unsigned length) {
    if (start >= 64 || length == 0 || length > 64) return 0;
    uint64_t loaded = load_le64(data, size) >> start;
    uint64_t mask = (length >= 64) ? ~0ULL : ((1ULL << length) - 1);
    return loaded & mask;
}

inline uint64_t extract_be(const uint8_t* data, size_t size,
                            unsigned start, unsigned length) {
    if (start >= 64 || length == 0 || length > 64) return 0;

    uint64_t loaded = load_be64(data, size);

    size_t byte_position = start / 8;
    size_t bit_position  = start % 8;
    size_t physical_msb  = (7 - byte_position) * 8 + bit_position;

    if (length > physical_msb + 1) return 0;

    size_t lsb_position = physical_msb - length + 1;
    loaded >>= lsb_position;

    uint64_t mask = (length >= 64) ? ~0ULL : ((1ULL << length) - 1);
    return loaded & mask;
}

} // namespace can
