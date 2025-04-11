/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup ble_sdk_uart_over_ble_main main.c
 * @{
 * @ingroup  ble_sdk_app_nus_eval
 * @brief    UART over BLE application main file.
 *
 * This file contains the source code for a sample application that uses the Nordic UART service.
 * This application uses the @ref srvlib_conn_params module.
 */


#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "app_timer.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "app_util_platform.h"
#include "bsp_btn_ble.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_drv_gpiote.h"
#include "app_button.h"

// Add ECDH related includes
#include "nrf_crypto_ecc.h"
#include "nrf_crypto_ecdh.h"

#if defined (UART_PRESENT)
#include "nrf_uart.h"
#endif
#if defined (UARTE_PRESENT)
#include "nrf_uarte.h"
#endif

#include "nrf_crypto.h"
#include "nrf_crypto_error.h"

#include "fds.h"
#include "nrf_fstorage.h"

#include "mbedtls/base64.h"

// Function declarations
ret_code_t encrypt_data(char *data, int len);
ret_code_t decrypt_data(const uint8_t *data, int len);
void flash_mgr_flash_mgr_init(void);
const uint8_t *flash_mgr_get_encryption_key(void);
const char *flash_mgr_get_device_name(void);

#define APP_BLE_CONN_CFG_TAG            1                                           /**< A tag identifying the SoftDevice BLE configuration. */

#define DEVICE_NAME                     "MEGO"                               /**< Name of device. Will be included in the advertising data. */
#define NUS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN                  /**< UUID type for the Nordic UART Service (vendor specific). */

// Key exchange message types
#define MSG_TYPE_KEY_EXCHANGE_REQ       0x01  // Reduced from 2 bytes to 1 byte
#define MSG_TYPE_KEY_EXCHANGE_RESP      0x02  // Reduced from 2 bytes to 1 byte
#define MSG_TYPE_DATA                   0x03  // Reduced from 2 bytes to 1 byte

#define KEY_EXCHANGE_MSG_HEADER_SIZE    1     // Reduced from 2 bytes to 1 byte
#define PUBLIC_KEY_SIZE                 48    // secp192r1 public key size
#define MAX_MESSAGE_SIZE                (KEY_EXCHANGE_MSG_HEADER_SIZE + PUBLIC_KEY_SIZE)

#define APP_BLE_OBSERVER_PRIO           3                                           /**< Application's BLE observer priority. You shouldn't need to modify this value. */

#define APP_ADV_INTERVAL                64                                          /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */

#define APP_ADV_DURATION                18000                                       /**< The advertising duration (180 seconds) in units of 10 milliseconds. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(30, UNIT_1_25_MS)             /**< Minimum acceptable connection interval (30 ms), Connection interval uses 1.25 ms units. */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(50, UNIT_1_25_MS)             /**< Maximum acceptable connection interval (50 ms), Connection interval uses 1.25 ms units. */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(2000, UNIT_10_MS)             /**< Connection supervisory timeout (2 seconds), Supervision Timeout uses 10 ms units. */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define UART_TX_BUF_SIZE                256                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                256                                         /**< UART RX buffer size. */

#define GPIO_INPUT_PIN  4   // Pin P0.4
#define BUTTON_DETECTION_DELAY APP_TIMER_TICKS(50) // Debounce delay                                        


BLE_NUS_DEF(m_nus, NRF_SDH_BLE_TOTAL_LINK_COUNT);                                   /**< BLE NUS service instance. */
NRF_BLE_GATT_DEF(m_gatt);                                                           /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                                             /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);                                                 /**< Advertising module instance. */

static uint16_t   m_conn_handle          = BLE_CONN_HANDLE_INVALID;                 /**< Handle of the current connection. */
static uint16_t   m_ble_nus_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;            /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */
static ble_uuid_t m_adv_uuids[]          =                                          /**< Universally unique service identifier. */
{
    {BLE_UUID_NUS_SERVICE, NUS_SERVICE_UUID_TYPE}
};

