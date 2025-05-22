#ifndef SPIFFS_STORAGE_H
#define SPIFFS_STORAGE_H

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

void spiffs_storage_init(void);
void spiffs_storage_test(void);
void spiffs_storage_deinit(void);

#endif // SPIFFS_STORAGE_H