#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "rfid_manager.h"

// File paths for RFID database
#define RFID_DB_PATH "/spiffs/rfid_database.bin"
#define RFID_CARDS_PATH "/spiffs/rfid_cards.bin"

// Admin card protection
#define ADMIN_CARD_ID 0x12345678

static const char *TAG = "rfid_manager";
static SemaphoreHandle_t rfid_mutex = NULL;

// RFID database header
typedef struct
{
    uint16_t card_count; // Number of cards in the database
    uint16_t max_cards;  // Maximum number of cards allowed in the database
    uint32_t checksum;   // Checksum for database integrity
} rfid_database_t;

// Default RFID cards
static const rfid_card_t default_cards[] = {
    {0x12345678, 1, "Admin Card", 0},
    {0x87654321, 1, "User Card 1", 0},
    {0xABCDEF00, 1, "User Card 2", 0}};

esp_err_t rfid_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing RFID manager");

    // Create mutex if it doesn't exist
    if (rfid_mutex == NULL)
    {
        rfid_mutex = xSemaphoreCreateMutex();
        if (rfid_mutex == NULL)
        {
            ESP_LOGE(TAG, "Failed to create rfid_mutex");
            return ESP_FAIL;
        }
    }

    // Initialize SPIFFS storage only if not already initialized
    if (!spiffs_storage_is_initialized())
    {
        if (!spiffs_storage_init())
        {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS storage");
            return ESP_FAIL;
        }
    }
    else
    {
        ESP_LOGI(TAG, "SPIFFS already initialized, skipping initialization");
    }

    // Check if the database exists, if not create it
    if (!spiffs_storage_file_exists(RFID_DB_PATH))
    {
        ESP_LOGI(TAG, "RFID database not found, creating new database");
        rfid_database_t db = {0, 200, 0}; // Initialize with 0 cards, max 200 cards
        if (!spiffs_storage_write_file(RFID_DB_PATH, (const char *)&db, sizeof(db), false, true))
        {
            ESP_LOGE(TAG, "Failed to create RFID database");
            return ESP_FAIL;
        }
    }

    // Load the database from file
    esp_err_t ret = rfid_manager_load_from_file();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to load RFID database: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "RFID manager initialized successfully");
    return ESP_OK;
}

esp_err_t rfid_manager_load_defaults(void)
{
    ESP_LOGI(TAG, "Loading default RFID cards");

    // Check if the database file exists
    if (!spiffs_storage_file_exists(RFID_DB_PATH))
    {
        // Create a new database file with default values
        rfid_database_t db = {0, 200, 0}; // Initialize with 0 cards, max 200 cards
        if (!spiffs_storage_write_file(RFID_DB_PATH, (const char *)&db, sizeof(db), false, true))
        {
            ESP_LOGE(TAG, "Failed to create RFID database");
            return ESP_FAIL;
        }
    }

    // Load default cards into the database
    for (size_t i = 0; i < sizeof(default_cards) / sizeof(default_cards[0]); i++)
    {
        esp_err_t ret = rfid_manager_add_card(default_cards[i].card_id, default_cards[i].name);
        if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) // Ignore if card already exists
        {
            ESP_LOGE(TAG, "Failed to add default card %lu: %s",
                     (unsigned long)default_cards[i].card_id, esp_err_to_name(ret));
            return ret;
        }
    }

    ESP_LOGI(TAG, "Default RFID cards loaded successfully");
    return ESP_OK;
}

