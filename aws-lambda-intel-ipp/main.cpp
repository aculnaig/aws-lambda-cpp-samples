#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/lambda-runtime/runtime.h>

#include <ipp.h>

using namespace aws::lambda_runtime;

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
    ippsCopy_8u(srcIppImage, jpegImage.GetUnderlyingData(), jpegImage.GetLength());

    Ipp8u *dstIppImage = ippsMalloc_8u((1920 / 2) * (1080 / 2) * 3);
    int srcStep = 1920 * 3;
    int dstStep = (1920 / 2) * 3;

    IppiSize srcSize = { 1920, 1080 };
    IppiRect srcRect = { 0, 0, 1920, 1080 };
    IppiSize dstSize = { (1920 / 2), (1080 / 2) };
    IppiPoint dstOffset = { 0, 0 };

    ippiResizeCubic_8u_C3R(srcIppImage, srcStep, dstIppImage, dstStep, dstOffset, dstSize, ippBorderRepl, 0, NULL, NULL);

    ByteBuffer dstJpegImageBuffer(dstIppImage, (1920 / 2) * (1080 / 2) * 3);
    auto dstJpegImage = HashingUtils::Base64Encode(dstJpegImageBuffer);

    JsonValue response;
    response.WithInteger("statusCode", 200);
    response.WithBool("isBase64Encoded", true);
    response.WithString("body", payload.View().GetString("body"));

    ippsFree(srcIppImage);
    ippsFree(dstIppImage);

    return invocation_response::success(response.View().WriteCompact(false), "image/jpg");
}

int main()
{
    ippInit();
    
    run_handler(handle_request);
}