/////////////////////////////////////////////////
//encrypt globals
/////////////////////////////////////////////////
#define NRF_CRYPTO_AES_MAX_DATA_SIZE    (256)
#define BASE64_MAX_DATA_SIZE (256)

// Optimize ECDH related structures and variables
static nrf_crypto_ecc_private_key_t m_private_key;
static nrf_crypto_ecc_public_key_t m_public_key;
static uint8_t m_raw_public_key[PUBLIC_KEY_SIZE];
static uint8_t m_shared_secret[24];   // secp192r1 shared secret size
static bool m_ecdh_initialized = false;

static uint8_t m_key[32] = {'A', 'O', 'R', 'D', 'I', 'C', ' ',
                            'S', 'E', 'M', 'I', 'C', 'O', 'N', 'D', 'U', 'C', 'T', 'O', 'R',
                            'A', 'E', 'S', '&', 'M', 'A', 'C', ' ', 'T', 'E', 'S', 'T'};

static nrf_crypto_key_size_id_t m_key_size = NRF_CRYPTO_KEY_SIZE_256;
static uint8_t iv[16];
static nrf_crypto_aes_info_t const * p_cbc_info;
static nrf_crypto_aes_context_t cbc_encr_ctx;   
static nrf_crypto_aes_context_t cbc_decr_ctx;   

static char encrypted_data[NRF_CRYPTO_AES_MAX_DATA_SIZE];  
static int encrypted_data_len = 0;
  
static char decrypted_data[NRF_CRYPTO_AES_MAX_DATA_SIZE];    
static int decrypted_data_len = 0;

static uint8_t decoded_data[BASE64_MAX_DATA_SIZE];  
static int decoded_data_len = 0;

static char encoded_data[BASE64_MAX_DATA_SIZE];  
static int encoded_data_len = 0;

/////////////////////////////////////////////////
//FDS globals
/////////////////////////////////////////////////
/* Array to map FDS events to strings. */
static char const * fds_evt_str[] =
{
    "FDS_EVT_INIT",
    "FDS_EVT_WRITE",
    "FDS_EVT_UPDATE",
    "FDS_EVT_DEL_RECORD",
    "FDS_EVT_DEL_FILE",
    "FDS_EVT_GC",
};

/* Keep track of the progress of a delete_all operation. */
static struct
{
    bool delete_next;   //!< Delete next record.
    bool pending;       //!< Waiting for an fds FDS_EVT_DEL_RECORD event, to delete the next record.
} m_delete_all;

/* Flag to check fds initialization. */
static bool volatile m_fds_initialized;


/////////////////////////////////////////////////

void base64_encode_example() {
    const char *input = "Hello, nRF!";
    size_t input_len = strlen(input);
    unsigned char output[64];
    size_t output_len;

    // Encode
    mbedtls_base64_encode(output, sizeof(output), &output_len, (const unsigned char *)input, input_len);
}

void base64_decode_example() {
    const char *encoded = "JURSFDQ1MjVjN2FiMWM3OGNiMgDLVaU=";
    unsigned char output[64];
    size_t output_len;

    // Decode
    mbedtls_base64_decode(output, sizeof(output), &output_len, (const unsigned char *)encoded, strlen(encoded));
    output[output_len] = '\0';  // Ensure null-termination
}


//Initialize all encryption data.
void crypt_init()
{
    ret_code_t  ret_val;
    memset(iv, 0, sizeof(iv));    
    
    p_cbc_info = &g_nrf_crypto_aes_cbc_256_info;    
    ret_val = nrf_crypto_aes_init(&cbc_encr_ctx,
                                  p_cbc_info,
                                  NRF_CRYPTO_ENCRYPT);
    APP_ERROR_CHECK(ret_val);

    ret_val = nrf_crypto_aes_key_set(&cbc_encr_ctx, m_key);
    APP_ERROR_CHECK(ret_val);

    ret_val = nrf_crypto_aes_iv_set(&cbc_encr_ctx, iv);
    APP_ERROR_CHECK(ret_val);
}

