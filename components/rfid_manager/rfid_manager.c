#include <stdio.h>
#include "rfid_manager.h"

// RFID database header
typedef struct
{
    uint16_t card_count; // Number of cards in the database
    uint16_t max_cards;  // Maximum number of cards allowed in the database
    uint32_t checksum;   // Checksum for database integrity
} rfid_database_t;

esp_err_t rfid_manager_init(void)
{
    // Initialize SPIFFS storage
    esp_err_t ret = spiffs_storage_init();
    if (ret != ESP_OK)
    {
        return ret;
    }

    // Load the database from file
    return rfid_manager_load_from_file();
}

esp_err_t rfid_manager_load_defaults(void)
{
    // Create a default database file if it doesn't exist
    if (!spiffs_storage_file_exists("/spiffs/rfid_database.bin"))
    {
        rfid_database_t default_db = {0, 200, 0}; // Initialize with 0 cards, max 200 cards
        spiffs_storage_write_file("/spiffs/rfid_database.bin", (const char *)&default_db, sizeof(default_db), false, true);
    }
    return ESP_OK;
}

esp_err_t rfid_manager_add_card(uint32_t card_id, const char *name)
{
    // Load the database
    rfid_database_t db;
    if (!spiffs_storage_read_file("/spiffs/rfid_database.bin", (char *)&db, sizeof(db)))
    {
        return ESP_FAIL; // Failed to read database
    }

    // Check if the card already exists
    rfid_card_t cards[db.max_cards];
    if (!rfid_manager_list_cards(cards, db.max_cards))
    {
        return ESP_FAIL; // Failed to list cards
    }

    for (uint16_t i = 0; i < db.card_count; i++)
    {
        if (cards[i].card_id == card_id)
        {
            return ESP_ERR_INVALID_STATE; // Card already exists
        }
    }

    // Add the new card
    if (db.card_count < db.max_cards)
    {
        rfid_card_t new_card = {card_id, 1, "", 0};
        strncpy(new_card.name, name, sizeof(new_card.name) - 1);
        new_card.name[sizeof(new_card.name) - 1] = '\0'; // Ensure null termination

        // Write the updated database back to file
        db.card_count++;
        spiffs_storage_write_file("/spiffs/rfid_database.bin", (const char *)&db, sizeof(db), false, true);
        spiffs_storage_write_file("/spiffs/rfid_cards.bin", (const char *)&new_card, sizeof(new_card), true, true);
        return ESP_OK;
    }
    else
    {
        return ESP_ERR_NO_MEM; // Database is full
    }
}

esp_err_t rfid_manager_remove_card(uint32_t card_id)
{
    // Load the database
    rfid_database_t db;
    if (!spiffs_storage_read_file("/spiffs/rfid_database.bin", (char *)&db, sizeof(db)))
    {
        return ESP_FAIL; // Failed to read database
    }

    // Load the cards
    rfid_card_t cards[db.max_cards];
    if (!rfid_manager_list_cards(cards, db.max_cards))
    {
        return ESP_FAIL; // Failed to list cards
    }

    // Find and remove the card
    for (uint16_t i = 0; i < db.card_count; i++)
    {
        if (cards[i].card_id == card_id)
        {
            // Shift remaining cards down
            for (uint16_t j = i; j < db.card_count - 1; j++)
            {
                cards[j] = cards[j + 1];
            }
            db.card_count--;

            size_t new_size = db.card_count * sizeof(rfid_card_t);
            // Write the updated database back to file
            spiffs_storage_write_file("/spiffs/rfid_database.bin", (const char *)&db, sizeof(db), false, true);
            bool write_ok = spiffs_storage_write_file("/spiffs/rfid_cards.bin", (const char *)cards, new_size, false, true);

            if (!write_ok)
            {
                return ESP_FAIL;
            }

            return ESP_OK;
        }
    }
    return ESP_ERR_NOT_FOUND; // Card not found
}

