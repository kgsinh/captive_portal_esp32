idf_component_register(SRCS 
                        "main.c" 
                    INCLUDE_DIRS "include"
                    REQUIRES app_local_server app_time_sync app_wifi 
                    nvs_flash esp_wifi nvs_storage spiffs_storage rfid_manager)
