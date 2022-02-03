#ifndef FFH264DECODERINSTANCE_H
#define FFH264DECODERINSTANCE_H

#include "FFDecoderInstance.h"
extern "C"
{
#include <jpeglib.h>
}

#define USE_NVMPI
#if defined (USE_NVMPI)
#include <nvmpi.h>
#endif

class FFH264DecoderInstance : public FFDecoderInstance
{
public:
    FFH264DecoderInstance(const std::string& address, bool sync, int w, int h);
    FFH264DecoderInstance(const FFH264DecoderInstance&c) = delete;
    virtual ~FFH264DecoderInstance();

private:
    void run();

private:
    std::atomic_bool m_stop;
    std::thread *m_mainThread;

    FFPlayerInstance *m_player = nullptr;

    // decoder
    AVStream *m_videoStream = nullptr;
    std::mutex m_codecContextLocker;
    AVCodecContext *m_videoCodecContext = nullptr;

    AVCodecContext *m_jpegContext = nullptr;

#if defined (USE_NVMPI)

    typedef struct {
        char eos_reached;
        nvmpictx* ctx;
        AVClass *av_class;
    } nvmpiDecodeContext;

    static nvCodingType nvmpi_get_codingtype(AVCodecContext *avctx)
    {
        switch (avctx->codec_id) {
            case AV_CODEC_ID_H264:          return NV_VIDEO_CodingH264;
            case AV_CODEC_ID_HEVC:          return NV_VIDEO_CodingHEVC;
            case AV_CODEC_ID_VP8:           return NV_VIDEO_CodingVP8;
            case AV_CODEC_ID_VP9:           return NV_VIDEO_CodingVP9;
            case AV_CODEC_ID_MPEG4:     return NV_VIDEO_CodingMPEG4;
            case AV_CODEC_ID_MPEG2VIDEO:    return NV_VIDEO_CodingMPEG2;
            default:                        return NV_VIDEO_CodingUnused;
        }
    }
    int nvmpiInitDecoder(AVCodecContext *avctx) {
        int ret=0;
        nvmpiDecodeContext *nvmpi_context = (nvmpiDecodeContext *)avctx->priv_data;
        nvCodingType codectype=NV_VIDEO_CodingUnused;

        codectype = nvmpi_get_codingtype(avctx); // NV_VIDEO_CodingH264

        if (codectype == NV_VIDEO_CodingUnused) {
            av_log(avctx, AV_LOG_ERROR, "Unknown codec type (%d).\n", avctx->codec_id);
            ret = AVERROR_UNKNOWN;
            return ret;
        }

        nvmpi_context->ctx = nvmpi_create_decoder(codectype,NV_PIX_YUV420);

        if(!nvmpi_context->ctx){
            av_log(avctx, AV_LOG_ERROR, "Failed to nvmpi_create_decoder (code = %d).\n", ret);
            ret = AVERROR_UNKNOWN;
            return ret;
        }
        return ret;
    }
#endif
};

#endif // FFH264DECODERINSTANCE_H
