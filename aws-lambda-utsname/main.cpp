#include <aws/core/Aws.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/lambda-runtime/runtime.h>

#include <string.h>
#include <errno.h>
#include <sys/utsname.h>

using namespace aws::lambda_runtime;

extern int errno;

static invocation_response handle_request(invocation_request const& req)
{
    using namespace Aws;
    using namespace Aws::Utils;
    using namespace Aws::Utils::Json;

    int ret = 0;
    struct utsname uts_name;

    if ((ret = uname(&uts_name)) == -1)
        return invocation_response::failure(strerror(ret), StringUtils::to_string(errno));

    JsonValue res;
    res.WithString("sysname", uts_name.sysname);
    res.WithString("nodename", uts_name.nodename);
    res.WithString("release", uts_name.release);
    res.WithString("version", uts_name.version);
    res.WithString("machine", uts_name.machine);
    #ifdef _GNU_SOURCE
        res.WithString("domainname", uts_name.domainname);
    #endif

    return invocation_response::success(res.View().WriteCompact(false), "application/json");
}

int main()
{
    using namespace Aws;

    SDKOptions options;
    InitAPI(options);
    {
        run_handler(handle_request);
    }
    ShutdownAPI(options);

    return 0;
}
