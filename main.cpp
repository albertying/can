#include <format>
#include <fstream>
#include <iostream>
#include <string>

#include "candump.hpp"
#include "dbc.hpp"
#include "decode.hpp"

namespace {

void list(const std::map<uint64_t, dbc::Body>& db) {
    for (const auto& [k, bd] : db) {
        std::cout << std::format("0x{:X}{} {} ({} bytes)\n", bd.frame_id,
                                 bd.extended ? "x" : "", bd.name, bd.length);
        for (const auto& sg : bd.signals)
            std::cout << std::format("  {:<18} {:>2}|{:<2} @{}{}  *{:g} {:+g} {}\n",
                                     sg.name, sg.start_bit, sg.length,
                                     sg.l_endian ? '1' : '0',
                                     sg.is_signed ? '-' : '+',
                                     sg.scale, sg.offset, sg.unit);
    }
}

void run(const std::map<uint64_t, dbc::Body>& db, std::istream& in) {
    std::string line;
    size_t unknown = 0, unparsed = 0, line_num = 0;
    while (std::getline(in, line)) {
        ++line_num;
        if (line.empty()) continue;
        const auto frame = can::parse_line(line);
        if (!frame) {
            ++unparsed;
            continue;
        }
        if (frame->remote) continue;
        const auto it = db.find(dbc::key(frame->id, frame->extended));
        if (it == db.end()) {
            ++unknown;
            continue;
        }
        for (const auto& d : can::decode(it->second, frame->data, frame->length)) {
            std::cout << std::format("{},{},{:g}\n", line_num, d.signal->name,
                                     d.value);
        }
    }
    if (unknown || unparsed)
        std::cerr << std::format("({} frames not in dbc, {} lines unparsed)\n",
                                 unknown, unparsed);
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) {
        std::cerr << std::format("usage: {} <file.dbc> [candump.log|-]\n", argv[0]);
        return 2;
    }
    try {
        const auto db = dbc::parse(argv[1]);
        if (argc == 2) {
            list(db);
            return 0;
        }

        std::string log_path = argv[2];
        if (log_path == "-") {
            run(db, std::cin);
            return 0;
        }
        std::ifstream log(log_path);
        if (!log) throw std::runtime_error("can't open " + log_path);
        run(db, log);
    } catch (const std::exception& e) {
        std::cerr << std::format("error: {}\n", e.what());
        return 1;
    }
}
