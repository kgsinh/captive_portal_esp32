idf_component_register(SRCS "rfid_manager.c"
                    INCLUDE_DIRS "include"
                    REQUIRES spiffs_storage log freertos)
