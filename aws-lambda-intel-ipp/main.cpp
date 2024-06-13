#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/lambda-runtime/runtime.h>

#include <ipp/ipp.h>

using namespace aws::lambda_runtime;

static invocation_response handle_request(invocation_request const& req)
{

}

int main()
{
    run_handler(handle_request);
}
