#include "app_http.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_hw.h"
#include "esp_http_server.h"
#include "esp_log.h"

#define TAG "app.http"

static const char *const speed_labels[] = {"OFF", "LOW", "MED", "HIGH"};
static httpd_handle_t server = NULL;

static esp_err_t send_state_response(httpd_req_t *req, const char *status, uint8_t speed)
{
    char response[96];
    int response_length = snprintf(response, sizeof(response),
                                   "{\"status\":\"%s\",\"speed\":%u,\"label\":\"%s\"}",
                                   status, (unsigned int)speed, speed_labels[speed]);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    if (strcmp(status, "ok") == 0)
    {
        httpd_resp_set_status(req, "200 OK");
    }
    return httpd_resp_send(req, response, response_length);
}

static esp_err_t send_bad_request(httpd_req_t *req)
{
    httpd_resp_set_status(req, "400 Bad Request");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"level must be an integer from 0 to 3\"}");
}

static esp_err_t apply_speed(httpd_req_t *req, uint8_t speed)
{
    if (!apphw_set_state(speed))
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Unable to update fan state");
        return ESP_FAIL;
    }
    return send_state_response(req, "ok", apphw_get_state());
}

static esp_err_t speed_query_handler(httpd_req_t *req)
{
    char query[64];
    char level_value[8];
    size_t query_length = httpd_req_get_url_query_len(req);
    if (query_length == 0 || query_length >= sizeof(query) ||
        httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
        httpd_query_key_value(query, "level", level_value, sizeof(level_value)) != ESP_OK)
    {
        return send_bad_request(req);
    }

    errno = 0;
    char *end = NULL;
    long speed = strtol(level_value, &end, 10);
    if (errno != 0 || end == level_value || *end != '\0' || speed < 0 || speed > 3)
    {
        return send_bad_request(req);
    }
    return apply_speed(req, (uint8_t)speed);
}

static esp_err_t fixed_speed_handler(httpd_req_t *req)
{
    uint8_t speed = (uint8_t)(uintptr_t)req->user_ctx;
    return apply_speed(req, speed);
}

static esp_err_t turn_on_handler(httpd_req_t *req)
{
    if (!apphw_turn_on_if_off())
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Unable to update fan state");
        return ESP_FAIL;
    }
    return send_state_response(req, "ok", apphw_get_state());
}

static esp_err_t status_handler(httpd_req_t *req)
{
    return send_state_response(req, "ok", apphw_get_state());
}

static esp_err_t register_uri(const char *uri, httpd_method_t method,
                              esp_err_t (*handler)(httpd_req_t *), void *user_ctx)
{
    httpd_uri_t route = {
        .uri = uri,
        .method = method,
        .handler = handler,
        .user_ctx = user_ctx};
    return httpd_register_uri_handler(server, &route);
}

esp_err_t apphttp_start(void)
{
    if (server != NULL)
    {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_uri_handlers = 8;
    esp_err_t err = httpd_start(&server, &config);
    if (err != ESP_OK)
    {
        server = NULL;
        return err;
    }

    err = register_uri("/speed", HTTP_GET, speed_query_handler, NULL);
    if (err == ESP_OK) err = register_uri("/off", HTTP_GET, fixed_speed_handler, (void *)(uintptr_t)0);
    if (err == ESP_OK) err = register_uri("/low", HTTP_GET, fixed_speed_handler, (void *)(uintptr_t)1);
    if (err == ESP_OK) err = register_uri("/med", HTTP_GET, fixed_speed_handler, (void *)(uintptr_t)2);
    if (err == ESP_OK) err = register_uri("/medium", HTTP_GET, fixed_speed_handler, (void *)(uintptr_t)2);
    if (err == ESP_OK) err = register_uri("/high", HTTP_GET, fixed_speed_handler, (void *)(uintptr_t)3);
    if (err == ESP_OK) err = register_uri("/on", HTTP_GET, turn_on_handler, NULL);
    if (err == ESP_OK) err = register_uri("/status", HTTP_GET, status_handler, NULL);

    if (err != ESP_OK)
    {
        httpd_stop(server);
        server = NULL;
        ESP_LOGE(TAG, "failed to register HTTP routes: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "HTTP server listening on port 80");
    return ESP_OK;
}
