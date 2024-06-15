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

    JsonValue response;
    response.WithInteger("statusCode", 200);
    response.WithBool("isBase64Encoded", true);
    response.WithString("body", payload.View().GetString("body"));

    return invocation_response::success(response.View().WriteReadable(false), "image/jpg");
}

int main()
{
    ippInit();
    
    run_handler(handle_request);
}
