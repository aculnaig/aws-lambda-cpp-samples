#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/lambda-runtime/runtime.h>

#include <zimg.h>

using namespace aws::lambda_runtime;

char const TAG[] = "LAMBDA_ALLOC";

static invocation_response my_handler(invocation_request const& req)
{
    using namespace Aws::Utils::Json;
    using namespace Aws::Utils;

    AWS_LOGSTREAM_INFO(TAG, "Parsing JSON body");    

    // Decode base64 image
    JsonValue json(req.payload);
    if (!json.WasParseSuccessful()) {
        return invocation_response::failure("Failed to parse input JSON", "InvalidJSON");
    }

    auto v = json.View();

    if (!v.ValueExists("body")) {
        return invocation_response::failure("Request body is not a string", "InvalidBody"); 
    }

    AWS_LOGSTREAM_INFO(TAG, "Decode Base64 encoded body");    

    auto body = HashingUtils::Base64Decode(v.GetString("body"));
    unsigned char *body_buffer = (unsigned char *) body.GetUnderlyingData();
    size_t body_len = body.GetLength();

    unsigned src_w = 1920;
    unsigned src_h = 1080;

    unsigned dst_w = 1920 / 2;
    unsigned dst_h = 1080 / 2;
    
    unsigned body_buffer_resize_len = dst_w * dst_h * 3;
    unsigned char body_buffer_resize[body_buffer_resize_len];

    AWS_LOGSTREAM_INFO(TAG, "zimg filter graph init");    

    // Resize the image by half with zimg library
    zimg_filter_graph *graph = 0;
	zimg_image_buffer_const src_buf = { ZIMG_API_VERSION };
	zimg_image_buffer dst_buf = { ZIMG_API_VERSION };
	zimg_image_format src_format;
	zimg_image_format dst_format;
	size_t tmp_size;
	void *tmp = 0;

    src_format.width = src_w;
	src_format.height = src_h;
	src_format.pixel_type = ZIMG_PIXEL_BYTE;   

    src_format.subsample_w = 1;
	src_format.subsample_h = 1;

    src_format.color_family = ZIMG_COLOR_YUV;

    dst_format.width = dst_w;
	dst_format.height = dst_h;
	dst_format.pixel_type = ZIMG_PIXEL_BYTE;

	dst_format.subsample_w = 1;
	dst_format.subsample_h = 1;

	dst_format.color_family = ZIMG_COLOR_YUV;

    zimg_filter_graph_build(&src_format, &dst_format, 0);

    zimg_filter_graph_get_tmp_size(graph, &tmp_size);

    tmp = malloc(tmp_size);

    src_buf.plane[0].data = body_buffer;
    src_buf.plane[0].stride = src_w;
    src_buf.plane[0].mask = ZIMG_BUFFER_MAX;
    
    dst_buf.plane[0].data = body_buffer_resize;
    dst_buf.plane[0].stride = dst_w;
    dst_buf.plane[0].mask = ZIMG_BUFFER_MAX;

    AWS_LOGSTREAM_INFO(TAG, "zimg filter graph process");    

    zimg_filter_graph_process(graph, &src_buf, &dst_buf, tmp, 0, 0, 0, 0);

    zimg_filter_graph_free(graph);
    free(tmp);

    Aws::Utils::ByteBuffer body_res(body_buffer_resize, body_buffer_resize_len);
    
    AWS_LOGSTREAM_INFO(TAG, "Base64 encoding binary payload");

    auto image_encoded = HashingUtils::Base64Encode(body_res);

    JsonValue res;
    res.WithString("body", image_encoded);

    return invocation_response::success(res.View().WriteCompact(false), "application/json; charset=utf-8");
}

std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()> GetConsoleLoggerFactory()
{
    return [] {
        return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
            "console_logger", Aws::Utils::Logging::LogLevel::Trace);
    };
}

int main()
{
    using namespace Aws;

    SDKOptions options;
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Trace;
    options.loggingOptions.logger_create_fn = GetConsoleLoggerFactory();

    InitAPI(options);
    {
        run_handler(my_handler);
    }
    ShutdownAPI(options);

    return 0;
}
