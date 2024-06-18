#include <aws/core/Aws.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/memory/AWSMemory.h>
#include <aws/lambda-runtime/runtime.h>
#include <aws/logging/logging.h>

extern "C" {
    #include <ImageMagick-7/MagickWand/MagickWand.h>
}

const char TAG[] = "AWS_LAMBDA_JPEG_ALLOC";

using namespace aws::lambda_runtime;

static invocation_response handle_request(invocation_request const& req)
{
    using namespace Aws;
    using namespace Aws::Utils;
    using namespace Aws::Utils::Json;

    JsonValue payload(req.payload);

    if (payload.View().ValueExists("isBase64Encoded"))
        if (payload.View().GetBool("isBase64Encoded") == false)
            return invocation_response::failure("Not a binary file", "NonBinaryFile");

    auto body = HashingUtils::Base64Decode(payload.View().GetString("body"));
    uint8_t *body_data = (uint8_t *) body.GetUnderlyingData();
    size_t body_len = (size_t) body.GetLength();

    MagickBooleanType Status;
    ExceptionType Exception;
    char *WandError;
    MagickWand *Wand = NewMagickWand();

    MagickReadImageBlob(Wand, body_data, body_len);
    Exception = MagickGetExceptionType(Wand);
    if (Exception != UndefinedException) {
        WandError = MagickGetException(Wand, &Exception);
        return invocation_response::failure(WandError, "MagickReadImageBlob"); 
    }

    AWS_LOGSTREAM_INFO(TAG, "Read JPEG file properties...");

    size_t NProperties;
    char **Exif = MagickGetImageProperties(Wand, "*:*", &NProperties);
    Exception = MagickGetExceptionType(Wand);
    if (Exception != UndefinedException) {
        WandError = MagickGetException(Wand, &Exception);
        return invocation_response::failure(WandError, "MagickGetImageProperties"); 
    }

    JsonValue response;
    for (int i = 0; i < NProperties; i++) {
        char *Property = MagickGetImageProperty(Wand, Exif[i]);
        response.WithString(Exif[i], Property);    
        MagickRelinquishMemory(Property);
    }

    MagickRelinquishMemory(Exif);
    DestroyMagickWand(Wand);

    return invocation_response::success(response.View().WriteCompact(false), "application/json");
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
        MagickWandGenesis();

        run_handler(handle_request);

        MagickWandTerminus();
    }
    ShutdownAPI(options);

    return 0;
}
