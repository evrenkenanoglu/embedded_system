#include "HttpUriGet.hpp"
#include <cstring>

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

HttpUriGet::HttpUriGet(const char* uriName, Handler handler, void* user_ctx, const char* static_content)
    : HttpUri(
          uriName,         // URI Name
          HttpMethod::GET, // Method
          handler,         // Handler
          user_ctx         // User Context
      )
{
    if (!user_ctx)
    {
        SYS_LOG_D("No user_ctx provided, using default HTML handler");
        // If No user_ctx provided, set user_ctx to this for default handler
        setUserContext(this);
    }

    setStaticContent(static_content, static_content ? std::strlen(static_content) : 0);
}