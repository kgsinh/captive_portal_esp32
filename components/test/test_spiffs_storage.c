#include "test_spiffs_storage.h"

static const char *TAG = "TEST_SPIFFS_STORAGE";

void test_spiffs_storage(void)
{
    // Initialize SPIFFS
    spiffs_storage_init();

    // Create a file
    TEST_ASSERT_TRUE(spiffs_storage_create_file(TEST_FILE_NAME));

    // Write to the file
    TEST_ASSERT_TRUE(spiffs_storage_write_file(TEST_FILE_NAME, TEST_FILE_CONTENT, false));

    // Check if the file exists
    TEST_ASSERT_TRUE(spiffs_storage_file_exists(TEST_FILE_NAME));

    // Read from the file
    char buffer[128] = {0};
    TEST_ASSERT_TRUE(spiffs_storage_read_file(TEST_FILE_NAME, buffer, sizeof(buffer)));
    TEST_ASSERT_EQUAL_STRING(TEST_FILE_CONTENT, buffer);

    // Get file size
    int32_t size = spiffs_storage_get_file_size(TEST_FILE_NAME);
    TEST_ASSERT_GREATER_THAN(0, size);

    // Rename the file
    const char *new_file_name = "/spiffs/renamed_test_file.txt";
    TEST_ASSERT_TRUE(spiffs_storage_rename_file(TEST_FILE_NAME, new_file_name));

    // Check if the old file name exists
    TEST_ASSERT_FALSE(spiffs_storage_file_exists(TEST_FILE_NAME));

    // Check if the new file name exists
    TEST_ASSERT_TRUE(spiffs_storage_file_exists(new_file_name));

    // List files in SPIFFS
    TEST_ASSERT_TRUE(spiffs_storage_list_files());

    // Read a line from the file
    char line_buffer[64] = {0};
    TEST_ASSERT_TRUE(spiffs_storage_read_file_line(new_file_name, line_buffer, sizeof(line_buffer)));
    ESP_LOGI(TAG, "Read line from file: %s", line_buffer);
    TEST_ASSERT_EQUAL_STRING(TEST_FILE_CONTENT, line_buffer);

    // Delete the renamed file
    TEST_ASSERT_TRUE(spiffs_storage_delete_file(new_file_name));

    // Check if the file was deleted
    TEST_ASSERT_FALSE(spiffs_storage_file_exists(new_file_name));

    // Deinitialize SPIFFS
    spiffs_storage_deinit();
}
