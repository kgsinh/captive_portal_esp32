#include "unity.h"
#include "esp_system.h"
#include "esp_log.h"
#include "rfid_manager.h"
#include "spiffs_storage.h"
#include <string.h>

static const char *TAG = "TEST_RFID_MANAGER";

// Test constants
#define TEST_CARD_ID_1 0x12345678
#define TEST_CARD_ID_2 0x87654321
#define TEST_CARD_NAME_1 "Test Card 1"
#define TEST_CARD_NAME_2 "Test Card 2"

TEST_CASE("RFID Manager: Initialize", "[rfid_manager]")
{
    esp_err_t ret = rfid_manager_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_TRUE(rfid_manager_is_database_valid());
}

TEST_CASE("RFID Manager: Add Card", "[rfid_manager]")
{
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_init());
    rfid_manager_format_database();

    uint32_t cards_added = 0;
    // add cards to test the limit
    for (uint32_t i = 0; i < 250; i++)
    {
        char card_name[32];
        snprintf(card_name, sizeof(card_name), "Card %lu", i);
        esp_err_t ret = rfid_manager_add_card(0x10000000 + i, card_name);
        if (ret == ESP_OK)
        {
            cards_added++;
        }
        else
        {
            // Should get ESP_ERR_NO_MEM when limit is reached
            TEST_ASSERT_EQUAL(ESP_ERR_NO_MEM, ret);
            break;
        }
    }

    uint16_t count = rfid_manager_get_card_count();
    TEST_ASSERT_EQUAL_UINT16(cards_added, count);

    ESP_LOGI(TAG, "Successfully added %lu cards before hitting limit", cards_added);
}

TEST_CASE("RFID Manager: Remove Card", "[rfid_manager]")
{
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_init());
    rfid_manager_format_database();

    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_add_card(TEST_CARD_ID_1, TEST_CARD_NAME_1));

    esp_err_t ret = rfid_manager_remove_card(TEST_CARD_ID_1);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    uint16_t count = rfid_manager_get_card_count();
    TEST_ASSERT_EQUAL_UINT16(0, count);

    ret = rfid_manager_check_card(TEST_CARD_ID_1);
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, ret);
}

TEST_CASE("RFID Manager: Check Card", "[rfid_manager]")
{
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_init());
    rfid_manager_format_database();

    esp_err_t ret = rfid_manager_check_card(TEST_CARD_ID_1);
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, ret);

    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_add_card(TEST_CARD_ID_1, TEST_CARD_NAME_1));
    ret = rfid_manager_check_card(TEST_CARD_ID_1);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

TEST_CASE("RFID Manager: List Cards", "[rfid_manager]")
{
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_init());
    rfid_manager_format_database();

    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_add_card(TEST_CARD_ID_1, TEST_CARD_NAME_1));
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_add_card(TEST_CARD_ID_2, TEST_CARD_NAME_2));

    rfid_card_t cards[10];
    esp_err_t ret = rfid_manager_list_cards(cards, 10);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    TEST_ASSERT_EQUAL_UINT32(TEST_CARD_ID_1, cards[0].card_id);
    TEST_ASSERT_EQUAL_STRING(TEST_CARD_NAME_1, cards[0].name);
    TEST_ASSERT_EQUAL_UINT8(1, cards[0].active);

    TEST_ASSERT_EQUAL_UINT32(TEST_CARD_ID_2, cards[1].card_id);
    TEST_ASSERT_EQUAL_STRING(TEST_CARD_NAME_2, cards[1].name);
    TEST_ASSERT_EQUAL_UINT8(1, cards[1].active);
}

TEST_CASE("RFID Manager: Get Card Count", "[rfid_manager]")
{
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_init());
    rfid_manager_format_database();

    uint16_t count = rfid_manager_get_card_count();
    TEST_ASSERT_EQUAL_UINT16(0, count);

    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_add_card(TEST_CARD_ID_1, TEST_CARD_NAME_1));
    count = rfid_manager_get_card_count();
    TEST_ASSERT_EQUAL_UINT16(1, count);

    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_add_card(TEST_CARD_ID_2, TEST_CARD_NAME_2));
    count = rfid_manager_get_card_count();
    TEST_ASSERT_EQUAL_UINT16(2, count);
}

TEST_CASE("RFID Manager: Load Defaults", "[rfid_manager]")
{
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_init());
    rfid_manager_format_database();

    esp_err_t ret = rfid_manager_load_defaults();
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    uint16_t count = rfid_manager_get_card_count();
    TEST_ASSERT_EQUAL_UINT16(3, count);

    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_check_card(0x12345678)); // Admin Card
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_check_card(0x87654321)); // User Card 1
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_check_card(0xABCDEF00)); // User Card 2
}

TEST_CASE("RFID Manager: Get JSON", "[rfid_manager]")
{
    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_init());
    rfid_manager_format_database();

    TEST_ASSERT_EQUAL(ESP_OK, rfid_manager_add_card(TEST_CARD_ID_1, TEST_CARD_NAME_1));

    char json_buffer[1024];
    esp_err_t ret = rfid_manager_get_card_list_json(json_buffer, sizeof(json_buffer));
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    TEST_ASSERT_NOT_NULL(strstr(json_buffer, "\"status\""));
    TEST_ASSERT_NOT_NULL(strstr(json_buffer, "\"count\""));
    TEST_ASSERT_NOT_NULL(strstr(json_buffer, "\"cards\""));
    TEST_ASSERT_NOT_NULL(strstr(json_buffer, TEST_CARD_NAME_1));
}
