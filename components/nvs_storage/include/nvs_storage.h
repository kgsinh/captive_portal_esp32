#ifndef NVS_STORAGE_H
#define NVS_STORAGE_H

void nvs_storage_init(void);
bool nvs_storage_test(void);
bool nvs_storage_get_wifi_credentials(char *ssid, char *password);
bool nvs_storage_set_wifi_credentials(const char *ssid, const char *password);
bool wifi_credentials_test(void);

#endif // NVS_STORAGE_H
