#include "HttpUriGet.hpp"
#include "HAL/Platform/ESP32/Library/logImpl.h"

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

    /* Send response with custom headers and body set as the string passed in user context */
    esp_err_t err = httpd_resp_send(req, htmlPage->getHtmlContent(), HTTPD_RESP_USE_STRLEN);
    if (err != ESP_OK) {
        logger().log(ILog::LogLevel::ERROR, "Failed to send response: " + std::to_string(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send response");
        return err;
    }
    
    return ESP_OK;
}