esp_err_t rfid_manager_check_card(uint32_t card_id)
{
    // Load the database
    rfid_database_t db;
    if (!spiffs_storage_read_file("/spiffs/rfid_database.bin", (char *)&db, sizeof(db)))
    {
        return ESP_FAIL; // Failed to read database
    }

    // Load the cards
    rfid_card_t cards[db.max_cards];
    if (!rfid_manager_list_cards(cards, db.max_cards))
    {
        return ESP_FAIL; // Failed to list cards
    }

    // Check if the card exists
    for (uint16_t i = 0; i < db.card_count; i++)
    {
        if (cards[i].card_id == card_id)
        {
            return ESP_OK; // Card found
        }
    }
    return ESP_ERR_NOT_FOUND; // Card not found
}

uint16_t rfid_manager_get_card_count(void)
{
    // Load the database
    rfid_database_t db;
    if (!spiffs_storage_read_file("/spiffs/rfid_database.bin", (char *)&db, sizeof(db)))
    {
        return 0; // Failed to read database
    }
    return db.card_count;
}

esp_err_t rfid_manager_list_cards(rfid_card_t *cards, uint16_t max_cards)
{
    // Load the database
    rfid_database_t db;
    if (!spiffs_storage_read_file("/spiffs/rfid_database.bin", (char *)&db, sizeof(db)))
    {
        return ESP_FAIL; // Failed to read database
    }

    // Check if the provided buffer is large enough
    if (max_cards < db.card_count)
    {
        return ESP_ERR_NO_MEM; // Not enough space in the provided buffer
    }

    // Load the cards
    if (!spiffs_storage_read_file("/spiffs/rfid_cards.bin", (char *)cards, db.card_count * sizeof(rfid_card_t)))
    {
        return ESP_FAIL; // Failed to read cards
    }

    return ESP_OK; // Success
}

esp_err_t rfid_manager_save_to_file(void)
{
    // Save the current database to file
    rfid_database_t db;
    if (!spiffs_storage_read_file("/spiffs/rfid_database.bin", (char *)&db, sizeof(db)))
    {
        return ESP_FAIL; // Failed to read database
    }

    // Write the database back to file
    if (!spiffs_storage_write_file("/spiffs/rfid_database.bin", (const char *)&db, sizeof(db), false, true))
    {
        return ESP_FAIL; // Failed to write database
    }

    return ESP_OK; // Success
}

esp_err_t rfid_manager_load_from_file(void)
{
    // Load the database from file
    if (!spiffs_storage_file_exists("/spiffs/rfid_database.bin"))
    {
        return ESP_ERR_NOT_FOUND; // Database file does not exist
    }

    rfid_database_t db;
    if (!spiffs_storage_read_file("/spiffs/rfid_database.bin", (char *)&db, sizeof(db)))
    {
        return ESP_FAIL; // Failed to read database
    }

    // Validate the database
    if (db.card_count > db.max_cards)
    {
        return ESP_ERR_INVALID_STATE; // Invalid database state
    }

    return ESP_OK; // Success
}

esp_err_t rfid_manager_format_database(void)
{
    // Create a new empty database
    rfid_database_t db = {0, 200, 0}; // Initialize with 0 cards, max 200 cards

    // Write the new database to file
    if (!spiffs_storage_write_file("/spiffs/rfid_database.bin", (const char *)&db, sizeof(db), false, true))
    {
        return ESP_FAIL; // Failed to write database
    }

    // Optionally clear the card file
    spiffs_storage_delete_file("/spiffs/rfid_cards.bin");

    return ESP_OK; // Success
}

bool rfid_manager_is_database_valid(void)
{
    // Check if the database file exists
    if (!spiffs_storage_file_exists("/spiffs/rfid_database.bin"))
    {
        return false; // Database file does not exist
    }

    // Load the database
    rfid_database_t db;
    if (!spiffs_storage_read_file("/spiffs/rfid_database.bin", (char *)&db, sizeof(db)))
    {
        return false; // Failed to read database
    }

    // Validate the database state
    if (db.card_count > db.max_cards || db.card_count < 0)
    {
        return false; // Invalid card count
    }

    return true; // Database is valid
}