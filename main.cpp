#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>

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

struct Stats {
    size_t n = 0;
    double min = 0.0, max = 0.0, sum = 0.0;
};

void run(const std::map<uint64_t, dbc::Body>& db, std::istream& in,
         const std::unordered_set<std::string>& filter, bool do_stats) {
    std::string line;
    size_t unknown = 0, unparsed = 0;
    std::unordered_map<std::string, Stats> stats;
    while (std::getline(in, line)) {
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
            if (!filter.empty() && !filter.count(d.signal->name)) continue;
            std::cout << std::format("{:.6f},{},{:g}\n", frame->timestamp,
                                     d.signal->name, d.value);
            if (do_stats) {
                auto& s = stats[d.signal->name];
                if (s.n == 0) { s.min = s.max = d.value; }
                else { if (d.value < s.min) s.min = d.value; if (d.value > s.max) s.max = d.value; }
                s.sum += d.value;
                ++s.n;
            }
        }
    }
    if (unknown || unparsed)
        std::cerr << std::format("({} frames not in dbc, {} lines unparsed)\n",
                                 unknown, unparsed);
    if (do_stats) {
        for (const auto& [name, s] : stats) {
            std::cerr << std::format("{:<20} n={:<6} min={:.1f}  max={:.1f}  mean={:.1f}\n",
                                     name, s.n, s.min, s.max,
                                     s.n ? s.sum / static_cast<double>(s.n) : 0.0);
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    std::unordered_set<std::string> filter;
    std::string log_path;
    std::string dbc_path;
    bool do_stats = false;

    // parse args: tool <dbc> [log] [-s sig1,sig2,...] [--stats]
    int i = 1;
    for (; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "-s") {
            if (++i >= argc) {
                std::cerr << "error: -s requires an argument\n";
                return 2;
            }
            std::string sigs = argv[i];
            size_t pos = 0;
            while (pos < sigs.size()) {
                const size_t comma = sigs.find(',', pos);
                const size_t end = (comma == std::string::npos) ? sigs.size() : comma;
                filter.insert(sigs.substr(pos, end - pos));
                pos = (comma == std::string::npos) ? sigs.size() : comma + 1;
            }
        } else if (arg == "--stats") {
            do_stats = true;
        } else if (dbc_path.empty()) {
            dbc_path = std::string(arg);
        } else {
            log_path = std::string(arg);
        }
    }

    if (dbc_path.empty()) {
        std::cerr << std::format("usage: {} <file.dbc> [candump.log|-] [-s sig,...] [--stats]\n",
                                 argv[0]);
        return 2;
    }

    try {
        const auto db = dbc::parse(dbc_path);
        if (log_path.empty()) {
            list(db);
            return 0;
        }

        if (log_path == "-") {
            run(db, std::cin, filter, do_stats);
            return 0;
        }
        std::ifstream log(log_path);
        if (!log) throw std::runtime_error("can't open " + log_path);
        run(db, log, filter, do_stats);
    } catch (const std::exception& e) {
        std::cerr << std::format("error: {}\n", e.what());
        return 1;
    }
}
