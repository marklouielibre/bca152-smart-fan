#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_netif.h"

#include "app_wifi.h"
#include "app_hw.h"
#include "app_http.h"

#define TAG "app"

static void handle_wifi_connect(void)
{
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    if (netif != NULL && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK)
    {
        ESP_LOGI(TAG, "Wi-Fi connected; HTTP control at http://" IPSTR "/", IP2STR(&ip_info.ip));
    }

    if (apphttp_start() != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to start HTTP server");
    }
}

static void handle_wifi_failed(void)
{
    ESP_LOGE(TAG, "wifi failed");
}

void app_main()
{
    apphw_init(NULL);

    connect_wifi_params_t cbs = {
        .on_connected = handle_wifi_connect,
        .on_failed = handle_wifi_failed};
    appwifi_connect(cbs);
}