// Initialize ECDH and generate key pair
ret_code_t ecdh_init(void)
{
    ret_code_t err_code;
    size_t size;

    if (m_ecdh_initialized)
    {
        return NRF_SUCCESS;
    }

    // Generate new key pair using secp192r1 curve
    err_code = nrf_crypto_ecc_key_pair_generate(NULL,
                                               &g_nrf_crypto_ecc_secp192r1_curve_info,
                                               &m_private_key,
                                               &m_public_key);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Convert public key to raw format for transmission
    size = sizeof(m_raw_public_key);
    err_code = nrf_crypto_ecc_public_key_to_raw(&m_public_key,
                                               m_raw_public_key,
                                               &size);
    if (err_code != NRF_SUCCESS)
    {
        nrf_crypto_ecc_private_key_free(&m_private_key);
        nrf_crypto_ecc_public_key_free(&m_public_key);
        return err_code;
    }

    // Free the public key structure as we only need the raw format
    err_code = nrf_crypto_ecc_public_key_free(&m_public_key);
    if (err_code != NRF_SUCCESS)
    {
        nrf_crypto_ecc_private_key_free(&m_private_key);
        return err_code;
    }

    m_ecdh_initialized = true;
    return NRF_SUCCESS;
}

// Compute shared secret from received public key
ret_code_t ecdh_compute_shared_secret(const uint8_t *p_peer_public_key, size_t key_size)
{
    ret_code_t err_code;
    nrf_crypto_ecc_public_key_t peer_public_key;
    size_t shared_secret_size;

    if (!m_ecdh_initialized)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    if (p_peer_public_key == NULL)
    {
        return NRF_ERROR_NULL;
    }

    if (key_size != sizeof(m_raw_public_key))
    {
        return NRF_ERROR_INVALID_LENGTH;
    }

    // Convert received public key to internal format
    err_code = nrf_crypto_ecc_public_key_from_raw(&g_nrf_crypto_ecc_secp192r1_curve_info,
                                                 &peer_public_key,
                                                 p_peer_public_key,
                                                 key_size);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Compute shared secret
    shared_secret_size = sizeof(m_shared_secret);
    err_code = nrf_crypto_ecdh_compute(NULL,
                                      &m_private_key,
                                      &peer_public_key,
                                      m_shared_secret,
                                      &shared_secret_size);
    
    // Clean up regardless of success
    nrf_crypto_ecc_public_key_free(&peer_public_key);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
}

// Clean up ECDH resources
void ecdh_cleanup(void)
{
    if (m_ecdh_initialized)
    {
        nrf_crypto_ecc_private_key_free(&m_private_key);
        m_ecdh_initialized = false;
    }
}

