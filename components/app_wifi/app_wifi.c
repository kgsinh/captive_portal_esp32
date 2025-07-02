#include <string.h>

#include "app_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs_storage.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY CONFIG_ESP_MAXIMUM_RETRY

#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

static const char *TAG = "wifi station";

static int s_retry_num = 0;

esp_event_handler_instance_t instance_any_id;
esp_event_handler_instance_t instance_got_ip;

// FUNCTION PROTOTYPES
void wifi_init_sta(void);
static void wifi_init_softap(void);
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void wifi_scan_networks(void);

bool sta_wifi_init(void)
{
    wifi_init_sta();

    return true;
}

bool softap_init(void)
{
    wifi_init_softap();

    return true;
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "WiFi station started, attempting to connect...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_event_sta_disconnected_t *disconnected_event = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGW(TAG, "WiFi disconnected, reason: %d", disconnected_event->reason);

        // Log specific disconnect reasons for debugging
        switch (disconnected_event->reason)
        {
        case WIFI_REASON_AUTH_EXPIRE:
            ESP_LOGW(TAG, "Disconnect reason: Authentication expired");
            break;
        case WIFI_REASON_AUTH_LEAVE:
            ESP_LOGW(TAG, "Disconnect reason: Authentication left");
            break;
        case WIFI_REASON_ASSOC_LEAVE:
            ESP_LOGW(TAG, "Disconnect reason: Association left");
            break;
        case WIFI_REASON_ASSOC_NOT_AUTHED:
            ESP_LOGW(TAG, "Disconnect reason: Association not authenticated");
            break;
        case WIFI_REASON_DISASSOC_PWRCAP_BAD:
            ESP_LOGW(TAG, "Disconnect reason: Disassociation due to bad power capability");
            break;
        case WIFI_REASON_DISASSOC_SUPCHAN_BAD:
            ESP_LOGW(TAG, "Disconnect reason: Disassociation due to bad supported channel");
            break;
        case WIFI_REASON_IE_INVALID:
            ESP_LOGW(TAG, "Disconnect reason: Invalid information element");
            break;
        case WIFI_REASON_MIC_FAILURE:
            ESP_LOGW(TAG, "Disconnect reason: MIC failure");
            break;
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
            ESP_LOGW(TAG, "Disconnect reason: 4-way handshake timeout");
            break;
        case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT:
            ESP_LOGW(TAG, "Disconnect reason: Group key update timeout");
            break;
        case WIFI_REASON_IE_IN_4WAY_DIFFERS:
            ESP_LOGW(TAG, "Disconnect reason: Information element in 4-way handshake differs");
            break;
        case WIFI_REASON_GROUP_CIPHER_INVALID:
            ESP_LOGW(TAG, "Disconnect reason: Invalid group cipher");
            break;
        case WIFI_REASON_PAIRWISE_CIPHER_INVALID:
            ESP_LOGW(TAG, "Disconnect reason: Invalid pairwise cipher");
            break;
        case WIFI_REASON_AKMP_INVALID:
            ESP_LOGW(TAG, "Disconnect reason: Invalid AKMP");
            break;
        case WIFI_REASON_UNSUPP_RSN_IE_VERSION:
            ESP_LOGW(TAG, "Disconnect reason: Unsupported RSN information element version");
            break;
        case WIFI_REASON_INVALID_RSN_IE_CAP:
            ESP_LOGW(TAG, "Disconnect reason: Invalid RSN information element capabilities");
            break;
        case WIFI_REASON_802_1X_AUTH_FAILED:
            ESP_LOGW(TAG, "Disconnect reason: 802.1X authentication failed");
            break;
        case WIFI_REASON_CIPHER_SUITE_REJECTED:
            ESP_LOGW(TAG, "Disconnect reason: Cipher suite rejected");
            break;
        case WIFI_REASON_BEACON_TIMEOUT:
            ESP_LOGW(TAG, "Disconnect reason: Beacon timeout");
            break;
        case WIFI_REASON_NO_AP_FOUND:
            ESP_LOGW(TAG, "Disconnect reason: No AP found");
            break;
        case WIFI_REASON_AUTH_FAIL:
            ESP_LOGW(TAG, "Disconnect reason: Authentication failed");
            break;
        case WIFI_REASON_ASSOC_FAIL:
            ESP_LOGW(TAG, "Disconnect reason: Association failed");
            break;
        case WIFI_REASON_HANDSHAKE_TIMEOUT:
            ESP_LOGW(TAG, "Disconnect reason: Handshake timeout");
            break;
        default:
            ESP_LOGW(TAG, "Disconnect reason: Unknown (%d)", disconnected_event->reason);
            break;
        }

        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP (attempt %d/%d)", s_retry_num, EXAMPLE_ESP_MAXIMUM_RETRY);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to connect to AP after %d attempts", EXAMPLE_ESP_MAXIMUM_RETRY);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
    }
}

