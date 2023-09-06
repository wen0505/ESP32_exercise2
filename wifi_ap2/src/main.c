#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_server.h"
#include <string.h>

#define WIFI_SSID "esp32_ap1"
#define WIFI_PWD "password"
#define WIFI_CHANNEL 11
#define MAX_CONN_CNT 1

static const char *TAG = "ap-app";

static void handle_wifi_events(void *, esp_event_base_t, int32_t, void *);

static void init_wifi(void)
{
    if (nvs_flash_init() != ESP_OK)
    {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_event_loop_create_default();
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &handle_wifi_events, NULL);

    esp_netif_init();
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_SSID,
            .ssid_len = strlen(WIFI_SSID),
            .channel = WIFI_CHANNEL,
            .password = WIFI_PWD,
            .max_connection = MAX_CONN_CNT,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config);
    esp_wifi_start();
}

static void handle_wifi_events(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        ESP_LOGI(TAG, "a station connected");
    }
}

void app_main(void)
{
    init_wifi();
}