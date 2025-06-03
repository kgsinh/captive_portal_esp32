idf_component_register(SRCS "test_rfid_manager.c"
                    INCLUDE_DIRS "include"
                    REQUIRES spiffs_storage rfid_manager unity)
