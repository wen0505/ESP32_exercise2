#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_client.h"

#define WIFI_SSID "esp32_ap1"
#define WIFI_PASS "password"

static EventGroupHandle_t wifi_events;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define MAX_RETRY 10
static int retry_cnt = 0;

static const char *TAG = "wifi_app";

static void handle_wifi_connection(void *, esp_event_base_t, int32_t, void *);

static void init_wifi(void)
{
    if (nvs_flash_init() != ESP_OK)
    {
        nvs_flash_erase();
        nvs_flash_init();
    }

    wifi_events = xEventGroupCreate();
    esp_event_loop_create_default();
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &handle_wifi_connection, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &handle_wifi_connection, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
    };

    esp_netif_init();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();

    EventBits_t bits = xEventGroupWaitBits(wifi_events,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGE(TAG, "connected to ap SSID:%s password:%s", WIFI_SSID, WIFI_PASS);
    }
    else
    {
        ESP_LOGE(TAG, "failed");
    }
}

static void handle_wifi_connection(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (retry_cnt++ < MAX_RETRY)
        {
            esp_wifi_connect();
            ESP_LOGI(TAG, "wifi connect retry: %d", retry_cnt);
        }
        else
        {
            xEventGroupSetBits(wifi_events, WIFI_FAIL_BIT);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "ip: %d.%d.%d.%d", IP2STR(&event->ip_info.ip));
        retry_cnt = 0;
        xEventGroupSetBits(wifi_events, WIFI_CONNECTED_BIT);
    }
}

void app_main(void)
{
    init_wifi();
}