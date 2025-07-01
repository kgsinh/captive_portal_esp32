#include "unity.h"
#include "rfid_manager.h"
#include "spiffs_storage.h"
#include <string.h>

static const char *TAG = "TEST_RFID_MANAGER";

// Test constants
#define TEST_CARD_ID_1 0x12345678
#define TEST_CARD_ID_2 0x87654321
#define TEST_CARD_NAME_1 "Test Card 1"
#define TEST_CARD_NAME_2 "Test Card 2"

// setUp and tearDown functions
void setUp(void)
{
    // Initialize RFID manager
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_init());

    // Start with clean database for each test
    rfid_manager_format_database();
}

void tearDown(void)
{
    // Clean up test database
    rfid_manager_format_database();
}

// =============================================================================
// BASIC FUNCTIONALITY TESTS
// =============================================================================

void test_rfid_manager_init(void)
{
    // Test initialization (already done in setUp, but verify it works)
    esp_err_t ret = rfid_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Verify database is valid
    TEST_ASSERT_TRUE(rfid_manager_is_database_valid());
}

void test_rfid_manager_add_card(void)
{
    // Add a card
    esp_err_t ret = rfid_manager_add_card(TEST_CARD_ID_1, TEST_CARD_NAME_1);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Verify card was added
    uint16_t count = rfid_manager_get_card_count();
    TEST_ASSERT_EQUAL_UINT16(1, count);

    // Verify card exists
    ret = rfid_manager_check_card(TEST_CARD_ID_1);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

void test_rfid_manager_remove_card(void)
{
    // Add a card first
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_add_card(TEST_CARD_ID_1, TEST_CARD_NAME_1));

    // Remove the card
    esp_err_t ret = rfid_manager_remove_card(TEST_CARD_ID_1);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Verify card was removed
    uint16_t count = rfid_manager_get_card_count();
    TEST_ASSERT_EQUAL_UINT16(0, count);

    // Verify card no longer exists
    ret = rfid_manager_check_card(TEST_CARD_ID_1);
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, ret);
}

void test_rfid_manager_check_card(void)
{
    // Check non-existent card
    esp_err_t ret = rfid_manager_check_card(TEST_CARD_ID_1);
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, ret);

    // Add card and check again
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_add_card(TEST_CARD_ID_1, TEST_CARD_NAME_1));
    ret = rfid_manager_check_card(TEST_CARD_ID_1);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

void test_rfid_manager_list_cards(void)
{
    // Add test cards
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_add_card(TEST_CARD_ID_1, TEST_CARD_NAME_1));
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_add_card(TEST_CARD_ID_2, TEST_CARD_NAME_2));

    // List cards
    rfid_card_t cards[10];
    esp_err_t ret = rfid_manager_list_cards(cards, 10);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Verify card data
    TEST_ASSERT_EQUAL_UINT32(TEST_CARD_ID_1, cards[0].card_id);
    TEST_ASSERT_EQUAL_STRING(TEST_CARD_NAME_1, cards[0].name);
    TEST_ASSERT_EQUAL_UINT8(1, cards[0].active);

    TEST_ASSERT_EQUAL_UINT32(TEST_CARD_ID_2, cards[1].card_id);
    TEST_ASSERT_EQUAL_STRING(TEST_CARD_NAME_2, cards[1].name);
    TEST_ASSERT_EQUAL_UINT8(1, cards[1].active);
}

void test_rfid_manager_get_card_count(void)
{
    // Empty database
    uint16_t count = rfid_manager_get_card_count();
    TEST_ASSERT_EQUAL_UINT16(0, count);

    // Add cards and check count
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_add_card(TEST_CARD_ID_1, TEST_CARD_NAME_1));
    count = rfid_manager_get_card_count();
    TEST_ASSERT_EQUAL_UINT16(1, count);

    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_add_card(TEST_CARD_ID_2, TEST_CARD_NAME_2));
    count = rfid_manager_get_card_count();
    TEST_ASSERT_EQUAL_UINT16(2, count);
}

void test_rfid_manager_load_defaults(void)
{
    // Load default cards
    esp_err_t ret = rfid_manager_load_defaults();
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Verify default cards are loaded
    uint16_t count = rfid_manager_get_card_count();
    TEST_ASSERT_EQUAL_UINT16(3, count);

    // Verify specific default cards exist
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_check_card(0x12345678)); // Admin Card
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_check_card(0x87654321)); // User Card 1
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_check_card(0xABCDEF00)); // User Card 2
}

void test_rfid_manager_get_json(void)
{
    // Add a card
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_add_card(TEST_CARD_ID_1, TEST_CARD_NAME_1));

    // Get JSON representation
    char json_buffer[1024];
    esp_err_t ret = rfid_manager_get_card_list_json(json_buffer, sizeof(json_buffer));
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Basic JSON validation - should contain expected fields
    TEST_ASSERT_NOT_NULL(strstr(json_buffer, "\"status\""));
    TEST_ASSERT_NOT_NULL(strstr(json_buffer, "\"count\""));
    TEST_ASSERT_NOT_NULL(strstr(json_buffer, "\"cards\""));
    TEST_ASSERT_NOT_NULL(strstr(json_buffer, TEST_CARD_NAME_1));
}

// =============================================================================
// MAIN TEST FUNCTION
// =============================================================================

void test_rfid_manager(void)
{
    // Run basic RFID manager tests
    RUN_TEST(test_rfid_manager_init);
    RUN_TEST(test_rfid_manager_add_card);
    RUN_TEST(test_rfid_manager_remove_card);
    RUN_TEST(test_rfid_manager_check_card);
    RUN_TEST(test_rfid_manager_list_cards);
    RUN_TEST(test_rfid_manager_get_card_count);
    RUN_TEST(test_rfid_manager_load_defaults);
    RUN_TEST(test_rfid_manager_get_json);
}