// Handle key exchange protocol
static ret_code_t handle_key_exchange(const uint8_t * p_data, uint16_t length, uint16_t conn_handle)
{
    ret_code_t err_code;
    static bool key_exchanged = false;
    
    if (p_data == NULL)
    {
        return NRF_ERROR_NULL;
    }

    if (key_exchanged)
    {
        return NRF_SUCCESS;
    }

    if (length < KEY_EXCHANGE_MSG_HEADER_SIZE)
    {
        return NRF_ERROR_INVALID_LENGTH;
    }

    uint8_t msg_type = p_data[0];  // Single byte message type

    switch (msg_type)
    {
        case MSG_TYPE_KEY_EXCHANGE_REQ:
        {
            if (length != MAX_MESSAGE_SIZE)
            {
                return NRF_ERROR_INVALID_LENGTH;
            }

            // Compute shared secret from received public key
            err_code = ecdh_compute_shared_secret(p_data + KEY_EXCHANGE_MSG_HEADER_SIZE, 
                                                PUBLIC_KEY_SIZE);
            if (err_code != NRF_SUCCESS)
            {
                return err_code;
            }

            // Send our public key back
            uint8_t response[MAX_MESSAGE_SIZE];
            response[0] = MSG_TYPE_KEY_EXCHANGE_RESP;
            memcpy(response + KEY_EXCHANGE_MSG_HEADER_SIZE, 
                  m_raw_public_key, 
                  sizeof(m_raw_public_key));
            
            uint16_t response_length = sizeof(response);
            do
            {
                err_code = ble_nus_data_send(&m_nus, response, &response_length, conn_handle);
                if ((err_code != NRF_ERROR_INVALID_STATE) &&
                    (err_code != NRF_ERROR_RESOURCES) &&
                    (err_code != NRF_ERROR_NOT_FOUND))
                {
                    return err_code;
                }
            } while (err_code == NRF_ERROR_RESOURCES);

            key_exchanged = true;
            return NRF_SUCCESS;
        }

        case MSG_TYPE_KEY_EXCHANGE_RESP:
        {
            if (length != MAX_MESSAGE_SIZE)
            {
                return NRF_ERROR_INVALID_LENGTH;
            }

            // Compute shared secret from received public key
            err_code = ecdh_compute_shared_secret(p_data + KEY_EXCHANGE_MSG_HEADER_SIZE, 
                                                PUBLIC_KEY_SIZE);
            if (err_code != NRF_SUCCESS)
            {
                return err_code;
            }

            key_exchanged = true;
            return NRF_SUCCESS;
        }

        default:
            return NRF_ERROR_INVALID_DATA;
    }
}

/**@brief Function for handling the data from the Nordic UART Service.
 *
 * @details This function will process the data received from the Nordic UART BLE Service and send
 *          it to the UART module.
 *
 * @param[in] p_evt       Nordic UART Service event.
 */
static void nus_data_handler(ble_nus_evt_t * p_evt)
{
    if (p_evt->type == BLE_NUS_EVT_RX_DATA)
    {
        uint32_t err_code;
        static bool key_exchanged = false;

        if (!key_exchanged)
        {
            err_code = handle_key_exchange(p_evt->params.rx_data.p_data, 
                                         p_evt->params.rx_data.length,
                                         m_conn_handle);
            if (err_code == NRF_SUCCESS)
            {
                key_exchanged = true;
                return;
            }
            else if (err_code != NRF_ERROR_INVALID_DATA)
            {
                return;
            }
        }

        err_code = decrypt_data(p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);
        APP_ERROR_CHECK(err_code);

        for(;decrypted_data_len > 0 && decrypted_data[decrypted_data_len-1] < ' '; decrypted_data_len--);

        int result = mbedtls_base64_decode(decoded_data, sizeof(decoded_data), &decoded_data_len, decrypted_data, decrypted_data_len);

        for (uint32_t i = 0; i < decoded_data_len; i++)
        {
            do
            {                
                err_code = app_uart_put(decoded_data[i]);
                if ((err_code != NRF_SUCCESS) && (err_code != NRF_ERROR_BUSY))
                {
                    APP_ERROR_CHECK(err_code);
                }
            } while (err_code == NRF_ERROR_BUSY);
        }
    }
}


/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    uint32_t           err_code;
    ble_nus_init_t     nus_init;
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    // Initialize NUS.
    memset(&nus_init, 0, sizeof(nus_init));

    nus_init.data_handler = nus_data_handler;

    err_code = ble_nus_init(&m_nus, &nus_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling an event from the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module
 *          which are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply setting
 *       the disconnect_on_fail config parameter, but instead we use the event handler
 *       mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    uint32_t err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    ret_code_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            break;
        case BLE_ADV_EVT_IDLE:
            err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
            APP_ERROR_CHECK(err_code);
            break;
        default:
            break;
    }
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    uint32_t err_code;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
            printf("+CONNECTED\r\n");
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            printf("+DISCONNECTED\r\n");
            break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_2MBPS,
                .tx_phys = BLE_GAP_PHY_2MBPS,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for the SoftDevice initialization.
 *
 * @details This function initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}


