// Regression for sequenced FullBox flag reads on MSVC and GCC.
#include "MP4.AVCC.h"
#include "MP4.TFHD.h"
#include "MP4.TRUN.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

class MemoryStream : public MP4::BinaryStream {
public:
    explicit MemoryStream(std::initializer_list<uint8_t> data)
        : bytes(data), cursor(0) {}
    void read(char* output, std::streamsize count) override {
        if (count < 0 || static_cast<size_t>(count) > bytes.size() - cursor)
            throw std::runtime_error("read past fixture");
        std::memcpy(output, bytes.data() + cursor, static_cast<size_t>(count));
        cursor += static_cast<size_t>(count);
    }
    void ignore(std::streamsize count = 1) override {
        if (count < 0 || static_cast<size_t>(count) > bytes.size() - cursor)
            throw std::runtime_error("skip past fixture");
        cursor += static_cast<size_t>(count);
    }
    size_t pos() override { return cursor; }
private:
    std::vector<uint8_t> bytes;
    size_t cursor;
};

int main() {
    MemoryStream header({0, 2, 0, 0x38, 0, 0, 0, 1,
                         0, 0, 3, 0xe8, 0, 0, 0x10, 0, 2, 0, 0, 0});
    MP4::TFHD tfhd;
    tfhd.processData(&header, 20);
    if (!tfhd.has_default_sample_duration || !tfhd.has_default_sample_size ||
        !tfhd.has_default_sample_flags || tfhd.default_sample_duration != 1000 ||
        tfhd.default_sample_size != 4096 || tfhd.default_sample_flags != 0x02000000 ||
        header.pos() != 20) {
        std::fprintf(stderr, "TFHD_FLAGS_FAIL\n");
        return 1;
    }
    MemoryStream run({0, 0, 3, 0, 0, 0, 0, 2,
                      0, 0, 3, 0xe8, 0, 0, 0, 16,
                      0, 0, 3, 0xe8, 0, 0, 0, 20});
    MP4::TRUN trun;
    trun.processData(&run, 24);
    if (!trun.has_sample_duration || !trun.has_sample_size ||
        trun.has_sample_flags || trun.samples.size() != 2 ||
        trun.samples[0].duration != 1000 || trun.samples[0].size != 16 ||
        trun.samples[1].duration != 1000 || trun.samples[1].size != 20 ||
        run.pos() != 24) {
        std::fprintf(stderr, "TRUN_FLAGS_FAIL\n");
        return 1;
    }
    MemoryStream config({1, 100, 0, 10, 0xff, 0xe1, 0, 4, 0x67, 100, 0, 10,
                         1, 0, 2, 0x68, 0, 0xfd, 0xf8, 0xf8, 0});
    MP4::AVCC avcc;
    avcc.processData(&config, 21);
    if (config.pos() != 21 || avcc.spsVector.size() != 1 ||
        avcc.ppsVector.size() != 1 || avcc.nal_length != 4) {
        std::fprintf(stderr, "AVCC_HIGH_PROFILE_BOUNDARY_FAIL\n");
        return 1;
    }
    std::puts("MP4_FRAGMENT_FLAGS_PASS");
    return 0;
}
