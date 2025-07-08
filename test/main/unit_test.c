#include <stdio.h>
#include "unity.h"
#include "esp_log.h"

static const char *TAG = "unit_test";

static void print_banner(const char *text)
{
    printf("\n#### %s #####\n\n", text);
}

void app_main(void)
{
    print_banner("Starting Interactive Test Menu");

    UNITY_BEGIN();

    ESP_LOGI(TAG, "Test system initialized. Use the menu to run individual tests.");

    unity_run_menu();
}