esp_err_t rfid_manager_add_card(uint32_t card_id, const char *name)
{
    ESP_LOGI(TAG, "Adding RFID card: %lu, name: %s", (unsigned long)card_id, name ? name : "NULL");

    if (name == NULL)
    {
        ESP_LOGE(TAG, "Invalid name (NULL)");
        return ESP_ERR_INVALID_ARG;
    }

    // Acquire mutex for thread safety
    if (xSemaphoreTake(rfid_mutex, portMAX_DELAY) != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to take rfid_mutex");
        return ESP_FAIL;
    }

    // Load the database
    rfid_database_t db;
    if (!spiffs_storage_read_file(RFID_DB_PATH, (char *)&db, sizeof(db)))
    {
        ESP_LOGE(TAG, "Failed to read RFID database");
        xSemaphoreGive(rfid_mutex);
        return ESP_FAIL;
    }

    // Check if the card already exists
    rfid_card_t *cards = NULL;
    esp_err_t result = ESP_FAIL;

    if (db.card_count > 0)
    {
        cards = (rfid_card_t *)malloc(db.card_count * sizeof(rfid_card_t));
        if (cards == NULL)
        {
            ESP_LOGE(TAG, "Failed to allocate memory for cards");
            xSemaphoreGive(rfid_mutex);
            return ESP_ERR_NO_MEM;
        }

        if (!spiffs_storage_read_file(RFID_CARDS_PATH, (char *)cards, db.card_count * sizeof(rfid_card_t)))
        {
            ESP_LOGE(TAG, "Failed to read RFID cards");
            free(cards);
            xSemaphoreGive(rfid_mutex);
            return ESP_FAIL;
        }

        for (uint16_t i = 0; i < db.card_count; i++)
        {
            if (cards[i].card_id == card_id)
            {
                ESP_LOGW(TAG, "Card already exists: %lu", (unsigned long)card_id);
                free(cards);
                xSemaphoreGive(rfid_mutex);
                return ESP_ERR_INVALID_STATE;
            }
        }

        free(cards);
    }

    // Add the new card
    if (db.card_count < db.max_cards)
    {
        rfid_card_t new_card = {card_id, 1, "", 0};
        strncpy(new_card.name, name, sizeof(new_card.name) - 1);
        new_card.name[sizeof(new_card.name) - 1] = '\0'; // Ensure null termination
        new_card.timestamp = (uint32_t)time(NULL);       // Set current timestamp

        // Write the updated database back to file
        db.card_count++;
        if (!spiffs_storage_write_file(RFID_DB_PATH, (const char *)&db, sizeof(db), false, true))
        {
            ESP_LOGE(TAG, "Failed to update RFID database");
            xSemaphoreGive(rfid_mutex);
            return ESP_FAIL;
        }

        if (!spiffs_storage_write_file(RFID_CARDS_PATH, (const char *)&new_card, sizeof(new_card), true, true))
        {
            ESP_LOGE(TAG, "Failed to write new RFID card");
            // Revert database count
            db.card_count--;
            spiffs_storage_write_file(RFID_DB_PATH, (const char *)&db, sizeof(db), false, true);
            xSemaphoreGive(rfid_mutex);
            return ESP_FAIL;
        }

        ESP_LOGI(TAG, "Card added successfully: %lu", (unsigned long)card_id);
        result = ESP_OK;
    }
    else
    {
        ESP_LOGE(TAG, "Database is full (%u cards)", db.max_cards);
        result = ESP_ERR_NO_MEM;
    }

    xSemaphoreGive(rfid_mutex);
    return result;
}

