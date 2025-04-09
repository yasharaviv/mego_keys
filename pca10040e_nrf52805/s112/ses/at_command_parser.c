#include "at_command_parser.h"
#include "flash_manager.h"

#include "nrf_ble_gatt.h"
#include "nrf_sdh_ble.h"
#include "ble_gap.h"
#include "version.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrf_delay.h"


#define DEVICE_NAME_MAX_LEN 20
#define CRYPT_KEY_LEN 32
#define PARAM_LENGTH 10

ret_code_t at_command_parse(char * cmd, int len)
{
    NRF_LOG_INFO("at_command_parse, command: %s", cmd);
    if (strncmp(cmd, "AT+NAME=", 8) == 0) 
    {
        // Extract the new name
        char new_name[DEVICE_NAME_MAX_LEN] = {0};
        strncpy(new_name, cmd + 8, sizeof(new_name) - 1);

        NRF_LOG_INFO("at_command_parse, command: AT+NAME, param: %s", new_name);

        flash_mgr_set_device_name(new_name);

        ble_gap_conn_sec_mode_t sec_mode;
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

        sd_ble_gap_device_name_set(&sec_mode, (const uint8_t*)new_name, strlen(new_name));
        
        printf("OK\r\n");
        //nrf_delay_ms(1000);
        
    } 
    else if (strncmp(cmd, "AT+NAME?", 8) == 0) 
    {
        NRF_LOG_INFO("at_command_parse, command: AT+NAME?");

        // Retrieve the current device name
        uint8_t dev_name[DEVICE_NAME_MAX_LEN];
        uint16_t name_len = sizeof(dev_name);
        
        if (sd_ble_gap_device_name_get(dev_name, &name_len) == NRF_SUCCESS) {
            dev_name[name_len] = '\0';  // Ensure null termination
            printf("+NAME: %s\r\n", dev_name);
            printf("OK\r\n");
        } else {
            printf("ERROR\r\n");
        }
    }
    else if (strncmp(cmd, "AT+CRYPTKEY=", 12) == 0) 
    {
        // Extract the new name
        char key[CRYPT_KEY_LEN] = {0};
        strncpy(key, cmd + 12, sizeof(key) - 1);

        NRF_LOG_INFO("at_command_parse, command: AT+CRYPTKEY, param: %s", key);


        flash_mgr_set_encryption_key(key, CRYPT_KEY_LEN);
        
        printf("OK\r\n");
    }    
    else if (strncmp(cmd, "AT+RESET", 8) == 0) 
    {
        NRF_LOG_INFO("at_command_parse, command: AT+RESET");        
        
        printf("OK\r\n");

        nrf_delay_ms(100);

        NVIC_SystemReset();
    }
    else if (strncmp(cmd, "AT+VERSION?", 11) == 0) 
    {
        NRF_LOG_INFO("at_command_parse, command: AT+VERSION?");
        
        printf("+VERSION: %s\r\n", FW_VERSION);
        printf("OK\r\n");           
    }
    else if (strncmp(cmd, "AT+ADINTERVAL=", 14) == 0) 
    {
        // Extract the new name
        char param[PARAM_LENGTH] = {0};
        strncpy(param, cmd + 14, sizeof(param) - 1);

        NRF_LOG_INFO("at_command_parse, command: AT+ADINTERVAL, param: %s", param);


        //TODO - implement

        printf("OK\r\n");
    } 
    else if (strncmp(cmd, "AT+ADINTERVAL?", 14) == 0) 
    {
        NRF_LOG_INFO("at_command_parse, command: AT+ADINTERVAL?");
        
        //TODO - get the real interval value.
        int interval = 40;

        char result[100] = {0};
        snprintf(result, sizeof(result), "AT+ADINTERVAL:%04d\r\n", interval);
        
        printf(result);
        printf("OK\r\n");           
    }
    else if (strncmp(cmd, "AT+DISCON=", 10) == 0) 
    {
        // Extract the new name
        char param[PARAM_LENGTH] = {0};
        strncpy(param, cmd + 10, sizeof(param) - 1);

        NRF_LOG_INFO("at_command_parse, command: AT+DISCON, param: %s", param);

        if(strcmp(param, "1") == 0)
        {
            //TODO - disconnect from device.
        }

        printf("OK\r\n");
    } 
    else if (strncmp(cmd, "AT+SAVE=", 8) == 0) 
    {
        // Extract the new name
        char param[PARAM_LENGTH] = {0};
        strncpy(param, cmd + 8, sizeof(param) - 1);

        NRF_LOG_INFO("at_command_parse, command: AT+SAVE, param: %s", param);

        if(strcmp(param, "1") == 0)
        {
            NRF_LOG_INFO("at_command_parse, command: AT+SAVE, saving");

            ret_code_t rc = flash_mgr_save();
            if(rc != NRF_SUCCESS)
            {
                NRF_LOG_INFO("at_command_parse, command: AT+SAVE, failed");
                printf("ERROR\r\n");
            }
            else
            {
                NRF_LOG_INFO("at_command_parse, command: AT+SAVE, succeeded.");
                printf("OK\r\n");

                nrf_delay_ms(100);

                NVIC_SystemReset();
            }
            return;
        }

        printf("OK\r\n");
    } 
    else if (strncmp(cmd, "AT+STOP=", 8) == 0) 
    {
        // Extract the new name
        char param[PARAM_LENGTH] = {0};
        strncpy(param, cmd + 8, sizeof(param) - 1);

        NRF_LOG_INFO("at_command_parse, command: AT+STOP, param: %s", param);

        if(strcmp(param, "1") == 0)
        {
            //TODO - 1: adv stop,scan stop,UART alive
        }
        else if(strcmp(param, "2") == 0)
        {
            //TODO - 2:adv stop ,scan stop, UART stop
        }
        else if(strcmp(param, "3") == 0)
        {
            //TODO - 3:adv on ,scan stop, UART stop
        }
        else
        {
            NRF_LOG_INFO("at_command_parse, unknown parameter: %s", param)
        }

        printf("OK\r\n");
    } 
    else if (strncmp(cmd, "AT+UART?", 8) == 0) 
    {
        //AT+UART:<param>,<param2>,<param3>
        //Baud rate, Stop bit,Parity
        
        NRF_LOG_INFO("at_command_parse, command: AT+UART?");
        int baudRate = 115200;
        int stopBit = 1;
        int parity = 0;

        char result[100] = {0};
        snprintf(result, sizeof(result), "AT+UART:%d,%s,%s\r\n", baudRate, stopBit, parity);

        printf(result);
        printf("OK\r\n");
    } 
    else if (strncmp(cmd, "AT+UART=", 8) == 0) 
    {
        // Extract the new name
        char param[PARAM_LENGTH] = {0};
        strncpy(param, cmd + 8, sizeof(param) - 1);

        NRF_LOG_INFO("at_command_parse, command: AT+STOP, param: %s", param);
        
        //TODO - 3:adv on ,scan stop, UART stop
        

        printf("OK\r\n");
    } 
    else if (strncmp(cmd, "AT+DEFAULT", 10) == 0) 
    {
        NRF_LOG_INFO("at_command_parse, command: AT+DEFAULT");
        
        printf("OK\r\n");
    } 
    else if (strncmp(cmd, "AT+DATAMODE=", 12) == 0) 
    {
        NRF_LOG_INFO("at_command_parse, command: AT+DATAMODE");

        //TODO - implement
        
        printf("OK\r\n");
    } 
    else 
    {
        NRF_LOG_ERROR("at_command_parse, unknown command: %s", cmd);

        printf("ERROR\r\n");
    }
    return NRF_SUCCESS;
}