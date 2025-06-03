#ifndef TEST_RFID_MANAGER_H
#define TEST_RFID_MANAGER_H

#include "rfid_manager.h"
#include "spiffs_storage.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"

void test_rfid_manager_add_card(void);
void test_rfid_manager_remove_card(void);
void test_rfid_manager_get_card(void);
void test_rfid_manager_check_card(void);
void test_rfid_manager_list_cards(void);
void test_rfid_manager_get_card_count(void);
void test_rfid_manager_format_database(void);
void test_rfid_manager_load_defaults(void);
void test_rfid_manager(void);

#endif // TEST_RFID_MANAGER_H