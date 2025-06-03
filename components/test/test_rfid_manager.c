#include "test_rfid_manager.h"

static const char *TAG = "TEST_RFID_MANAGER";

void setUp(void)
{
    // Initialize SPIFFS storage
    // Initialize RFID manager
    TEST_ASSERT_TRUE(rfid_manager_init());
}

void tearDown(void)
{
    // Deinitialize SPIFFS storage
    spiffs_storage_deinit();
}

void test_rfid_manager_add_card(void)
{
    // Test adding a card
    uint32_t card_id = 12345678;
    const char *card_name = "Test Card";

    esp_err_t ret = rfid_manager_add_card(card_id, card_name);
    TEST_ASSERT_TRUE(ret == ESP_OK);

    // Check if the card was added
    rfid_card_t card;
    ret = rfid_manager_get_card_count();
    TEST_ASSERT_TRUE(ret == ESP_OK);
    TEST_ASSERT_EQUAL_UINT32(card.card_id, card_id);
    TEST_ASSERT_EQUAL_STRING(card.name, card_name);
}

void test_rfid_manager_remove_card(void)
{
    // Test removing a card
    uint32_t card_id = 12345678;

    esp_err_t ret = rfid_manager_remove_card(card_id);
    TEST_ASSERT_TRUE(ret == ESP_OK);

    // Check if the card was removed
    rfid_card_t card;
    ret = rfid_manager_get_card_count();
    TEST_ASSERT_TRUE(ret == ESP_ERR_NOT_FOUND);
}

void test_rfid_manager_get_card(void)
{
    // Test getting a card
    uint32_t card_id = 12345678;
    rfid_card_t card;

    esp_err_t ret = rfid_manager_get_card_count();
    TEST_ASSERT_TRUE(ret == ESP_OK);
    ret = rfid_manager_get_card_count();
    TEST_ASSERT_TRUE(ret == ESP_OK);
    TEST_ASSERT_EQUAL_UINT32(card.card_id, card_id);
    TEST_ASSERT_EQUAL_STRING(card.name, "Test Card"); // Assuming the card was added in test_rfid_manager_add_card
    TEST_ASSERT_TRUE(card.active == 1);               // Assuming the card is active
}

void test_rfid_manager_get_card_count(void)
{
    // Test getting the card count
    uint16_t count = rfid_manager_get_card_count();
    TEST_ASSERT_GREATER_THAN(0, count);
}

void test_rfid_manager_list_cards(void)
{
    // Test listing cards
    rfid_card_t cards[200];
    esp_err_t ret = rfid_manager_list_cards(cards, 200);
    TEST_ASSERT_TRUE(ret == ESP_OK);

    // Check if the list is not empty
    uint16_t count = rfid_manager_get_card_count();
    TEST_ASSERT_GREATER_THAN(0, count);
    TEST_ASSERT_GREATER_THAN(0, cards[0].card_id); // Assuming at least one card exists
}

void test_rfid_manager_format_database(void)
{
    // Test formatting the database
    esp_err_t ret = rfid_manager_format_database();
    TEST_ASSERT_TRUE(ret == ESP_OK);

    // Check if the database is empty
    uint16_t count = rfid_manager_get_card_count();
    TEST_ASSERT_EQUAL_UINT16(0, count);
}

void test_rfid_manager_load_defaults(void)
{
    // Test loading default cards
    esp_err_t ret = rfid_manager_load_defaults();
    TEST_ASSERT_TRUE(ret == ESP_OK);

    // Check if the default cards are loaded
    uint16_t count = rfid_manager_get_card_count();
    TEST_ASSERT_GREATER_THAN(0, count);
}

void test_rfid_manager_check_card(void)
{
    // Test checking if a card exists
    uint32_t card_id = 12345678;
    esp_err_t ret = rfid_manager_check_card(card_id);
    TEST_ASSERT_TRUE(ret == ESP_OK); // Card should exist after adding it in test_rfid_manager_add_card
}

void test_rfid_manager(void)
{
    ESP_LOGI(TAG, "Starting RFID Manager tests...");
    // Run all tests
    test_rfid_manager_add_card();
    test_rfid_manager_remove_card();
    test_rfid_manager_get_card();
    test_rfid_manager_get_card_count();
    test_rfid_manager_list_cards();
    test_rfid_manager_format_database();
    test_rfid_manager_load_defaults();
    test_rfid_manager_check_card();
    // Additional tests can be added here
    ESP_LOGI(TAG, "RFID Manager tests completed successfully.");
}
