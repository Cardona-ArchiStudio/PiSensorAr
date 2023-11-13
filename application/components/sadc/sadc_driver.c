/**
 * @file sadc_driver.c
 * @brief SAADC driver for handling Analog-to-Digital convertions.
 * 
 * This module provides functions to initialize SAADC, handle the specifics of 
 * the Successive Approximation Analog-to-Digital Converter (SAADC), interfacing 
 * directly with the hardware to manage analog sensor inputs.
 * 
 * @author Henry Cardon <henry@cardona.se>
 * @date Created on: 2023-10-12
 *
 * Copyright (c) 2017-2023 Cardona Architecture Studio <cardona-archistudio.com>
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * License: MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "sadc_driver.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "app_error.h"

static nrf_saadc_value_t samples[SAADC_BUF_COUNT][SAADC_BUF_SIZE];
static nrfx_saadc_channel_t channel_config = NRFX_SAADC_DEFAULT_CHANNEL_SE(SADC_SENSOR_CHANNEL, 0);

/* 
 * Global variables used for managing SAADC state and data.
 */ 
static volatile bool data_ready_flag = false;  // Flag indicating if new data is ready.
static nrf_saadc_value_t* current_buffer = NULL;  // Pointer to the current SAADC buffer.

/* 
 * Prototypes for internal functions.
 */ 
static void sadc_event_handler(nrfx_saadc_evt_t const * p_event);
static uint32_t next_free_buf_index(void);

/**
 * @brief Initialize the SAADC module.
 * 
 * Sets up the SAADC interface with specific hardware settings. This function must
 * be called before any other SAADC operations.
 *
 * @return ret_code_t Returns NRF_SUCCESS if initialization is successful,
 *                    otherwise returns an error code indicating the type of failure.
 */
ret_code_t sadc_init(void) {
    ret_code_t err_code;

    // Initialize SAADC
    err_code = nrfx_saadc_init(NRFX_SAADC_CONFIG_IRQ_PRIORITY);
    APP_ERROR_CHECK(err_code);
 
    // Configure SAADC channel
    err_code = nrfx_saadc_channels_config(&channel_config, 1);
    APP_ERROR_CHECK(err_code);

    if (err_code != NRFX_SUCCESS) {
        NRF_LOG_ERROR("SADC initialization failed: %d", err_code);
        return err_code;
    }
    return NRF_SUCCESS;
}

/**
 * @brief Start the SAADC sampling.
 * 
 * Configures and starts the SAADC sampling with advanced mode settings.
 * This function sets up double buffering to manage high sampling frequencies.
 *
 * @param cc_value The capture-compare value for the SAADC internal timer.
 * @return ret_code_t Returns NRF_SUCCESS if the start operation is successful,
 *                    otherwise returns an error code indicating the type of failure.
 */
ret_code_t sadc_start(uint32_t cc_value)
{
    ret_code_t err_code;

    // Configure advanced SAADC settings
    nrfx_saadc_adv_config_t saadc_adv_config = NRFX_SAADC_DEFAULT_ADV_CONFIG;
    saadc_adv_config.internal_timer_cc = cc_value;
    saadc_adv_config.start_on_end = true;

    // Set SAADC to advanced mode
    err_code = nrfx_saadc_advanced_mode_set((1<<0), NRF_SAADC_RESOLUTION_10BIT,
                                            &saadc_adv_config,
                                            sadc_event_handler);
    APP_ERROR_CHECK(err_code);
                                            
    // Configure double buffering
    err_code = nrfx_saadc_buffer_set(&samples[next_free_buf_index()][0], SAADC_BUF_SIZE);
    APP_ERROR_CHECK(err_code);

    err_code = nrfx_saadc_buffer_set(&samples[next_free_buf_index()][0], SAADC_BUF_SIZE);
    APP_ERROR_CHECK(err_code);

    // Start SAADC sampling
    err_code = nrfx_saadc_mode_trigger();
    APP_ERROR_CHECK(err_code);

    if (err_code != NRFX_SUCCESS) {
        NRF_LOG_ERROR("SADC start failed: %d", err_code);
        return err_code;
    }
    return NRF_SUCCESS;
}

/**
 * @brief SAADC Event Handler with Internal Error Handling.
 * 
 * Handles various events from the SAADC, such as data completion and buffer requests.
 *
 * @param p_event Pointer to the SAADC event structure containing event details.
 */
static void sadc_event_handler(nrfx_saadc_evt_t const * p_event)
{
    ret_code_t err_code;

    switch (p_event->type)
    {
        case NRFX_SAADC_EVT_DONE:
            // Data acquisition completed; update buffer and flag
            set_current_buffer(p_event->data.done.p_buffer);
            set_data_ready_flag(true);
            break;

        case NRFX_SAADC_EVT_BUF_REQ:
            // Request for new buffer; set up next available buffer
            err_code = nrfx_saadc_buffer_set(&samples[next_free_buf_index()][0], SAADC_BUF_SIZE);
            if (err_code != NRF_SUCCESS) {
                NRF_LOG_ERROR("Error in NRFX_SAADC_EVT_BUF_REQ: %d", err_code);
            }
            break;

        default:
            // Log unexpected event types as internal errors
            NRF_LOG_ERROR("Unexpected SAADC event: NRF_ERROR_INTERNAL");
            break;
    }
}

/**
 * @brief Provides the index of the next available SAADC buffer.
 * 
 * @return uint32_t Index of the next available buffer.
 */
static uint32_t next_free_buf_index(void)
{
    static uint32_t buffer_index = -1;
    buffer_index = (buffer_index + 1) % SAADC_BUF_COUNT;
    return buffer_index;
}

/**
 * @brief Set the data ready flag.
 *
 * Controls the state of a flag indicating whether new SAADC data is ready to be processed.
 *
 * @param state The new state of the data ready flag.
 */
void set_data_ready_flag(bool state) {
    data_ready_flag = state;
}

/**
 * @brief Get the current state of the data ready flag.
 *
 * Retrieves the current state of the flag indicating if new SAADC data is ready.
 *
 * @return bool The current state of the data ready flag.
 */
bool get_data_ready_flag(void) {
    return data_ready_flag;
}

/**
 * @brief Set the current buffer for SAADC data.
 *
 * Specifies the buffer to be used for storing the next set of SAADC data.
 *
 * @param buffer Pointer to the SAADC data buffer.
 */
void set_current_buffer(nrf_saadc_value_t* buffer) {
    current_buffer = buffer;
}

/**
 * @brief Get the current buffer used for SAADC data.
 *
 * Retrieves the pointer to the current buffer being used for SAADC data.
 *
 * @return nrf_saadc_value_t* Pointer to the current SAADC data buffer.
 */
nrf_saadc_value_t* get_current_buffer(void) {
    return current_buffer;
}
