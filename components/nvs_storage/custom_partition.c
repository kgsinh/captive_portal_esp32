#include "custom_partition.h"

#define NVS_PARAMS_PARTITION_NAME "params"

void nvs_custom_partition_init(void)
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init_partition(NVS_PARAMS_PARTITION_NAME);

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase_partition(NVS_PARAMS_PARTITION_NAME));
        err = nvs_flash_init_partition(NVS_PARAMS_PARTITION_NAME);
    }

    if (err != ESP_OK)
    {
        printf("Error (%s) initializing NVS partition!\n", esp_err_to_name(err));
        return;
    }

    nvs_custom_partition_test();
}

bool nvs_custom_partition_set_params(const char *key, const char *value)
{
    esp_err_t err;
    nvs_handle_t my_handle;

    // Open
    err = nvs_open_from_partition(NVS_PARAMS_PARTITION_NAME, "cust_part", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return false;
    }

    // Write key-value pair
    err = nvs_set_str(my_handle, key, value);
    if (err != ESP_OK)
    {
        printf("Error (%s) writing key-value pair!\n", esp_err_to_name(err));
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

bool nvs_custom_partition_get_params(const char *key, char *value, size_t value_len)
{
    esp_err_t err;
    nvs_handle_t my_handle;

    // Open
    err = nvs_open_from_partition(NVS_PARAMS_PARTITION_NAME, "cust_part", NVS_READONLY, &my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return false;
    }

    // Read key-value pair
    size_t required_size = 0;
    err = nvs_get_str(my_handle, key, NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        printf("Error (%s) reading key-value pair length!\n", esp_err_to_name(err));
        nvs_close(my_handle);
        return false;
    }

    if (required_size > value_len)
    {
        printf("Buffer size is too small for the value.\n");
        nvs_close(my_handle);
        return false;
    }

    err = nvs_get_str(my_handle, key, value, &required_size);
    if (err != ESP_OK)
    {
        printf("Error (%s) reading key-value pair!\n", esp_err_to_name(err));
        nvs_close(my_handle);
        return false;
    }

    // Null-terminate the string
    //   value[required_size] = '\0';

    // Close
    nvs_close(my_handle);

    //  printf("Successfully read key-value pair: %s = %s\n", key, value);
    // Check if the value is empty
    if (strlen(value) == 0)
    {
        printf("The value is empty.\n");
        return false;
    }

    return true;
}

// test function
void nvs_custom_partition_test(void)
{
    char value[64] = {0};

    // Test setting a parameter
    if (nvs_custom_partition_set_params("test_key", "Test Value"))
    {
        printf("Successfully set parameter in NVS.\n");
    }
    else
    {
        printf("Failed to set parameter in NVS.\n");
    }

    memset(value, 0, sizeof(value)); // Clear the buffer

    // Test getting a parameter
    if (nvs_custom_partition_get_params("test_key", value, sizeof(value)))
    {
        printf("Successfully got parameter from NVS: %s\n", value);
    }
    else
    {
        printf("Failed to get parameter from NVS.\n");
    }

    // Test setting a parameter
    if (nvs_custom_partition_set_params("test_key", "Test Val"))
    {
        printf("Successfully set parameter in NVS.\n");
    }
    else
    {
        printf("Failed to set parameter in NVS.\n");
    }

    memset(value, 0, sizeof(value)); // Clear the buffer

    // Test getting a parameter
    if (nvs_custom_partition_get_params("test_key", value, sizeof(value)))
    {
        printf("Successfully got parameter from NVS: %s\n", value);
    }
    else
    {
        printf("Failed to get parameter from NVS.\n");
    }
}