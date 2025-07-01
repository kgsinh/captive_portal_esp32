#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "esp_task_wdt.h"
#include "esp_log.h"
#include "spiffs_storage.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Test function declarations
void test_rfid_manager(void);

static void print_banner(const char *text);

static const char *TAG = "example_unit_test";

void app_main(void)
{
    print_banner("Running all the registered tests");
    UNITY_BEGIN();

    ESP_LOGI(TAG, "Initializing SPIFFS storage");

    // Try to deinitialize first in case it's already initialized
    spiffs_storage_deinit();

    // Now initialize SPIFFS
    if (!spiffs_storage_init())
    {
        ESP_LOGE(TAG, "Failed to initialize SPIFFS storage");
        UNITY_END();
        return;
    }

    RUN_TEST(test_rfid_manager);

    // unity_run_all_tests();

    UNITY_END();

    // Clean up
    spiffs_storage_deinit();

    // print_banner("Starting interactive test menu");
    /* This function will not return, and will be busy waiting for UART input.
     * Make sure that task watchdog is disabled if you use this function.
     */
    // unity_run_menu();
}

static void print_banner(const char *text)
{
    printf("\n#### %s #####\n\n", text);
}
