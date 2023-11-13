#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "nrf_drv_uart.h"
#include "nrf_gpio.h"

/**
 * @brief Error codes for UART operations.
 */
typedef enum {
    UART_SUCCESS = 0,        // Successful operation
    UART_ERROR,              // General error
    UART_ERROR_BUSY,         // UART is busy
    UART_ERROR_TIMEOUT,      // Operation timed out
    UART_ERROR_PARITY,       // Parity error detected
    UART_ERROR_BREAK,        // Break condition detected
    UART_ERROR_FRAMING,      // Framing error detected
    UART_ERROR_OVERRUN       // Buffer overrun error
} uart_error_t;

/**
 * @brief Return type for UART operations.
 */
typedef uart_error_t uart_ret_code_t;

// UART configuration constants
#define UART_MAX_BUFFER_SIZE 256   // Maximum buffer size for transmission/reception

// Special byte definitions for enhanced communication protocol
#define START_BYTE 0xAA            // Start byte for frame
#define STOP_BYTE  0x55            // Stop byte for frame

// Definitions for Arduino's checksum and framing errors handling
#define ACK 0x06   // Acknowledgment byte if the checksum is correct
#define NAK 0x15   // Negative acknowledgment byte if the checksum is incorrect
#define REJ 0x21   // Rejection byte for framing error

// UART PIN definitions
#define TX_PIN_NUMBER NRF_GPIO_PIN_MAP(0,6)   // TX Pin
#define RX_PIN_NUMBER NRF_GPIO_PIN_MAP(0,8)   // RX Pin
#define CTS_PIN_NUMBER NRF_GPIO_PIN_MAP(0,7)  // CTS Pin
#define RTS_PIN_NUMBER NRF_GPIO_PIN_MAP(0,5)  // RTS Pin

/**
 * @brief Set the state of the 'can_send_data' flag.
 *
 * Controls the state of a flag that indicates whether the UART is ready to transmit data.
 *
 * @param state The new state of the 'can_send_data' flag.
 */
void set_can_send_data(bool state);

/**
 * @brief Get the current state of the 'can_send_data' flag.
 *
 * Retrieves the current state of the flag that indicates if the UART is ready to transmit data.
 *
 * @return bool The current state of the 'can_send_data' flag.
 */
bool get_can_send_data(void);

/**
 * @brief Initialize the UART module.
 *
 * Sets up the UART interface with specified hardware settings including baud rate,
 * hardware flow control, and pin configuration. This function must be called
 * before performing any other UART operations.
 *
 * @return uart_ret_code_t Returns UART_SUCCESS if initialization is successful,
 *                         otherwise returns an error code indicating the type of failure.
 */
ret_code_t uart_init(void);

/**
 * @brief Send data over UART.
 *
 * Transmits the specified data over UART. The transmission is only performed if
 * the 'can_send_data' flag is true, indicating that the device is ready to send data.
 *
 * @param data Pointer to the data to be sent.
 * @param length Length of the data to be sent.
 * @return uart_ret_code_t Returns UART_SUCCESS if transmission is successful,
 *                         otherwise returns an error code indicating the type of failure.
 */
ret_code_t uart_send(uint8_t *data, uint8_t length);

/**
 * @brief Receive data from UART.
 *
 * Receives data over UART and stores it in the specified buffer. The function
 * checks if the buffer length exceeds the maximum buffer size.
 *
 * @param data Pointer to the buffer where received data will be stored.
 * @param length Length of the buffer.
 * @return uart_ret_code_t Returns UART_SUCCESS if reception is successful,
 *                         otherwise returns an error code indicating the type of failure.
 */
ret_code_t uart_receive(uint8_t *data, uint8_t length);

#ifdef __cplusplus
}
#endif

#endif // UART_DRIVER_H