/**@brief Function for handling events from the GATT library. */
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    if ((m_conn_handle == p_evt->conn_handle) && (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {
        m_ble_nus_max_data_len = p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH;
    }
}


/**@brief Function for initializing the GATT library. */
void gatt_init(void)
{
    ret_code_t err_code;

    err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, NRF_SDH_BLE_GATT_MAX_MTU_SIZE);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated by button press.
 */
void bsp_event_handler(bsp_event_t event)
{
    uint32_t err_code;
    switch (event)
    {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break;

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BSP_EVENT_WHITELIST_OFF:
            if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            {
                err_code = ble_advertising_restart_without_whitelist(&m_advertising);
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }
            }
            break;

        default:
            break;
    }
}


/**@brief   Function for handling app_uart events.
 *
 * @details This function will receive a single character from the app_uart module and append it to
 *          a string. The string will be be sent over BLE when the last character received was a
 *          'new line' '\n' (hex 0x0A) or if the string has reached the maximum data length.
 */
void uart_event_handle(app_uart_evt_t * p_event)
{
    static uint8_t data_array[BLE_NUS_MAX_DATA_LEN];
    static int index = 0;
    static bool key_exchanged = false;
    uint32_t       err_code;

    switch (p_event->evt_type)
    {
        case APP_UART_DATA_READY:
            UNUSED_VARIABLE(app_uart_get(&data_array[index]));
            index++;

            if (((index > 3) && 
                 (data_array[index - 3] == 0xA5) && 
                 (data_array[index - 2] == 0xA6) && 
                 (data_array[index - 1] == 0xA7)) ||
                (index >= m_ble_nus_max_data_len))
            {
                if (index > 3)
                {
                    mbedtls_base64_encode(encoded_data, sizeof(encoded_data), &encoded_data_len, (const unsigned char *)data_array, index-3);

                    if (!key_exchanged)
                    {
                        uint8_t key_exchange[66];
                        key_exchange[0] = 0x00;
                        key_exchange[1] = MSG_TYPE_KEY_EXCHANGE_REQ;
                        memcpy(key_exchange + 2, m_raw_public_key, sizeof(m_raw_public_key));
                        
                        uint16_t length = sizeof(key_exchange);
                        do
                        {
                            err_code = ble_nus_data_send(&m_nus, key_exchange, &length, m_conn_handle);
                            if ((err_code != NRF_ERROR_INVALID_STATE) &&
                                (err_code != NRF_ERROR_RESOURCES) &&
                                (err_code != NRF_ERROR_NOT_FOUND))
                            {
                                APP_ERROR_CHECK(err_code);
                            }
                        } while (err_code == NRF_ERROR_RESOURCES);
                        
                        key_exchanged = true;
                    }
                    else
                    {
                        err_code = encrypt_data(encoded_data, encoded_data_len); 
                        APP_ERROR_CHECK(err_code);

                        do
                        {
                            uint16_t length = (uint16_t)encrypted_data_len;
                            err_code = ble_nus_data_send(&m_nus, encrypted_data, &length, m_conn_handle);
                            if ((err_code != NRF_ERROR_INVALID_STATE) &&
                                (err_code != NRF_ERROR_RESOURCES) &&
                                (err_code != NRF_ERROR_NOT_FOUND))
                            {
                                APP_ERROR_CHECK(err_code);
                            }
                        } while (err_code == NRF_ERROR_RESOURCES);
                    }
                }

                memset(data_array, 0, sizeof(data_array));
                index = 0;
            }
            break;

        case APP_UART_COMMUNICATION_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_communication);
            break;

        case APP_UART_FIFO_ERROR:
            APP_ERROR_HANDLER(p_event->data.error_code);
            break;

        default:
            break;
    }
}


/**@brief  Function for initializing the UART module.
 */
