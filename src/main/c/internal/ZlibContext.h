#pragma once

#include <zlib.h>

#include <vector>
#include <stdexcept>


namespace seasocks {

class ZlibContext {
public:
    ZlibContext(const ZlibContext&) = delete;
    ZlibContext& operator=(const ZlibContext&) = delete;

    ZlibContext(int deflateBits=15, int inflateBits=15, int memLevel=6) {
        int ret;

        deflateStream.zalloc = Z_NULL;
        deflateStream.zfree = Z_NULL;
        deflateStream.opaque = Z_NULL;

        ret = ::deflateInit2(
            &deflateStream,
            Z_DEFAULT_COMPRESSION,
            Z_DEFLATED,
            deflateBits * -1,
            memLevel,
            Z_DEFAULT_STRATEGY
        );

        if (ret != Z_OK) {
            throw std::runtime_error("error initialising zlib deflater");
        }

        inflateStream.zalloc = Z_NULL;
        inflateStream.zfree = Z_NULL;
        inflateStream.opaque = Z_NULL;
        inflateStream.avail_in = 0;
        inflateStream.next_in = Z_NULL;

        ret = ::inflateInit2(
            &inflateStream,
            inflateBits * -1
        );

        if (ret != Z_OK) {
            ::deflateEnd(&deflateStream);
            throw std::runtime_error("error initialising zlib inflater");
        }

        streamsInitialised = true;
    }

    ~ZlibContext() {
        if (!streamsInitialised) return;
        ::deflateEnd(&deflateStream);
        ::inflateEnd(&inflateStream);
    }

    void deflate(const uint8_t* input, size_t inputLen, std::vector<uint8_t>& output) {
        deflateStream.next_in = (unsigned char *)input;
        deflateStream.avail_in = inputLen;

        do {
            deflateStream.next_out = buffer;
            deflateStream.avail_out = sizeof(buffer);

            (void) ::deflate(&deflateStream, Z_SYNC_FLUSH);

            output.insert(output.end(), buffer, buffer + sizeof(buffer) - deflateStream.avail_out);
        } while (deflateStream.avail_out == 0);

        // Remove 4-byte tail end prior to transmission (see RFC 7692, section 7.2.1)
        output.resize(output.size() - 4);
    }

    // WARNING: inflate() alters input
    bool inflate(std::vector<uint8_t>& input, std::vector<uint8_t>& output, int& zlibError) {
        // Append 4 octets prior to decompression (see RFC 7692, section 7.2.2)
        uint8_t tail_end[4] = {0x00, 0x00, 0xff, 0xff};
        input.insert(input.end(), tail_end, tail_end + 4);

        inflateStream.next_in = input.data();
        inflateStream.avail_in = input.size();

        do {
            inflateStream.next_out = buffer;
            inflateStream.avail_out = sizeof(buffer);

            int ret = ::inflate(&inflateStream, Z_SYNC_FLUSH);

            if (ret != Z_OK && ret != Z_STREAM_END) {
                zlibError = ret;
                return false;
            }

            output.insert(output.end(), buffer, buffer + sizeof(buffer) - inflateStream.avail_out);
        } while (inflateStream.avail_out == 0);

        return true;
    }

private:
    z_stream deflateStream;
    z_stream inflateStream;
    bool streamsInitialised = false;
    uint8_t buffer[16384];
};

}  // namespace seasocks
