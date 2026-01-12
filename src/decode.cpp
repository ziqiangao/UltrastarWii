// decode.cpp
#include "decode.hpp"
#include <string>

H264StreamDecoder::H264StreamDecoder() {
    decoder = h264bsdAlloc();
    if (!decoder) printf("Failed to allocate decoder\n");

    u32 status = h264bsdInit(decoder, 0);
    if (status != 0) {
        h264bsdFree(decoder);
        printf("Failed to initialize decoder\n");
    }
}

H264StreamDecoder::~H264StreamDecoder() {
    if (decoder) {
        h264bsdShutdown(decoder);
        h264bsdFree(decoder);
    }
}

void H264StreamDecoder::processChunk(const u8* data, size_t size) {
    const u8* byteStrm = data;
    u32 len = static_cast<u32>(size);
    u32 bytesRead = 0;

    while (len > 0) {
        u32 result = h264bsdDecode(decoder, const_cast<u8*>(byteStrm), len, 0, &bytesRead);

        switch (result) {
            case H264BSD_PIC_RDY: {
                u32 picId, isIdrPic, numErrMbs;
                u32* picture = h264bsdNextOutputPictureRGBA(decoder, &picId, &isIdrPic, &numErrMbs);
                if (picture) {
                    onPictureReady(picture, picId, isIdrPic, numErrMbs);
                }
                break;
            }
            case H264BSD_HDRS_RDY: {
                u32 croppingFlag, left, width, top, height;
                h264bsdCroppingParams(decoder, &croppingFlag, &left, &width, &top, &height);
                if (!croppingFlag) {
                    width = h264bsdPicWidth(decoder) * 16;
                    height = h264bsdPicHeight(decoder) * 16;
                }
                onHeadersReady(width, height);
                break;
            }
            case H264BSD_ERROR:
                onError("Decoding error");
                break;
            case H264BSD_PARAM_SET_ERROR:
                onError("Parameter set error");
                break;
            case H264BSD_MEMALLOC_ERROR:
                onError("Memory allocation error");
                break;
            case H264BSD_RDY:
                // continue processing
                break;
        }

        byteStrm += bytesRead;
        len -= bytesRead;
    }
}

void H264StreamDecoder::onPictureReady(u32* picture, u32 picId, u32 isIdrPic, u32 numErrMbs) {}
void H264StreamDecoder::onHeadersReady(u32 width, u32 height) {}
void H264StreamDecoder::onError(const std::string& error) {}
