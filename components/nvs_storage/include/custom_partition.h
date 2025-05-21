#ifndef CUSTOM_PARTITION_H
#define CUSTOM_PARTITION_H

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

void nvs_custom_partition_init(void);
bool nvs_custom_partition_set_params(const char *key, const char *value);
bool nvs_custom_partition_get_params(const char *key, char *value, size_t value_len);
void nvs_custom_partition_test(void);

#endif // CUSTOM_PARTITION_H
