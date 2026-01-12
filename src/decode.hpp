#if !defined(H264DECODER)
#define H264DECODER

#include <vector>
#include <memory>
#include <stdio.h>
#include <strings.h>
#include "decode.hpp"
#include "h264bsd/h264bsd_decoder.h"
#include "h264bsd/basetype.h"

class H264StreamDecoder {
private:
    storage_t* decoder;
    std::vector<u8> inputBuffer;

public:
    H264StreamDecoder();
    ~H264StreamDecoder();

    // Process a chunk of streamed data
    void processChunk(const u8* data, size_t size);

protected:
    // Callbacks to override
    virtual void onPictureReady(u32* picture, u32 picId, u32 isIdrPic, u32 numErrMbs);
    virtual void onHeadersReady(u32 width, u32 height);
    virtual void onError(const std::string& error); // <- use const char* for DevkitPPC
};

#endif // H264DECODER
