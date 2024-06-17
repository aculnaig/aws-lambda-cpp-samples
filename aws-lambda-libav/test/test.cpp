#include <aws/core/Aws.h>
#include <aws/core/utils/HashingUtils.h>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/error.h>
    #include <libavutil/opt.h>

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <errno.h>
    #include <execinfo.h>
}

using namespace Aws;
using namespace Aws::Utils;

static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    int nptrs;
    void *buffer[100];    
    char **strings;

    nptrs = backtrace(buffer, 100);
    printf("backtrace() returned %d address(es)\n", nptrs);

    strings = backtrace_symbols(buffer, nptrs);
    for (size_t i = 0; i < nptrs; i++)
        printf("%s\n", strings[i]);

    free(strings);

    char *blob = (char *) opaque;
    ByteBuffer blob_buffer = HashingUtils::Base64Decode(Aws::String(blob));
    uint8_t *blob_data = (uint8_t *) blob_buffer.GetUnderlyingData();

    memcpy(buf, blob_data, buf_size);

    return buf_size;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s filename\n", argv[0]);
        return 1;
    }

    SDKOptions options;
    InitAPI(options);

    const char *filename = argv[1];
    AVFormatContext *format_ctx = NULL;
    AVIOContext *io_ctx = NULL;
    AVCodecContext *codec_ctx = NULL;
    const AVCodec *codec = NULL;
    AVFrame *frame = NULL;
    AVPacket *packet = NULL;
    int ret = 0;
    char libav_error_buffer[AV_ERROR_MAX_STRING_SIZE] = { 0 };

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("fopen()");
        exit(EXIT_FAILURE);
    }

    if (fseek(file, 0, SEEK_END) == -1) {
        perror("ftell()");
        exit(EXIT_FAILURE);
    }

    long filesize = 0;
    if ((filesize = ftell(file)) == -1) {
        perror("ftell()");
        exit(EXIT_FAILURE);
    }

    rewind(file);

    uint8_t *blob = (uint8_t *) av_malloc(filesize);
    if (blob == NULL) {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }

    fread(blob, 1, filesize, file);
    if (ferror(file) != 0) {
        perror("fread()");
        exit(EXIT_FAILURE);
    }
    
    uint8_t *buffer = (uint8_t *) av_malloc(filesize);
    if (buffer == NULL) {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }

    format_ctx = avformat_alloc_context();
    if (format_ctx == NULL) {
        fprintf(stderr, "avformat_alloc_context() failed\n");
        exit(EXIT_FAILURE);
    }

    io_ctx = avio_alloc_context(buffer, filesize, 0, blob, read_packet, NULL, NULL);
    if (io_ctx == NULL) {
        fprintf(stderr, "avio_alloc_context() failed\n");
        exit(EXIT_FAILURE);
    }

    format_ctx->pb = io_ctx;

    if ((ret = avformat_open_input(&format_ctx, NULL, NULL, NULL)) != 0) {
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

    AVDictionary *dictionary = frame->metadata;
    AVDictionaryEntry *tag = NULL;
    while (tag = av_dict_get(dictionary, "", tag, AV_DICT_IGNORE_SUFFIX))
        if (tag == NULL)
            fprintf(stderr, "av_dict_get() failed\n");
        else
            printf("%s=%s\n", tag->key, tag->value);

    if (fclose(file) != 0) {
        perror("fclose()");
    }

    av_frame_free(&frame);
    av_packet_unref(packet);
    av_packet_free(&packet);
    avcodec_free_context(&codec_ctx);
    avio_context_free(&io_ctx);
    avformat_close_input(&format_ctx);
    avformat_free_context(format_ctx);

    ShutdownAPI(options);

    exit(EXIT_SUCCESS);
}