void wifi_init_sta(void)
{
    wifi_config_t wifi_config = {};
    char ssid[64] = {0};
    char password[64] = {0};

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config.sta.threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD;
    wifi_config.sta.sae_pwe_h2e = ESP_WIFI_SAE_MODE;
    strncpy((char *)wifi_config.sta.sae_h2e_identifier, EXAMPLE_H2E_IDENTIFIER, sizeof(wifi_config.sta.sae_h2e_identifier));

    if (nvs_storage_get_wifi_credentials(ssid, password))
    {
        printf("Wi-Fi SSID: %s\n", ssid);
        printf("Wi-Fi Password: %s\n", password);
        strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    }
    else
    {
        printf("Failed to get Wi-Fi credentials from NVS.\n");
        strncpy((char *)wifi_config.sta.ssid, EXAMPLE_ESP_WIFI_SSID, sizeof(wifi_config.sta.ssid));
        strncpy((char *)wifi_config.sta.password, EXAMPLE_ESP_WIFI_PASS, sizeof(wifi_config.sta.password));
    }

    ESP_LOGI(TAG, "Configuring WiFi with SSID: %s", (char *)wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Perform a WiFi scan to help diagnose connection issues
    ESP_LOGI(TAG, "Performing network scan for diagnostics...");
    wifi_scan_networks();
}

static void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = CONFIG_ESP_WIFI_AP_SSID,
            .ssid_len = strlen(CONFIG_ESP_WIFI_AP_SSID),
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = CONFIG_ESP_MAX_AP_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
}

static void wifi_scan_networks(void)
{
    ESP_LOGI(TAG, "Starting WiFi scan...");

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true};

    esp_err_t err = esp_wifi_scan_start(&scan_config, true);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "WiFi scan failed: %s", esp_err_to_name(err));
        return;
    }

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);

    if (ap_count == 0)
    {
        ESP_LOGW(TAG, "No access points found!");
        return;
    }

    wifi_ap_record_t *ap_info = malloc(sizeof(wifi_ap_record_t) * ap_count);
    if (ap_info == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for AP list");
        return;
    }

    esp_wifi_scan_get_ap_records(&ap_count, ap_info);

    ESP_LOGI(TAG, "Found %d access points:", ap_count);

    bool target_found = false;
    for (int i = 0; i < ap_count; i++)
    {
        const char *auth_mode_str;
        switch (ap_info[i].authmode)
        {
        case WIFI_AUTH_OPEN:
            auth_mode_str = "OPEN";
            break;
        case WIFI_AUTH_WEP:
            auth_mode_str = "WEP";
            break;
        case WIFI_AUTH_WPA_PSK:
            auth_mode_str = "WPA_PSK";
            break;
        case WIFI_AUTH_WPA2_PSK:
            auth_mode_str = "WPA2_PSK";
            break;
        case WIFI_AUTH_WPA_WPA2_PSK:
            auth_mode_str = "WPA_WPA2_PSK";
            break;
        case WIFI_AUTH_WPA2_ENTERPRISE:
            auth_mode_str = "WPA2_ENTERPRISE";
            break;
        case WIFI_AUTH_WPA3_PSK:
            auth_mode_str = "WPA3_PSK";
            break;
        case WIFI_AUTH_WPA2_WPA3_PSK:
            auth_mode_str = "WPA2_WPA3_PSK";
            break;
        default:
            auth_mode_str = "UNKNOWN";
            break;
        }

        ESP_LOGI(TAG, "  %d: SSID: %-32s | RSSI: %d | Auth: %-15s | Channel: %d",
                 i + 1, ap_info[i].ssid, ap_info[i].rssi, auth_mode_str, ap_info[i].primary);

        // Check if this is our target network
        if (strcmp((char *)ap_info[i].ssid, EXAMPLE_ESP_WIFI_SSID) == 0)
        {
            target_found = true;
            ESP_LOGI(TAG, "*** TARGET NETWORK FOUND: %s ***", EXAMPLE_ESP_WIFI_SSID);
            ESP_LOGI(TAG, "    Signal strength: %d dBm", ap_info[i].rssi);
            ESP_LOGI(TAG, "    Security: %s", auth_mode_str);
            ESP_LOGI(TAG, "    Channel: %d", ap_info[i].primary);

            // Check if our auth mode is compatible
            if (ap_info[i].authmode != ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD)
            {
                ESP_LOGW(TAG, "    WARNING: Network security (%s) doesn't match configured threshold (%d)",
                         auth_mode_str, ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD);
            }
        }
    }

    if (!target_found)
    {
        ESP_LOGE(TAG, "*** TARGET NETWORK '%s' NOT FOUND! ***", EXAMPLE_ESP_WIFI_SSID);
        ESP_LOGE(TAG, "Please check:");
        ESP_LOGE(TAG, "  1. Network name is correct");
        ESP_LOGE(TAG, "  2. Network is broadcasting (not hidden)");
        ESP_LOGE(TAG, "  3. ESP32 is within range");
        ESP_LOGE(TAG, "  4. Router is powered on");
    }

    free(ap_info);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}
