#ifndef FLASH_MGR_H
#define FLASH_MGR_H
#include "app_error.h"
#include "nrf.h"


/* A dummy structure to save in flash. */
typedef struct
{    
    char        device_name[16];
    uint8_t     encryption_key[32];    
} configuration_t;


ret_code_t flash_mgr_init();
ret_code_t flash_mgr_write_record(uint32_t fid,
                                  uint32_t key,
                                  void const * p_data,
                                  uint32_t len);

ret_code_t flash_mgr_read_record();
ret_code_t flash_mgr_reset();
ret_code_t flash_mgr_save();

ret_code_t flash_mgr_set_device_name(char * device_name);
ret_code_t flash_mgr_set_encryption_key(uint8_t * encryption_key, int len);

const char * flash_mgr_get_device_name();
const uint8_t * flash_mgr_get_encryption_key();


#endif //FLASH_MGR_H
  