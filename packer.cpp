// packer.cpp — упаковка midi/ и soundfonts/ в resources.pak (со сжатием zlib)
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdint>
#include <algorithm>
#include <zlib.h>

namespace fs = std::filesystem;

static void put_u32(std::ofstream& f, uint32_t v) {
    f.write(reinterpret_cast<const char*>(&v), 4);
}

static std::vector<char> read_file(const fs::path& p) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f) return {};
    auto sz = f.tellg();
    f.seekg(0);
    std::vector<char> buf((size_t)sz);
    f.read(buf.data(), sz);
    return buf;
}

// Сжатие zlib. Возвращает true, если сжатие дало выигрыш.
static bool try_compress(const std::vector<char>& in,
                         std::vector<char>& out,
                         uLong& out_size) {
    uLong bound = compressBound((uLong)in.size());
    out.resize(bound);
    out_size = bound;                        // <— ЭТОЙ СТРОКИ НЕ ХВАТАЛО
    int rc = compress2((Bytef*)out.data(), &out_size,
                       (const Bytef*)in.data(), (uLong)in.size(),
                       Z_BEST_COMPRESSION);
    if (rc != Z_OK) return false;
    if (out_size >= in.size()) return false;
    out.resize(out_size);
    return true;
}

static void scan_dir(const std::string& dir, const std::string& prefix,
                     std::vector<std::pair<std::string, std::vector<char>>>& out) {
    fs::path base(dir);
    if (!fs::exists(base)) {
        std::cerr << "WARNING: folder not found: " << dir << "\n";
        return;
    }
    std::string base_str = base.generic_string();
    if (!base_str.empty() && base_str.back() != '/') base_str += '/';

    for (auto& e : fs::recursive_directory_iterator(base)) {
        if (!e.is_regular_file()) continue;
        std::string full = e.path().generic_string();
        std::string rel;
        if (full.rfind(base_str, 0) == 0)
            rel = full.substr(base_str.size());
        else
            rel = e.path().filename().generic_string();
        std::string name = prefix + rel;
        auto data = read_file(e.path());
        if (!data.empty()) out.emplace_back(name, std::move(data));
    }
}

int main() {
    std::vector<std::pair<std::string, std::vector<char>>> files;

    scan_dir("midi",       "midi/",       files);
    scan_dir("soundfonts", "soundfonts/", files);
    scan_dir("banks/adl",  "banks/adl/",  files);
    scan_dir("banks/opn",  "banks/opn/",  files);

    std::sort(files.begin(), files.end(),
              [](auto& a, auto& b){ return a.first < b.first; });

    std::ofstream out("resources.pak", std::ios::binary);
    if (!out) { std::cerr << "Cannot create resources.pak\n"; return 1; }

    out.write("MPAK", 4);
    put_u32(out, 2);                                // версия формата = 2 (с сжатием)
    put_u32(out, (uint32_t)files.size());

    uint64_t total_raw = 0, total_packed = 0;
    int compressed_count = 0;

    for (auto& [name, raw] : files) {
        std::vector<char> packed;
        uLong packed_size = 0;
        bool is_compressed = try_compress(raw, packed, packed_size);

        const char* data_ptr = is_compressed ? packed.data() : raw.data();
        uint32_t  data_size  = is_compressed ? (uint32_t)packed_size
                                             : (uint32_t)raw.size();

        put_u32(out, (uint32_t)name.size());
        out.write(name.data(), name.size());
        put_u32(out, (uint32_t)raw.size());          // исходный размер
        put_u32(out, data_size);                     // размер в pak
        uint8_t flag = is_compressed ? 1 : 0;
        out.write(reinterpret_cast<const char*>(&flag), 1);
        out.write(data_ptr, data_size);

        total_raw    += raw.size();
        total_packed += data_size + 13 + name.size();
        if (is_compressed) ++compressed_count;

        double ratio = raw.empty() ? 0.0
                                   : 100.0 * (1.0 - (double)data_size / raw.size());
        std::cout << "  " << (is_compressed ? "[z] " : "[  ] ")
                  << name << "  " << raw.size() << " -> " << data_size
                  << " (" << (int)ratio << "% saved)\n";
    }

    std::cout << "\nDone: resources.pak\n";
    std::cout << "  files:        " << files.size() << "\n";
    std::cout << "  compressed:   " << compressed_count << " / " << files.size() << "\n";
    std::cout << "  raw total:    " << total_raw    << " bytes\n";
    std::cout << "  packed total: " << total_packed << " bytes\n";
    double saved = total_raw ? 100.0 * (1.0 - (double)total_packed / total_raw) : 0.0;
    std::cout << "  overall:      " << (int)saved << "% saved\n";
    return 0;
}