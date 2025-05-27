#ifndef TEST_SPIFFS_STORAGE_H
#define TEST_SPIFFS_STORAGE_H

#include "spiffs_storage.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>

#define TEST_FILE_NAME "/spiffs/test_file.txt"
#define TEST_FILE_CONTENT "This is a test file for SPIFFS storage."

void test_spiffs_storage(void);

#endif // TEST_SPIFFS_STORAGE_H
