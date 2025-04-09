
#include "flash_manager.h"
#include "boards.h"
#include "fds.h"
#include "nrf_soc.h"
#include "sdk_config.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#define CONFIG_FILE     (0x8010)
#define CONFIG_REC_KEY  (0x7010)

static configuration_t m_configuration =
{
    .encryption_key = {'N', 'O', 'R', 'D', 'I', 'C', ' ',
                            'S', 'E', 'M', 'I', 'C', 'O', 'N', 'D', 'U', 'C', 'T', 'O', 'R',
                            'A', 'E', 'S', '&', 'M', 'A', 'C', ' ', 'T', 'E', 'S', 'T'},    
    .device_name = "MEGO",
};

static fds_record_t const m_fds_record =
{
    .file_id           = CONFIG_FILE,
    .key               = CONFIG_REC_KEY,
    .data.p_data       = &m_configuration,
    /* The length of a record is always expressed in 4-byte units (words). */
    .data.length_words = (sizeof(m_configuration) + 3) / sizeof(uint32_t),
};

ret_code_t flash_mgr_flash_mgr_init()
{
    ret_code_t rc = NRF_SUCCESS;

    //read the configuration.
    //if it does not exist, create a default one in flash.

    fds_record_desc_t desc = {0};
    fds_find_token_t  tok  = {0};

    rc = fds_record_find(CONFIG_FILE, CONFIG_REC_KEY, &desc, &tok);

    if (rc == NRF_SUCCESS)
    {
        /* A config file is in flash. Let's update it. */
        fds_flash_record_t config = {0};

        /* Open the record and read its contents. */
        rc = fds_record_open(&desc, &config);
        APP_ERROR_CHECK(rc);

        /* Copy the configuration from flash into m_dummy_cfg. */
        memcpy(&m_configuration, config.p_data, sizeof(configuration_t));

        NRF_LOG_INFO("flash_mgr_flash_mgr_init, Config file found, device name: %s", m_configuration.device_name);

        /* Close the record when done reading. */
        rc = fds_record_close(&desc);
        APP_ERROR_CHECK(rc);
    }        
    else
    {
        /* System config not found; write a new one. */
        NRF_LOG_INFO("flash_mgr_flash_mgr_init, could not find configuratoin record in flash. Writing config file.");

        rc = fds_record_write(&desc, &m_fds_record);
        if ((rc != NRF_SUCCESS) && (rc == FDS_ERR_NO_SPACE_IN_FLASH))
        {
            NRF_LOG_INFO("flash_mgr_flash_mgr_init, No space in flash.");
        }
        else
        {
            APP_ERROR_CHECK(rc);
        }
    }

    return rc;
}

ret_code_t flash_mgr_write_record(uint32_t fid,
                                  uint32_t key,
                                  void const * p_data,
                                  uint32_t len)
{    
    fds_record_t const rec =
    {
        .file_id           = fid,
        .key               = key,
        .data.p_data       = p_data,
        .data.length_words = (len + 3) / sizeof(uint32_t)
    };

    ret_code_t rc = fds_record_write(NULL, &rec);
    if (rc != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("flash_mgr_write_record, fds_record_write failed. error: 0x%x.", rc);
    }
    return rc;
}

ret_code_t flash_mgr_read_record()
{
    ret_code_t err_code = NRF_SUCCESS;
    return err_code;
}

ret_code_t flash_mgr_reset()
{
    ret_code_t err_code = NRF_SUCCESS;
    return err_code;
}

ret_code_t flash_mgr_set_device_name(char * device_name)
{
    ret_code_t err_code = NRF_SUCCESS;

    if(strlen(device_name) > sizeof(m_configuration.device_name) - 1)
    {
        NRF_LOG_WARNING("flash_mgr_set_device_name, device name is too long. max length is %d", sizeof(m_configuration.device_name) - 1);
    }

    strcpy(m_configuration.device_name, device_name);

    //flash_mgr_save();
        
    return err_code;
}

ret_code_t flash_mgr_set_encryption_key(uint8_t * encryption_key, int len)
{
    ret_code_t err_code = NRF_SUCCESS;

    if(len != sizeof(m_configuration.encryption_key))
    {
        NRF_LOG_ERROR("flash_mgr_set_encryption_key, invalid size. valid size is %d", sizeof(m_configuration.encryption_key));
        return NRF_ERROR_INVALID_LENGTH;
    }

    memcpy(m_configuration.encryption_key, encryption_key, len);
    
    return err_code;
}

const char * flash_mgr_get_device_name()
{
    return (const char *)m_configuration.device_name;
}

const uint8_t * flash_mgr_get_encryption_key()
{
    return (const uint8_t *)m_configuration.encryption_key;
/*
    if(len != sizeof(m_configuration.encryption_key))
    {
        NRF_LOG_ERROR("flash_mgr_get_encryption_key, invalid size. valid size is %d", sizeof(m_configuration.encryption_key));
        return NRF_ERROR_INVALID_LENGTH;
    }

    memcpy(encryption_key, m_configuration.encryption_key, len);

    return;
    */
}

ret_code_t flash_mgr_save()
{
    NRF_LOG_DEBUG("flash_mgr_save");

    fds_record_desc_t desc = {0};
    fds_find_token_t  tok  = {0};
    ret_code_t rc;

    rc = fds_record_find(CONFIG_FILE, CONFIG_REC_KEY, &desc, &tok);
    
    if (rc == NRF_SUCCESS)
    {
        rc = fds_record_update(&desc, &m_fds_record);
    }
    else
    {
        
    }

    return rc;
}