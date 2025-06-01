#ifndef RFID_MANAGER_H
#define RFID_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

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
bool rfid_manager_is_database_valid(void);

#endif // RFID_MANAGER_H