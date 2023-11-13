/**
 * @file uart_driver.c
 * @brief UART driver for handling serial communication
 * 
 * This module provides functions to initialize UART, send and receive data
 * over UART, and handle UART events.
 * 
 * @author Henry Cardon <henry@cardona.se>
 * @date Created on: 2023-11-12
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
#include "uart_driver.h" 

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "app_error.h"
#include "nrf_delay.h"

// UART instance and configuration 
static const nrf_drv_uart_t uart_instance = NRF_DRV_UART_INSTANCE(0);
static nrf_drv_uart_config_t uart_config = NRF_DRV_UART_DEFAULT_CONFIG;

// Global variable to track the communication state
static volatile bool can_send_data = true;
static volatile bool uart_tx_complete = true;

/**
 * Prototypes for internal functions.
 */ 
static void uart_event_handler(nrf_drv_uart_event_t * p_event, void* p_context);

/**
 * @brief Initialize the UART module.
 *
 * Sets up the UART interface with specified hardware settings including baud rate,
 * hardware flow control, and pin configuration. This function must be called
 * before performing any other UART operations.
 *
 * @return ret_code_t Returns NRF_SUCCESS if initialization is successful, 
 *                    otherwise returns an error code indicating the type of failure.
 */
ret_code_t uart_init(void) {
    // Set UART configuration
    uart_config.pseltxd = TX_PIN_NUMBER;    // TX Pin
    uart_config.pselrxd = RX_PIN_NUMBER;    // RX Pin 
    uart_config.pselcts = CTS_PIN_NUMBER;   // CTS Pin
    uart_config.pselrts = RTS_PIN_NUMBER;   // RTS Pin
    uart_config.baudrate = NRF_UART_BAUDRATE_115200; // Baud rate
    uart_config.hwfc = NRF_UART_HWFC_ENABLED;        // Enable hardware flow control
    uart_config.interrupt_priority = APP_IRQ_PRIORITY_HIGH;
    uart_config.parity = NRF_UART_PARITY_EXCLUDED;

    // Initialize UART
    ret_code_t err_code = nrf_drv_uart_init(&uart_instance, &uart_config, uart_event_handler);
    APP_ERROR_CHECK(err_code);

    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("UART initialization failed: %d", err_code);
        // Handle the error (e.g., retry, halt operation, etc.)
    }
    return err_code;
}

/**
 * @brief Send data over UART.
 *
 * Transmits the specified data over UART. The transmission is only performed if
 * the 'can_send_data' flag is true, indicating that the device is ready to send data.
 *
 * @param data Pointer to the data to be sent.
 * @param length Length of the data to be sent.
 * @return ret_code_t Returns NRF_SUCCESS if transmission is successful,
 *                    otherwise returns an error code indicating the type of failure.
 */
ret_code_t uart_send(uint8_t *data, uint8_t length) {
    ret_code_t err_code = NRF_SUCCESS;

    if (length > UART_MAX_BUFFER_SIZE) {
        return NRF_ERROR_INVALID_LENGTH; 
    }
    if (can_send_data) {
        if (uart_tx_complete) {
            uart_tx_complete = false; // Clear flag
            err_code = nrf_drv_uart_tx(&uart_instance, data, length);
            // Handle err_code...
            if (err_code == NRF_ERROR_BUSY) {
                // UART is busy, retry after a delay or skip
                NRF_LOG_ERROR("UART is busy: %d", err_code);
                nrf_delay_ms(10); // Delay before retry
                uart_tx_complete = true; // Set flag to indicate ready for retry
            } else if (err_code != NRF_SUCCESS) {
                NRF_LOG_ERROR("UART send failed: %d", err_code);
            }
        } else {
            // UART is busy, decide to skip or handle accordingly
            NRF_LOG_WARNING("Consider adding more diagnostic logging to understand why the UART becomes busy.");
        }
    }
    return err_code;
}

/**
 * @brief Receive data from UART.
 *
 * Receives data over UART and stores it in the specified buffer. The function
 * checks if the buffer length exceeds the maximum buffer size.
 *
 * @param data Pointer to the buffer where received data will be stored.
 * @param length Length of the buffer.
 * @return ret_code_t Returns NRF_SUCCESS if reception is successful,
 *                    otherwise returns an error code indicating the type of failure.
 */
ret_code_t uart_receive(uint8_t *data, uint8_t length) {
    if (length > UART_MAX_BUFFER_SIZE) {
        return NRF_ERROR_INVALID_LENGTH; 
    }

    ret_code_t err_code = nrf_drv_uart_rx(&uart_instance, data, length); 
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("UART receive failed: %d", err_code);
    }
    return err_code;
}

/**
 * @brief UART event handler.
 *
 * Handles various UART events such as transmission completion, reception of data,
 * and UART errors. This function is called by the UART driver upon the occurrence
 * of each UART event.
 *
 * @param p_event Pointer to the UART event structure.
 * @param p_context Additional context for the event handler (unused).
 */
static void uart_event_handler(nrf_drv_uart_event_t * p_event, void* p_context) {
    switch(p_event->type) {
        case NRF_DRV_UART_EVT_TX_DONE:
            // Transmission complete event handling
            uart_tx_complete = true; // Set flag on transmission complete
            break;

        case NRF_DRV_UART_EVT_RX_DONE:
            // Handle incoming ACK/NAK/REJ signals from the connected device
            uint8_t received_byte = p_event->data.rxtx.p_data[0];
            if (received_byte == ACK) {
                // Handle ACK received
                NRF_LOG_INFO("ACK received: %d", received_byte); 
            } else if (received_byte == NAK || received_byte == REJ) {
                // Handle NAK or REJ received
                NRF_LOG_WARNING("NAK or REJ received: %d", received_byte); 
            }
            break;

        case NRF_DRV_UART_EVT_ERROR:
            // Handle UART errors (framing, overrun, etc.)
            if(p_event->data.error.error_mask & NRF_UART_ERROR_PARITY_MASK) {
                // Parity error handling
            }
            if(p_event->data.error.error_mask & NRF_UART_ERROR_BREAK_MASK) {
                // Break condition handling
            }
            if(p_event->data.error.error_mask & NRF_UART_ERROR_FRAMING_MASK) {
                // Framing error handling
            } else if (p_event->data.error.error_mask & NRF_UART_ERROR_OVERRUN_MASK) {
                // Overrun error handling
            }
            // Clear UART error flags
            nrf_drv_uart_errorsrc_get(&uart_instance);
            break;

        default:
            // Handle other UART events as needed
            break;
    }
}

/**
 * @brief Set the 'can_send_data' flag state.
 *
 * Controls the state of the 'can_send_data' flag, which is used to determine
 * if the UART is ready to transmit data.
 *
 * @param state The new state of the 'can_send_data' flag.
 */
void set_can_send_data(bool state) {
    can_send_data = state;
}

/**
 * @brief Get the current state of the 'can_send_data' flag.
 *
 * Retrieves the current state of the 'can_send_data' flag, which indicates
 * if the UART is ready to transmit data.
 *
 * @return bool The current state of the 'can_send_data' flag.
 */
bool get_can_send_data(void) {
    return can_send_data;
}