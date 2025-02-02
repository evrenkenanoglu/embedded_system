#include "HttpUriGet.hpp"

/**
 * @brief Default HTTP GET handler for the app interface
 *
 * @param req HTTP request
 * @return error_t
 */
static error_t default_app_interface_get_handler(httpd_req_t* req);

HttpUriGet::HttpUriGet(const char* uriName, esp_err_t (*handler)(httpd_req_t*), void* user_ctx, const char* htmlContent)
    : HttpUri(uriName, HTTP_GET, handler ? handler : default_app_interface_get_handler, user_ctx)
    , _htmlContent(htmlContent)
{
}
/**
 * @brief Default HTTP GET handler for the app interface
 *
 * @param req HTTP request
 * @return error_t
 */
static error_t default_app_interface_get_handler(httpd_req_t* req)
{
    HttpUriGet* htmlPage = static_cast<HttpUriGet*>(req->user_ctx);

    if (htmlPage == nullptr)
    {
        return ESP_FAIL;
    }

    /* Send response with custom headers and body set as the *string passed in user context*/
    ESP_ERROR_CHECK(httpd_resp_send(req, htmlPage->getHtmlContent(), HTTPD_RESP_USE_STRLEN));
    return ESP_OK;
}