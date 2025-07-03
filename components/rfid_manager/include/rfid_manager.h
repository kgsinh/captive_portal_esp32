#ifndef RFID_MANAGER_H
#define RFID_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "spiffs_storage.h"

// RFID card structure
typedef struct
{
    uint32_t card_id;   // Unique identifier for the card
    uint8_t active;     // Active status of the card (0: inactive, 1: active)
    char name[32];      // Name associated with the card
    uint32_t timestamp; // Timestamp of the last access
} rfid_card_t;

// Initialization
esp_err_t rfid_manager_init(void);
esp_err_t rfid_manager_load_defaults(void);

// Card Management
esp_err_t rfid_manager_add_card(uint32_t card_id, const char *name);
esp_err_t rfid_manager_remove_card(uint32_t card_id);
esp_err_t rfid_manager_check_card(uint32_t card_id);

// Database Operations
uint16_t rfid_manager_get_card_count(void);
esp_err_t rfid_manager_list_cards(rfid_card_t *cards, uint16_t max_cards);
esp_err_t rfid_manager_save_to_file(void);
esp_err_t rfid_manager_load_from_file(void);

// Utility Functions
esp_err_t rfid_manager_format_database(void);
esp_err_t rfid_manager_reset_to_defaults(void);
bool rfid_manager_is_database_valid(void);
esp_err_t rfid_manager_get_card_list_json(char *buffer, size_t buffer_len);

#endif // RFID_MANAGER_H
