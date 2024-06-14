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

    JsonValue response;
    response.WithString("body", body.View().GetString("body"));

    return invocation_response::success(response.View().WriteReadable(false), "application/json; charset=utf-8");
}

int main()
{
    ippInit();
    
    run_handler(handle_request);
}