static void uart_init(void)
{
    uint32_t                     err_code;
    app_uart_comm_params_t const comm_params =
    {
        .rx_pin_no    = 12,
        .tx_pin_no    = 16,
        .rts_pin_no   = RTS_PIN_NUMBER,
        .cts_pin_no   = CTS_PIN_NUMBER,
        .flow_control = APP_UART_FLOW_CONTROL_DISABLED,
        .use_parity   = false,
#if defined (UART_PRESENT)
        .baud_rate    = NRF_UARTE_BAUDRATE_115200
#else
        .baud_rate    = NRF_UARTE_BAUDRATE_115200
#endif
    };

    APP_UART_FIFO_INIT(&comm_params,
                       UART_RX_BUF_SIZE,
                       UART_TX_BUF_SIZE,
                       uart_event_handle,
                       APP_IRQ_PRIORITY_LOWEST,
                       err_code);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    uint32_t               err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type          = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance = false;
    init.advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

    init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_adv_uuids;

    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;
    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}


/**@brief Function for initializing buttons and leds.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 */
static void buttons_leds_init(bool * p_erase_bonds)
{
    bsp_event_t startup_event;

    uint32_t err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}


/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    nrf_pwr_mgmt_run();
}


/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    uint32_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
}

const char *fds_err_str(ret_code_t ret)
{
    /* Array to map FDS return values to strings. */
    static char const * err_str[] =
    {
        "FDS_ERR_OPERATION_TIMEOUT",
        "FDS_ERR_NOT_INITIALIZED",
        "FDS_ERR_UNALIGNED_ADDR",
        "FDS_ERR_INVALID_ARG",
        "FDS_ERR_NULL_ARG",
        "FDS_ERR_NO_OPEN_RECORDS",
        "FDS_ERR_NO_SPACE_IN_FLASH",
        "FDS_ERR_NO_SPACE_IN_QUEUES",
        "FDS_ERR_RECORD_TOO_LARGE",
        "FDS_ERR_NOT_FOUND",
        "FDS_ERR_NO_PAGES",
        "FDS_ERR_USER_LIMIT_REACHED",
        "FDS_ERR_CRC_CHECK_FAILED",
        "FDS_ERR_BUSY",
        "FDS_ERR_INTERNAL",
    };

    return err_str[ret - NRF_ERROR_FDS_ERR_BASE];
}


static void fds_evt_handler(fds_evt_t const * p_evt)
{
    switch (p_evt->id)
    {
        case FDS_EVT_INIT:
            if (p_evt->result == NRF_SUCCESS)
            {
                m_fds_initialized = true;
            }
            break;

        case FDS_EVT_WRITE:
        {
            if (p_evt->result == NRF_SUCCESS)
            {
                // Record written successfully
            }
        } break;

        case FDS_EVT_DEL_RECORD:
        {
            if (p_evt->result == NRF_SUCCESS)
            {
                // Record deleted successfully
            }
            m_delete_all.pending = false;
        } break;

        default:
            break;
    }
}

void flash_storage_init(void) {
    ret_code_t rc = fds_register(fds_evt_handler);
    APP_ERROR_CHECK(rc);
    
    rc = fds_init();
    APP_ERROR_CHECK(rc);
}

/**@brief   Sleep until an event is received. */
static void power_manage(void)
{
#ifdef SOFTDEVICE_PRESENT
    (void) sd_app_evt_wait();
#else
    __WFE();
#endif
}


/**@brief   Wait for fds to initialize. */
static void wait_for_fds_ready(void)
{
    while (!m_fds_initialized)
    {
        power_manage();
    }
}

// GPIO event handler
void gpio_event_handler(uint8_t pin_no, uint8_t button_action) {
    // Remove AT command mode handling
}

// Button configuration
static app_button_cfg_t button_cfg[] = {
    {GPIO_INPUT_PIN, false, NRF_GPIO_PIN_PULLUP, gpio_event_handler}
};

