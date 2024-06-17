#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/lambda-runtime/runtime.h>

#include <ipp.h>

using namespace aws::lambda_runtime;

const char *TAG = "LAMBDA_IPP_ALLOC";

// TODO: handle errors as invocation_response::failure responses
static invocation_response handle_request(invocation_request const& req)
{
    using namespace Aws::Utils;
    using namespace Aws::Utils::Json;

    JsonValue payload(req.payload);

    if (payload.View().ValueExists("isBase64Encoded"))
        if (payload.View().GetBool("isBase64Encoded") == false)
            return invocation_response::failure("The file is not a JPEG", "InvalidJPEG");

    auto jpegImage = HashingUtils::Base64Decode(payload.View().GetString("body")); 

    Ipp8u *srcIppImage = ippsMalloc_8u(jpegImage.GetLength());
    if (srcIppImage == NULL)
        return invocation_response::failure("There is not enough memory", "NotEnoughMemory");

    Ipp8u *dstIppImage = ippsMalloc_8u((1920 / 2) * (1080 / 2) * 1);
        return invocation_response::failure("There is not enough memory", "NotEnoughMemory");

    IppiResizeSpec_32f *ippSpec = 0;
    int specSize = 0, initSize = 0, bufSize = 0;
    Ipp8u *ippBuffer = 0;
    Ipp8u *initBuf = 0;
    Ipp32u numChannels = 1;
    IppiPoint dstOffset = { 0, 0 };
    IppiBorderType border = ippBorderRepl;
    int srcStep = 1920 * 1;
    int dstStep = (1920 / 2) * 1;
    IppiSize srcSize = { 1920, 1080 };
    IppiSize dstSize = { (1920 / 2), (1080 / 2) };
    IppStatus status = ippStsNoErr;

    ippsCopy_8u(srcIppImage, jpegImage.GetUnderlyingData(), jpegImage.GetLength());

    status = ippiResizeGetSize_8u(srcSize, dstSize, ippCubic, 0, &specSize, &initSize);
    if (status != ippStsNoErr)
        return invocation_response::failure("Cannot get resize buffer size", "NotResizeSize");
        
    initBuf = ippsMalloc_8u(initSize);
    if (initBuf == NULL)
        return invocation_response::failure("There is not enough memory", "NotEnoughMemory");
        
    ippSpec = (IppiResizeSpec_32f *) ippsMalloc_8u(specSize);
    if (ippSpec == NULL)
        return invocation_response::failure("There is not enough memory", "NotEnoughMemory");

    status = ippiResizeCubicInit_8u(srcSize, dstSize, 1, 1, ippSpec, initBuf);
    if (status != ippStsNoErr)
        return invocation_response::failure("Cannot init cubic resize", "NotInitCubicResize");

    ippsFree(initBuf);

    status = ippiResizeGetBufferSize_8u(ippSpec, dstSize, numChannels, &bufSize);
    if (status != ippStsNoErr)
        return invocation_response::failure("Cannot get resize buffer size", "NotResizeBufferSize");
    ippBuffer = ippsMalloc_8u(bufSize);

    status = ippiResizeCubic_8u_C1R(srcIppImage, srcStep, dstIppImage, dstStep, dstOffset, dstSize, border, 0, ippSpec, ippBuffer);
    if (status != ippStsNoErr)
        return invocation_response::failure("Cannot bicubic resize", "NotBicubicResize");

    ByteBuffer dstJpegImageBuffer(dstIppImage, (1920 / 2) * (1080 / 2) * 1);
    auto dstJpegImage = HashingUtils::Base64Encode(dstJpegImageBuffer);

    JsonValue response;
    response.WithInteger("statusCode", 200);
    response.WithBool("isBase64Encoded", true);
    response.WithString("body", payload.View().GetString("body"));

    ippsFree(ippSpec);
    ippsFree(ippBuffer);
    ippsFree(srcIppImage);
    ippsFree(dstIppImage);

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
    ippInit();
    
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
