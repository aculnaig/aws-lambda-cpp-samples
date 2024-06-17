#include <aws/core/Aws.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/memory/AWSMemory.h>
#include <aws/lambda-runtime/runtime.h>
#include <aws/logging/logging.h>

#include <zimg.h>

using namespace aws::lambda_runtime;
using namespace aws::logging;

const char TAG[] = "AWS_LAMBDA_ZIMG_ALLOC";

// TODO: handle libraries functions errors and return invocation_response::failure
static invocation_response handle_request(invocation_request const& req)
{
    using namespace Aws::Utils;
    using namespace Aws::Utils::Json;
    using namespace Aws::Utils::Memory;

    // AWS API Gateway encodes the payload in base64 if it is binary data
    // since a Lambda can only receive JSON payloads

    log_info(TAG, "Decoding Base64 encoded body...");
    
    // Parse the JSON payload and get the body of the HTTP request
    JsonValue payload(req.payload);
    auto body = payload.View().GetString("body"); 
    auto image = HashingUtils::Base64Decode(body);

    log_info(TAG, "Initializing zimg structures to defaults...");
    // Initialize zimg structures to defaults
    zimg_filter_graph *graph = 0;
    zimg_image_buffer_const src_buf = { ZIMG_API_VERSION }; 
    zimg_image_buffer dst_buf = { ZIMG_API_VERSION }; 
    zimg_image_format src_format;
    zimg_image_format dst_format;
    size_t tmp_size;
    void *tmp = 0;
    
    zimg_image_format_default(&src_format, ZIMG_API_VERSION); 
    zimg_image_format_default(&dst_format, ZIMG_API_VERSION); 

    src_format.width = 1920;
	src_format.height = 1080;
	src_format.pixel_type = ZIMG_PIXEL_BYTE;

	src_format.subsample_w = 1;
	src_format.subsample_h = 1;

	src_format.color_family = ZIMG_COLOR_GREY;

	dst_format.width = 1920 / 2;
	dst_format.height = 1080 / 2;
	dst_format.pixel_type = ZIMG_PIXEL_BYTE;

	dst_format.subsample_w = 1;
	dst_format.subsample_h = 1;

	dst_format.color_family = ZIMG_COLOR_GREY;

    zimg_filter_graph_build(&src_format, &dst_format, 0);

    zimg_filter_graph_get_tmp_size(graph, &tmp_size);

    tmp = Aws::Malloc(TAG, tmp_size);
    size_t image_resized_buffer_len = (1920 / 2) * (1080 / 2) * 1;
    ByteBuffer image_resized_buffer(0, image_resized_buffer_len);

    src_buf.plane[0].data = static_cast<const void *>(image.GetUnderlyingData());
    src_buf.plane[0].stride = 1080;
    src_buf.plane[0].mask = ZIMG_BUFFER_MAX;

    src_buf.plane[0].data = static_cast<const void *>(image_resized_buffer.GetUnderlyingData());
    src_buf.plane[0].stride = (1080 / 2);
    src_buf.plane[0].mask = ZIMG_BUFFER_MAX;

    log_info(TAG, "Processing the image...");
    zimg_filter_graph_process(graph, &src_buf, &dst_buf, tmp, 0, 0, 0, 0);

    zimg_filter_graph_free(graph);
    Aws::Free(tmp);

    log_info(TAG, "Base64 encoding the body...");
    
    auto body_res = HashingUtils::Base64Encode(image_resized_buffer);

    JsonValue response;
    response.WithInteger("status", 200);
    response.WithBool("isBase64Encoded", true);
    response.WithString("body", body_res);
    return invocation_response::success(response.View().WriteCompact(false), "image/jpg");
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
        run_handler(handle_request);
    }
    ShutdownAPI(options);

    return 0;
}
