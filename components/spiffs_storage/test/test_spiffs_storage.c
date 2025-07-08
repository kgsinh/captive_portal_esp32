#include "unity.h"
#include "esp_system.h"
#include "esp_log.h"
#include "spiffs_storage.h"

static const char *TAG = "TEST_SPIFFS_STORAGE";

#define TEST_FILE_CONTENT "This is a test file content."
#define TEST_FILE_NAME "/spiffs/test_file.txt"

TEST_CASE("SPIFFS Storage: Full Test", "[spiffs_storage]")
{
    TEST_ASSERT_TRUE(spiffs_storage_init());

    TEST_ASSERT_TRUE(spiffs_storage_create_file(TEST_FILE_NAME));

    TEST_ASSERT_TRUE(spiffs_storage_write_file(TEST_FILE_NAME, TEST_FILE_CONTENT, sizeof(TEST_FILE_CONTENT), false, true));

    TEST_ASSERT_TRUE(spiffs_storage_file_exists(TEST_FILE_NAME));

    char buffer[128] = {0};
    TEST_ASSERT_TRUE(spiffs_storage_read_file(TEST_FILE_NAME, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING(TEST_FILE_CONTENT, buffer);

    int32_t size = spiffs_storage_get_file_size(TEST_FILE_NAME);
    TEST_ASSERT_GREATER_THAN(0, size);

    const char *new_file_name = "/spiffs/renamed_test_file.txt";
    TEST_ASSERT_TRUE(spiffs_storage_rename_file(TEST_FILE_NAME, new_file_name));

    TEST_ASSERT_FALSE(spiffs_storage_file_exists(TEST_FILE_NAME));
    TEST_ASSERT_TRUE(spiffs_storage_file_exists(new_file_name));

    TEST_ASSERT_TRUE(spiffs_storage_list_files());

    char line_buffer[64] = {0};
    TEST_ASSERT_TRUE(spiffs_storage_read_file_line(new_file_name, line_buffer, sizeof(line_buffer)));
    ESP_LOGI(TAG, "Read line from file: %s", line_buffer);
    TEST_ASSERT_EQUAL_STRING(TEST_FILE_CONTENT, line_buffer);

    TEST_ASSERT_TRUE(spiffs_storage_delete_file(new_file_name));
    TEST_ASSERT_FALSE(spiffs_storage_file_exists(new_file_name));

    spiffs_storage_deinit();
}
