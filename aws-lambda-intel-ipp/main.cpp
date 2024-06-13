#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/lambda-runtime/runtime.h>

#include <ipp.h>

using namespace aws::lambda_runtime;

static invocation_response handle_request(invocation_request const& req)
{
    using namespace Aws::Utils::Json;

    const IppLibraryVersion *lib = ippGetLibVersion();
    
    JsonValue response;
    response.WithString("name", lib->Name);
    response.WithString("version", lib->Version);

    return invocation_response::success(response.View().WriteReadable(false), "application/json; charset=utf-8");
}

int main()
{
    ippInit();
    
    run_handler(handle_request);
}
