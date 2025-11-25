#include "HttpUriPost.hpp"
#include <cstring>

#define ENABLE_SYS_LOG_D
#include "System/LogHandler.h"

HttpUriPost::HttpUriPost(const char* uriName, Handler handler, void* user_ctx)
    : HttpUri(
          uriName,          // URI Name
          HttpMethod::POST, // Method
          handler,          // Handler
          user_ctx          // User Context
      )
{

    if (!user_ctx)
    {
        SYS_LOG_I("No user_ctx provided, using default HTML handler");
        // If No user_ctx provided, set user_ctx to this for default handler
        setUserContext(this);

    }
}