void gpio_init(void) {
    ret_code_t err_code;

    // Initialize the GPIOTE module if not already initialized
    if (!nrf_drv_gpiote_is_init()) {
        err_code = nrf_drv_gpiote_init();
        APP_ERROR_CHECK(err_code);
    }

    // Initialize app_button module
    err_code = app_button_init(button_cfg, ARRAY_SIZE(button_cfg), BUTTON_DETECTION_DELAY);
    APP_ERROR_CHECK(err_code);

    // Enable button detection
    err_code = app_button_enable();
    APP_ERROR_CHECK(err_code);
}

//Encryption method
ret_code_t encrypt_data(char * data, int len) 
{     
    ret_code_t  ret_val;

    if (!m_ecdh_initialized)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    if (data == NULL || len <= 0)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    crypt_init();

    memset(encrypted_data, 0, sizeof(encrypted_data));    

    char data_to_encrypt[NRF_CRYPTO_AES_MAX_DATA_SIZE];
    int data_to_encrypt_len = 0;

    //We set the buffer with 0x4 instead of 0x0 due to the way the decryption in the android app works.
    memset(data_to_encrypt, 4, sizeof(data_to_encrypt));
    
    memcpy(data_to_encrypt, data, len);     
    
    //len should be a multiple of 16. so we take the closest 16 multiple to len.
    data_to_encrypt_len = ((len / 16)  + 1) * 16;  //integer divided by 16 

    encrypted_data_len = sizeof(encrypted_data);

    /* Encrypt using the shared secret as the key */
    ret_val = nrf_crypto_aes_finalize(&cbc_encr_ctx,
                                      (uint8_t *)data_to_encrypt,
                                      data_to_encrypt_len,
                                      (uint8_t *)encrypted_data,
                                      &encrypted_data_len);
     
    return ret_val;
}

//Decryption method
ret_code_t decrypt_data(const uint8_t * data, int len) 
{    
    ret_code_t  ret_val;

    if (!m_ecdh_initialized)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    if (data == NULL || len <= 0)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    if (len % 16 != 0)
    {
        return NRF_ERROR_INVALID_LENGTH;
    }

    crypt_init();
    
    memset(decrypted_data, 0, sizeof(decrypted_data));

    decrypted_data_len = sizeof(decrypted_data);
    
    ret_val = nrf_crypto_aes_crypt(&cbc_decr_ctx,
                                   p_cbc_info,
                                   NRF_CRYPTO_DECRYPT,
                                   m_shared_secret,  // Use shared secret as key
                                   iv,
                                   (uint8_t *)data,
                                   len,
                                   (uint8_t *)decrypted_data,
                                   &decrypted_data_len);
     
    return ret_val;
}

// Function declarations
ret_code_t encrypt_data(char *data, int len);
ret_code_t decrypt_data(const uint8_t *data, int len);
void flash_mgr_flash_mgr_init(void);
const uint8_t *flash_mgr_get_encryption_key(void);
const char *flash_mgr_get_device_name(void);

/**@brief Application main function.
 */
int main(void)
{
    bool erase_bonds;
    ret_code_t ret;

    // Initialize.
    uart_init();
    timers_init();
    gpio_init();
       
    ret = nrf_crypto_init();
    APP_ERROR_CHECK(ret);
   
    power_management_init();

    flash_storage_init();
    flash_mgr_flash_mgr_init();    

    const char *device_name = flash_mgr_get_device_name();
    const uint8_t *key = flash_mgr_get_encryption_key();
    memcpy(m_key, key, sizeof(m_key));
        
    ble_stack_init();
    gap_params_init(device_name);
    gatt_init();
    services_init();
    advertising_init();
    
    conn_params_init();

    // Initialize ECDH
    ret = ecdh_init();
    APP_ERROR_CHECK(ret);
    
    // Start execution.
    printf("OK\r\n");
    
    crypt_init();    

    advertising_start();

    // Enter main loop.
    for (;;)
    {
        idle_state_handle();
    }
}