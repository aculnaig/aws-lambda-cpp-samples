#include <aws/core/Aws.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/utils/memory/AWSMemory.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/lambda-runtime/runtime.h>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/opt.h>

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <errno.h>
}

using namespace aws::lambda_runtime;

const char *TAG = "AWS_LAMBDA_LIBAV_ALLOC";

static int read_packet(void *opaque, uint8_t *buf, int buf_size) {
    uint8_t *blob = (uint8_t *) opaque;
    
    memcpy(buf, blob, buf_size);
          
    return buf_size;
}

static invocation_response handle_request(invocation_request const& req)
{
    using namespace Aws::Utils;
    using namespace Aws::Utils::Json;
    using namespace Aws::Utils::Memory;

    JsonValue payload(req.payload);

    if (payload.View().ValueExists("isBase64Encoded"))
        if (payload.View().GetBool("isBase64Encoded") == false)
            return invocation_response::failure("The file is not a JPEG", "InvalidJPEG");

    auto body = HashingUtils::Base64Decode(payload.View().GetString("body"));
    auto body_buf = body.GetUnderlyingData();
    auto body_len = body.GetLength(); 

    auto buffer = (uint8_t *) Aws::Malloc(TAG, body_len);

    AVFormatContext *format_ctx = NULL;
    AVIOContext *io_ctx = NULL;
    AVCodecContext *codec_ctx = NULL;
    const AVCodec *codec = NULL;
    AVFrame *frame = NULL;
    AVPacket *packet = NULL;
    char libav_error_buffer[AV_ERROR_MAX_STRING_SIZE] = { 0 };
    int ret = 0;

    format_ctx = avformat_alloc_context();
    if (format_ctx == NULL) {
        fprintf(stderr, "avformat_alloc_context() failed\n");
        exit(EXIT_FAILURE);
    }

    io_ctx = avio_alloc_context(buffer, body_len, 0, (void *) body_buf, read_packet, NULL, NULL);
    if (io_ctx == NULL) {
        fprintf(stderr, "avio_alloc_context( failed\n)");
        exit(EXIT_FAILURE);
    }

    format_ctx->pb = io_ctx;

    if ((ret = avformat_open_input(&format_ctx, NULL, NULL, NULL)) != 0) {
        AVERROR(ret);
        av_strerror(ret, libav_error_buffer, sizeof(libav_error_buffer));
        fprintf(stderr, "avformat_open_input(): %s\n", libav_error_buffer);
        exit(EXIT_FAILURE);
    }

    if (avformat_find_stream_info(format_ctx, NULL) != 0) {
        fprintf(stderr, "avformat_find_stream_info() failed\n");
        exit(EXIT_FAILURE);
    }

    if (av_find_best_stream(format_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0) != 0) {
        fprintf(stderr, "av_find_best_stream() failed\n");
        exit(EXIT_FAILURE);
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (codec_ctx == NULL) {
        fprintf(stderr, "avcodec_alloc_context() failed\n");
        exit(EXIT_FAILURE);
    }

    if (avcodec_open2(codec_ctx, codec, NULL) != 0) {
        fprintf(stderr, "avcodec_open2() failed\n");
        exit(EXIT_FAILURE);
    }

    frame = av_frame_alloc();
    if (frame == NULL) {
        fprintf(stderr, "av_frame_alloc() failed\n");
        exit(EXIT_FAILURE);
    }

    packet = av_packet_alloc();
    if (packet == NULL) {
        fprintf(stderr, "av_packet_alloc() failed\n");
        exit(EXIT_FAILURE);
    }

    while (av_read_frame(format_ctx, packet) >= 0) {

        avcodec_send_packet(codec_ctx, packet);
        avcodec_receive_frame(codec_ctx, frame);

        break;
    }

    if (frame->metadata == NULL) {
        fprintf(stderr, "Input frame metadata are NULL\n");
        exit(EXIT_FAILURE);
    }

    JsonValue response;
    JsonValue metadata;

    AVDictionary *dictionary = frame->metadata;
    AVDictionaryEntry *tag = NULL;
    while ((tag = av_dict_get(dictionary, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        if (tag == NULL)
            continue;
        else
            metadata.WithString(tag->key, tag->value); 
    }

    response.WithObject("metadata", metadata);
    
    Aws::Free(buffer);
    av_frame_free(&frame);
    av_packet_unref(packet);
    av_packet_free(&packet);
    avcodec_free_context(&codec_ctx);
    avio_context_free(&io_ctx);
    avformat_close_input(&format_ctx);
    avformat_free_context(format_ctx);

    return invocation_response::success(response.View().WriteCompact(false), "application/json; charset=utf-8");
}

int main()
{
    using namespace Aws;
    
    SDKOptions options;
    InitAPI(options);
    {
        run_handler(handle_request);
    }
    ShutdownAPI(options);

    return 0;
}
