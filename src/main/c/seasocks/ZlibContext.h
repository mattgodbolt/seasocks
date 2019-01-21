#pragma once

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <vector>

namespace seasocks {

class ZlibContext {
public:
    ZlibContext(const ZlibContext&) = delete;
    ZlibContext& operator=(const ZlibContext&) = delete;

    ZlibContext();
    ~ZlibContext();
    void initialise(int deflateBits = 15, int inflateBits = 15, int memLevel = 6);

    void deflate(const uint8_t* input, size_t inputLen, std::vector<uint8_t>& output);

    // WARNING: inflate() alters input
    bool inflate(std::vector<uint8_t>& input, std::vector<uint8_t>& output, int& zlibError);

private:
    struct Impl;
    std::unique_ptr<Impl> _impl;
};

} // namespace seasocks
