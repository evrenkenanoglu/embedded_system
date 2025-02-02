#include "HttpUriPost.hpp"

/**
 * @brief Default HTTP POST handler
 *
 * @param req HTTP request
 * @return error_t
 */
static error_t default_post_handler(httpd_req_t* req)
{
    return ESP_OK;
}

HttpUriPost::HttpUriPost(const char* uriName, esp_err_t (*handler)(httpd_req_t*), void* user_ctx)
    : HttpUri(uriName, HTTP_POST, handler ? handler : default_post_handler, user_ctx)
{
}