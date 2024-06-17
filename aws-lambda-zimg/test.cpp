#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <aws/core/Aws.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/utils/memory/AWSMemory.h>

extern "C" {
    #include <zimg.h>
}

const char TAG[] = "AWS_ALLOC";

int main(int argc, char **argv)
{
    using namespace Aws;
    using namespace Aws::Utils;
    using namespace Aws::Utils::Memory;

    if (argc != 2) {
        fprintf(stderr, "usage: %s filename", argv[0]);
        exit(EXIT_FAILURE);
    }

    SDKOptions options;
    InitAPI(options);

    const char *filename = argv[1];

    FILE *file = fopen(filename, "rb");
    
    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    rewind(file);

    char *filebuffer = (char *) Aws::Malloc(TAG, filesize);
    fread(filebuffer, filesize, 1, file);

    ByteBuffer filecontent = HashingUtils::Base64Decode(Aws::String(filebuffer));
    char *filebody = (char *) filecontent.GetUnderlyingData();
    size_t filelen = (size_t) filecontent.GetLength();

    zimg_filter_graph *graph = 0;
    zimg_image_buffer_const src_buf = { ZIMG_API_VERSION }; 
    zimg_image_buffer dst_buf = { ZIMG_API_VERSION }; 
    zimg_image_format src_format;
    zimg_image_format dst_format;
    size_t tmp_size;
    void *tmp = 0;
    zimg_error_code_e zimg_last_error = ZIMG_ERROR_SUCCESS;
    char zimg_last_error_str[1024] = { 0 };

    zimg_image_format_default(&src_format, ZIMG_API_VERSION); 
    zimg_image_format_default(&dst_format, ZIMG_API_VERSION);

    src_format.width = 4896;
    src_format.height = 3264;
    src_format.pixel_type = ZIMG_PIXEL_BYTE;

    src_format.color_family = ZIMG_COLOR_RGB;

    dst_format.width = 4896 / 2;
    dst_format.height = 3264 / 2;
    dst_format.pixel_type = ZIMG_PIXEL_BYTE;

    dst_format.color_family = ZIMG_COLOR_RGB;

    graph = zimg_filter_graph_build(&src_format, &dst_format, 0);
    if (graph == NULL) {
        zimg_get_last_error(zimg_last_error_str, 1024);
        fprintf(stderr, "zimg_filter_graph_build(): %s\n", zimg_last_error_str);
        zimg_clear_last_error();
        exit(EXIT_FAILURE);
    }

    zimg_last_error = zimg_filter_graph_get_tmp_size(graph, &tmp_size);
    if (zimg_last_error != ZIMG_ERROR_SUCCESS) {
        zimg_get_last_error(zimg_last_error_str, 1024);
        fprintf(stderr, "zimg_filter_graph_get_tmp_size(): %s\n", zimg_last_error_str);
        zimg_clear_last_error();
        exit(EXIT_FAILURE);
    }

    tmp = Aws::Malloc(TAG, tmp_size);
    void *dst_file = Aws::Malloc(TAG, (4896 / 2) * (3264 / 2) * 3);

    src_buf.plane[0].data = (void *) filebody;
    src_buf.plane[0].stride = 3264;
    src_buf.plane[0].mask = ZIMG_BUFFER_MAX;

    src_buf.plane[0].data = dst_file;
    src_buf.plane[0].stride = (3264 / 2);
    src_buf.plane[0].mask = ZIMG_BUFFER_MAX;

    zimg_last_error = zimg_filter_graph_process(graph, &src_buf, &dst_buf, tmp, 0, 0, 0, 0);
    if (zimg_last_error != ZIMG_ERROR_SUCCESS) {
        zimg_get_last_error(zimg_last_error_str, 1024);
        fprintf(stderr, "zimg_filter_graph_process(): %s\n", zimg_last_error_str);
        zimg_clear_last_error();
        exit(EXIT_FAILURE);
    }

    zimg_filter_graph_free(graph);

    Aws::Free(tmp);
    Aws::Free(dst_file);
    Aws::Free(filebuffer);

    ShutdownAPI(options);

    return 0;
}
