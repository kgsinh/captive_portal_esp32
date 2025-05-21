
// Components
#include <sys/param.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/inet.h"
#include "esp_ota_ops.h"
#include "nvs_storage.h"
#include "custom_partition.h"
#include "app_local_server.h"
#include "app_time_sync.h"
#include "app_wifi.h"
static const char *TAG = "example";

#define EXAMPLE_ESP_WIFI_AP_SSID CONFIG_ESP_WIFI_AP_SSID
#define EXAMPLE_MAX_STA_CONN CONFIG_ESP_MAX_AP_STA_CONN

// FUNCTION PROTOTYPES

// static void handler_initialize(void);

void app_main(void)
{
    // Initialize NVS
    nvs_storage_init();

    // Initialize custom partition
    nvs_custom_partition_init();

    // Initialize networking stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop needed by the  main app
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    app_local_server_init();

    // Initialize Wi-Fi including netif with default config
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)); // move it to app_wifi.c

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    // Initialise ESP32 in SoftAP mode
    sta_wifi_init();
    softap_init();
    app_local_server_start();

    ESP_ERROR_CHECK(esp_wifi_start());

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    ESP_LOGI(TAG, "Set up softAP with IP: %s", ip_addr);

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:'%s' password:'%s'",
             EXAMPLE_ESP_WIFI_AP_SSID, CONFIG_ESP_WIFI_PASSWORD);

    time_sync_init();

    while (1)
    {
        app_local_server_process();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}