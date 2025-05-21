#include <stdio.h>
#include <inttypes.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "nvs_storage.h"

void nvs_storage_init(void)
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    // nvs_storage_test();
    // wifi_credentials_test();
}

bool nvs_storage_test(void)
{
    esp_err_t err;
    // Open
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle_t my_handle;
    err = nvs_open("nvs", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
        printf("Done\n");

        // Read
        printf("Reading restart counter from NVS ... ");
        int32_t restart_counter = 0; // value will default to 0, if not set yet in NVS
        err = nvs_get_i32(my_handle, "restart_counter", &restart_counter);
        switch (err)
        {
        case ESP_OK:
            printf("Done\n");
            printf("Restart counter = %" PRIu32 "\n", restart_counter);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            printf("The value is not initialized yet!\n");
            break;
        default:
            printf("Error (%s) reading!\n", esp_err_to_name(err));
        }

        // Write
        printf("Updating restart counter in NVS ... ");
        restart_counter++;
        err = nvs_set_i32(my_handle, "restart_counter", restart_counter);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        // Close
        nvs_close(my_handle);
    }
    return true;
}

bool nvs_storage_set_wifi_credentials(const char *ssid, const char *password)
{
    esp_err_t err;
    nvs_handle_t my_handle;

    // Open
    err = nvs_open("nvs", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return false;
    }

    // Write SSID
    err = nvs_set_str(my_handle, "_ssid", ssid);
    if (err != ESP_OK)
    {
        printf("Error (%s) writing SSID!\n", esp_err_to_name(err));
        nvs_close(my_handle);
        return false;
    }

    // Write Password
    err = nvs_set_str(my_handle, "_password", password);
    if (err != ESP_OK)
    {
        printf("Error (%s) writing Password!\n", esp_err_to_name(err));
        nvs_close(my_handle);
        return false;
    }

    // Commit written value.
    err = nvs_commit(my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%s) committing!\n", esp_err_to_name(err));
        nvs_close(my_handle);
        return false;
    }

    // Close
    nvs_close(my_handle);
    return true;
}

bool nvs_storage_get_wifi_credentials(char *ssid, char *password)
{
    esp_err_t err;
    nvs_handle_t my_handle;

    // Open
    err = nvs_open("nvs", NVS_READONLY, &my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return false;
    }

    // Read SSID
    size_t ssid_len = 0;
    err = nvs_get_str(my_handle, "_ssid", NULL, &ssid_len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        printf("Error (%s) reading SSID length!\n", esp_err_to_name(err));
        nvs_close(my_handle);
        return false;
    }
    if (ssid_len > 0)
    {
        err = nvs_get_str(my_handle, "_ssid", ssid, &ssid_len);
        if (err != ESP_OK)
        {
            printf("Error (%s) reading SSID!\n", esp_err_to_name(err));
            nvs_close(my_handle);
            return false;
        }
        ssid[ssid_len] = '\0'; // Null-terminate the string
    }

    // Read Password
    size_t password_len = 0;
    err = nvs_get_str(my_handle, "_password", NULL, &password_len);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        printf("Error (%s) reading Password length!\n", esp_err_to_name(err));
        nvs_close(my_handle);
        return false;
    }
    if (password_len > 0)
    {
        err = nvs_get_str(my_handle, "_password", password, &password_len);
        if (err != ESP_OK)
        {
            printf("Error (%s) reading Password!\n", esp_err_to_name(err));
            nvs_close(my_handle);
            return false;
        }
        password[password_len] = '\0'; // Null-terminate the string
    }

    // Close
    nvs_close(my_handle);

    printf("ssid_len: %d\n", ssid_len);
    printf("password_len: %d\n", password_len);

    if (ssid_len == 0)
    {
        printf("SSID or Password not found in NVS.\n");
        return false;
    }

    return true;
}

bool wifi_credentials_test(void)
{
    char ssid[64] = {0};
    char password[64] = {0};

    if (nvs_storage_get_wifi_credentials(ssid, password))
    {
        printf("SSID: %s\n", ssid);
        printf("Password: %s\n", password);
    }
    else
    {
        printf("Failed to get Wi-Fi credentials from NVS.\n");
    }

    // Test setting Wi-Fi credentials
    if (nvs_storage_set_wifi_credentials("TestSSID", "TestPassword"))
    {
        printf("Successfully set Wi-Fi credentials in NVS.\n");
    }
    else
    {
        printf("Failed to set Wi-Fi credentials in NVS.\n");
    }
    return true;
}
