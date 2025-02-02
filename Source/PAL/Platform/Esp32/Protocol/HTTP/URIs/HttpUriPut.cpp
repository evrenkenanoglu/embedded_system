#include "HttpUriPut.hpp"

/**
 * @brief Default HTTP Put handler
 *
 * @param req HTTP request
 * @return error_t
 */
static error_t default_put_handler(httpd_req_t* req)
{
    return ESP_OK;
}

HttpUriPut::HttpUriPut(const char* uriName, esp_err_t (*handler)(httpd_req_t*), void* user_ctx)
    : HttpUri(uriName, HTTP_PUT, handler ? handler : default_put_handler, user_ctx)
{
}