esp_err_t rfid_manager_remove_card(uint32_t card_id)
{
    ESP_LOGI(TAG, "Removing RFID card: %lu", (unsigned long)card_id);

    // Check if attempting to remove the protected admin card
    if (card_id == ADMIN_CARD_ID)
    {
        ESP_LOGW(TAG, "Attempted to remove protected admin card: %lu", (unsigned long)card_id);
        return ESP_ERR_NOT_SUPPORTED;
    }

    // Acquire mutex for thread safety
    if (xSemaphoreTake(rfid_mutex, portMAX_DELAY) != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to take rfid_mutex");
        return ESP_FAIL;
    }

    // Load the database
    rfid_database_t db;
    if (!spiffs_storage_read_file(RFID_DB_PATH, (char *)&db, sizeof(db)))
    {
        ESP_LOGE(TAG, "Failed to read RFID database");
        xSemaphoreGive(rfid_mutex);
        return ESP_FAIL;
    }

    // Check if there are any cards
    if (db.card_count == 0)
    {
        ESP_LOGW(TAG, "No cards in database to remove");
        xSemaphoreGive(rfid_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    // Allocate memory for cards
    rfid_card_t *cards = (rfid_card_t *)malloc(db.card_count * sizeof(rfid_card_t));
    if (cards == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for cards");
        xSemaphoreGive(rfid_mutex);
        return ESP_ERR_NO_MEM;
    }

    // Load the cards
    if (!spiffs_storage_read_file(RFID_CARDS_PATH, (char *)cards, db.card_count * sizeof(rfid_card_t)))
    {
        ESP_LOGE(TAG, "Failed to read RFID cards");
        free(cards);
        xSemaphoreGive(rfid_mutex);
        return ESP_FAIL;
    }

    // Find and remove the card
    bool found = false;
    for (uint16_t i = 0; i < db.card_count; i++)
    {
        if (cards[i].card_id == card_id)
        {
            ESP_LOGI(TAG, "Found card to remove: %lu, name: %s",
                     (unsigned long)card_id, cards[i].name);

            // Shift remaining cards down
            for (uint16_t j = i; j < db.card_count - 1; j++)
            {
                cards[j] = cards[j + 1];
            }
            db.card_count--;
            found = true;
            break;
        }
    }

    if (!found)
    {
        ESP_LOGW(TAG, "Card not found: %lu", (unsigned long)card_id);
        free(cards);
        xSemaphoreGive(rfid_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    // Write the updated database back to file
    if (!spiffs_storage_write_file(RFID_DB_PATH, (const char *)&db, sizeof(db), false, true))
    {
        ESP_LOGE(TAG, "Failed to update RFID database");
        free(cards);
        xSemaphoreGive(rfid_mutex);
        return ESP_FAIL;
    }

    // Write the updated cards back to file or delete if no cards remain
    bool write_ok = true;
    if (db.card_count > 0)
    {
        size_t new_size = db.card_count * sizeof(rfid_card_t);
        write_ok = spiffs_storage_write_file(RFID_CARDS_PATH, (const char *)cards, new_size, false, true);
    }
    else
    {
        // No cards remaining, delete the cards file
        if (spiffs_storage_file_exists(RFID_CARDS_PATH))
        {
            write_ok = spiffs_storage_delete_file(RFID_CARDS_PATH);
        }
    }

    free(cards);

    if (!write_ok)
    {
        ESP_LOGE(TAG, "Failed to write updated RFID cards");
        xSemaphoreGive(rfid_mutex);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Card removed successfully: %lu", (unsigned long)card_id);
    xSemaphoreGive(rfid_mutex);
    return ESP_OK;
}

esp_err_t rfid_manager_check_card(uint32_t card_id)
{
    ESP_LOGI(TAG, "Checking RFID card: %lu", (unsigned long)card_id);

    // Acquire mutex for thread safety
    if (xSemaphoreTake(rfid_mutex, portMAX_DELAY) != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to take rfid_mutex");
        return ESP_FAIL;
    }

    // Load the database
    rfid_database_t db;
    if (!spiffs_storage_read_file(RFID_DB_PATH, (char *)&db, sizeof(db)))
    {
        ESP_LOGE(TAG, "Failed to read RFID database");
        xSemaphoreGive(rfid_mutex);
        return ESP_FAIL;
    }

    // Check if there are any cards
    if (db.card_count == 0)
    {
        ESP_LOGW(TAG, "No cards in database to check");
        xSemaphoreGive(rfid_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    // Allocate memory for cards
    rfid_card_t *cards = (rfid_card_t *)malloc(db.card_count * sizeof(rfid_card_t));
    if (cards == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for cards");
        xSemaphoreGive(rfid_mutex);
        return ESP_ERR_NO_MEM;
    }

    // Load the cards
    if (!spiffs_storage_read_file(RFID_CARDS_PATH, (char *)cards, db.card_count * sizeof(rfid_card_t)))
    {
        ESP_LOGE(TAG, "Failed to read RFID cards");
        free(cards);
        xSemaphoreGive(rfid_mutex);
        return ESP_FAIL;
    }

    // Check if the card exists
    esp_err_t result = ESP_ERR_NOT_FOUND;
    for (uint16_t i = 0; i < db.card_count; i++)
    {
        if (cards[i].card_id == card_id)
        {
            if (cards[i].active)
            {
                ESP_LOGI(TAG, "Card found and active: %lu, name: %s",
                         (unsigned long)card_id, cards[i].name);
                result = ESP_OK;
            }
            else
            {
                ESP_LOGW(TAG, "Card found but inactive: %lu, name: %s",
                         (unsigned long)card_id, cards[i].name);
                result = ESP_ERR_INVALID_STATE;
            }
            break;
        }
    }

    if (result == ESP_ERR_NOT_FOUND)
    {
        ESP_LOGW(TAG, "Card not found: %lu", (unsigned long)card_id);
    }

    free(cards);
    xSemaphoreGive(rfid_mutex);
    return result;
}

uint16_t rfid_manager_get_card_count(void)
{
    ESP_LOGI(TAG, "Getting RFID card count");

    // Acquire mutex for thread safety
    if (xSemaphoreTake(rfid_mutex, portMAX_DELAY) != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to take rfid_mutex");
        return 0;
    }

    // Load the database
    rfid_database_t db;
    if (!spiffs_storage_read_file(RFID_DB_PATH, (char *)&db, sizeof(db)))
    {
        ESP_LOGE(TAG, "Failed to read RFID database");
        xSemaphoreGive(rfid_mutex);
        return 0;
    }

    ESP_LOGI(TAG, "RFID card count: %u", db.card_count);
    xSemaphoreGive(rfid_mutex);
    return db.card_count;
}

esp_err_t rfid_manager_list_cards(rfid_card_t *cards, uint16_t max_cards)
{
    ESP_LOGI(TAG, "Listing RFID cards (max: %u)", max_cards);

    if (cards == NULL)
    {
        ESP_LOGE(TAG, "Invalid cards buffer (NULL)");
        return ESP_ERR_INVALID_ARG;
    }

    // Acquire mutex for thread safety
    if (xSemaphoreTake(rfid_mutex, portMAX_DELAY) != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to take rfid_mutex");
        return ESP_FAIL;
    }

    // Load the database
    rfid_database_t db;
    if (!spiffs_storage_read_file(RFID_DB_PATH, (char *)&db, sizeof(db)))
    {
        ESP_LOGE(TAG, "Failed to read RFID database");
        xSemaphoreGive(rfid_mutex);
        return ESP_FAIL;
    }

    // Check if the provided buffer is large enough
    if (max_cards < db.card_count)
    {
        ESP_LOGE(TAG, "Buffer too small: provided %u, needed %u", max_cards, db.card_count);
        xSemaphoreGive(rfid_mutex);
        return ESP_ERR_NO_MEM;
    }

    // If there are no cards, return success immediately
    if (db.card_count == 0)
    {
        ESP_LOGI(TAG, "No cards in database");
        xSemaphoreGive(rfid_mutex);
        return ESP_OK;
    }

    // Load the cards
    if (!spiffs_storage_read_file(RFID_CARDS_PATH, (char *)cards, db.card_count * sizeof(rfid_card_t)))
    {
        ESP_LOGE(TAG, "Failed to read RFID cards");
        xSemaphoreGive(rfid_mutex);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Successfully listed %u RFID cards", db.card_count);
    xSemaphoreGive(rfid_mutex);
    return ESP_OK;
}

esp_err_t rfid_manager_save_to_file(void)
{
    ESP_LOGI(TAG, "Saving RFID database to file");

    // Acquire mutex for thread safety
    if (xSemaphoreTake(rfid_mutex, portMAX_DELAY) != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to take rfid_mutex");
        return ESP_FAIL;
    }

    // Save the current database to file
    rfid_database_t db;
    if (!spiffs_storage_read_file(RFID_DB_PATH, (char *)&db, sizeof(db)))
    {
        ESP_LOGE(TAG, "Failed to read RFID database");
        xSemaphoreGive(rfid_mutex);
        return ESP_FAIL;
    }

    // Calculate checksum (simple implementation)
    db.checksum = 0;
    for (uint16_t i = 0; i < db.card_count; i++)
    {
        db.checksum += i; // Placeholder for a real checksum calculation
    }

    // Write the database back to file
    if (!spiffs_storage_write_file(RFID_DB_PATH, (const char *)&db, sizeof(db), false, true))
    {
        ESP_LOGE(TAG, "Failed to write RFID database");
        xSemaphoreGive(rfid_mutex);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "RFID database saved successfully");
    xSemaphoreGive(rfid_mutex);
    return ESP_OK;
}

esp_err_t rfid_manager_load_from_file(void)
{
    ESP_LOGI(TAG, "Loading RFID database from file");

    // Acquire mutex for thread safety
    if (xSemaphoreTake(rfid_mutex, portMAX_DELAY) != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to take rfid_mutex");
        return ESP_FAIL;
    }

    // Check if the database file exists
    if (!spiffs_storage_file_exists(RFID_DB_PATH))
    {
        ESP_LOGW(TAG, "RFID database file does not exist");
        xSemaphoreGive(rfid_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    // Load the database
    rfid_database_t db;
    if (!spiffs_storage_read_file(RFID_DB_PATH, (char *)&db, sizeof(db)))
    {
        ESP_LOGE(TAG, "Failed to read RFID database");
        xSemaphoreGive(rfid_mutex);
        return ESP_FAIL;
    }

    // Validate the database
    if (db.card_count > db.max_cards)
    {
        ESP_LOGE(TAG, "Invalid database state: card_count (%u) > max_cards (%u)",
                 db.card_count, db.max_cards);
        xSemaphoreGive(rfid_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    // Check if the cards file exists if there are cards in the database
    if (db.card_count > 0 && !spiffs_storage_file_exists(RFID_CARDS_PATH))
    {
        ESP_LOGE(TAG, "RFID cards file does not exist but database has %u cards", db.card_count);
        xSemaphoreGive(rfid_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "RFID database loaded successfully: %u cards", db.card_count);
    xSemaphoreGive(rfid_mutex);
    return ESP_OK;
}

esp_err_t rfid_manager_format_database(void)
{
    ESP_LOGI(TAG, "Formatting RFID database");

    // Acquire mutex for thread safety
    if (xSemaphoreTake(rfid_mutex, portMAX_DELAY) != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to take rfid_mutex");
        return ESP_FAIL;
    }

    // Create a new empty database
    rfid_database_t db = {0, 200, 0}; // Initialize with 0 cards, max 200 cards

    // Write the new database to file
    if (!spiffs_storage_write_file(RFID_DB_PATH, (const char *)&db, sizeof(db), false, true))
    {
        ESP_LOGE(TAG, "Failed to write new RFID database");
        xSemaphoreGive(rfid_mutex);
        return ESP_FAIL;
    }

    // Delete the card file if it exists
    if (spiffs_storage_file_exists(RFID_CARDS_PATH))
    {
        if (!spiffs_storage_delete_file(RFID_CARDS_PATH))
        {
            ESP_LOGW(TAG, "Failed to delete RFID cards file");
            // Continue anyway, not a critical error
        }
    }

    ESP_LOGI(TAG, "RFID database formatted successfully");
    xSemaphoreGive(rfid_mutex);
    return ESP_OK;
}

esp_err_t rfid_manager_reset_to_defaults(void)
{
    ESP_LOGI(TAG, "Resetting RFID database to defaults");

    // First format the database to clear everything
    esp_err_t ret = rfid_manager_format_database();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to format database before loading defaults");
        return ret;
    }

    // Then load the default cards
    ret = rfid_manager_load_defaults();
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to load default cards");
        return ret;
    }

    ESP_LOGI(TAG, "RFID database reset to defaults successfully");
    return ESP_OK;
}

bool rfid_manager_is_database_valid(void)
{
    ESP_LOGI(TAG, "Checking if RFID database is valid");

    // Acquire mutex for thread safety
    if (xSemaphoreTake(rfid_mutex, portMAX_DELAY) != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to take rfid_mutex");
        return false;
    }

    // Check if the database file exists
    if (!spiffs_storage_file_exists(RFID_DB_PATH))
    {
        ESP_LOGW(TAG, "RFID database file does not exist");
        xSemaphoreGive(rfid_mutex);
        return false;
    }

    // Load the database
    rfid_database_t db;
    if (!spiffs_storage_read_file(RFID_DB_PATH, (char *)&db, sizeof(db)))
    {
        ESP_LOGE(TAG, "Failed to read RFID database");
        xSemaphoreGive(rfid_mutex);
        return false;
    }

    // Validate the database state
    if (db.card_count > db.max_cards)
    {
        ESP_LOGE(TAG, "Invalid database state: card_count (%u) > max_cards (%u)",
                 db.card_count, db.max_cards);
        xSemaphoreGive(rfid_mutex);
        return false;
    }

    // Check if the cards file exists if there are cards in the database
    if (db.card_count > 0 && !spiffs_storage_file_exists(RFID_CARDS_PATH))
    {
        ESP_LOGE(TAG, "RFID cards file does not exist but database has %u cards", db.card_count);
        xSemaphoreGive(rfid_mutex);
        return false;
    }

    // Verify file size matches expected size
    int32_t expected_size = db.card_count * sizeof(rfid_card_t);
    int32_t actual_size = spiffs_storage_get_file_size(RFID_CARDS_PATH);

    if (db.card_count > 0 && actual_size != expected_size)
    {
        ESP_LOGE(TAG, "RFID cards file size mismatch: expected %ld, got %ld",
                 (long)expected_size, (long)actual_size);
        xSemaphoreGive(rfid_mutex);
        return false;
    }

    ESP_LOGI(TAG, "RFID database is valid: %u cards", db.card_count);
    xSemaphoreGive(rfid_mutex);
    return true;
}

esp_err_t rfid_manager_get_card_list_json(char *buffer, size_t buffer_max_len)
{
    ESP_LOGI(TAG, "Getting RFID card list as JSON");

    rfid_database_t db;
    rfid_card_t *db_cards = NULL;
    uint16_t _length = 0;
    bool is_comma = false;
    esp_err_t result = ESP_OK;

    // Validate input parameters
    if (!buffer || buffer_max_len == 0)
    {
        ESP_LOGE(TAG, "Invalid buffer or buffer length");
        return ESP_ERR_INVALID_ARG;
    }

    // Acquire mutex for thread safety
    if (xSemaphoreTake(rfid_mutex, portMAX_DELAY) != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to take rfid_mutex");
        _length = snprintf(buffer, buffer_max_len, "{\"status\":\"error\",\"message\":\"Failed to acquire mutex\"}");
        return ESP_FAIL;
    }

    // Clear the buffer
    memset(buffer, 0, buffer_max_len);

    // Load the database
    if (!spiffs_storage_read_file(RFID_DB_PATH, (char *)&db, sizeof(db)))
    {
        ESP_LOGE(TAG, "Failed to read RFID database");
        _length = snprintf(buffer, buffer_max_len, "{\"status\":\"error\",\"message\":\"Failed to read database\"}");
        xSemaphoreGive(rfid_mutex);
        return ESP_FAIL;
    }

    // Start JSON response
    _length = snprintf(buffer, buffer_max_len, "{\"status\":\"ok\",\"count\":%u,\"cards\":[", db.card_count);

    // Load cards if there are any
    if (db.card_count > 0)
    {
        // Allocate memory for cards
        db_cards = (rfid_card_t *)malloc(db.card_count * sizeof(rfid_card_t));
        if (db_cards == NULL)
        {
            ESP_LOGE(TAG, "Failed to allocate memory for cards");
            _length = snprintf(buffer, buffer_max_len, "{\"status\":\"error\",\"message\":\"Memory allocation failed\"}");
            xSemaphoreGive(rfid_mutex);
            return ESP_ERR_NO_MEM;
        }

        if (!spiffs_storage_read_file(RFID_CARDS_PATH, (char *)db_cards, db.card_count * sizeof(rfid_card_t)))
        {
            ESP_LOGE(TAG, "Failed to read RFID cards");
            free(db_cards);
            _length = snprintf(buffer, buffer_max_len, "{\"status\":\"error\",\"message\":\"Failed to read cards\"}");
            xSemaphoreGive(rfid_mutex);
            return ESP_FAIL;
        }

        // Add cards to JSON
        for (uint16_t i = 0; i < db.card_count; i++)
        {
            // Check for buffer overflow with a safety margin
            if (_length + 100 >= buffer_max_len)
            {
                ESP_LOGW(TAG, "Buffer too small to include all cards, truncating");
                result = ESP_ERR_NO_MEM;
                break;
            }

            char id_str[16];
            snprintf(id_str, sizeof(id_str), "%lu", (unsigned long)db_cards[i].card_id);

            _length += snprintf(buffer + _length, buffer_max_len - _length,
                                "%s{\"id\":%s,\"name\":\"%s\",\"active\":%d,\"timestamp\":%lu}",
                                is_comma ? "," : "",
                                id_str,
                                db_cards[i].name,
                                db_cards[i].active,
                                (unsigned long)db_cards[i].timestamp);
            is_comma = true;
        }
    }

    // Close JSON array and object
    if (_length + 3 < buffer_max_len)
    {
        _length += snprintf(buffer + _length, buffer_max_len - _length, "]}");
    }
    else
    {
        // Ensure proper JSON closing even if we're at the buffer limit
        buffer[buffer_max_len - 3] = ']';
        buffer[buffer_max_len - 2] = '}';
        buffer[buffer_max_len - 1] = '\0';
        result = ESP_ERR_NO_MEM;
    }

    // Clean up
    if (db_cards != NULL)
    {
        free(db_cards);
    }

    ESP_LOGI(TAG, "Generated JSON for %u RFID cards", db.card_count);
    xSemaphoreGive(rfid_mutex);
    return result;
}
