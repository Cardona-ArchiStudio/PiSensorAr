#ifndef SADC_DRIVER_H
#define SADC_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "nrfx_saadc.h"

/**
 * @brief Error codes for SAADC operations.
 */
typedef enum {
    SADC_SUCCESS = 0,        // Successful operation
    SADC_ERROR,              // General error
    SADC_ERROR_BUSY,         // SAADC is busy
    SADC_ERROR_TIMEOUT,      // Operation timed out
    SADC_ERROR_INVALID_PARAM,// Invalid parameter
    SADC_ERROR_CALIBRATION   // Error during calibration
} sadc_error_t;

/**
 * @brief Return type for SAADC operations.
 */
typedef sadc_error_t sadc_ret_code_t;

// ADC channel definition for sensor readings
#define SADC_SENSOR_CHANNEL NRF_SAADC_INPUT_AIN0

// SAADC configuration constants
#define SADC_MAX_BUFFER_SIZE 256   // Maximum buffer size for ADC readings
#define SAADC_BUF_COUNT        2   // Number of buffers for SAADC
#define SAADC_BUF_SIZE         100 // Size of each buffer for SAADC
#define SAADC_SAMPLE_FREQUENCY 8000 // Sampling frequency in Hz

/**
 * @brief Set the data ready flag.
 *
 * Controls the state of a flag indicating whether new SAADC data is ready to be processed.
 *
 * @param state The new state of the data ready flag.
 */
void set_data_ready_flag(bool state);

/**
 * @brief Get the current state of the data ready flag.
 *
 * Retrieves the current state of the flag indicating if new SAADC data is ready.
 *
 * @return bool The current state of the data ready flag.
 */
bool get_data_ready_flag(void);

/**
 * @brief Set the current buffer for SAADC data.
 *
 * Specifies the buffer to be used for storing the next set of SAADC data.
 *
 * @param buffer Pointer to the SAADC data buffer.
 */
void set_current_buffer(nrf_saadc_value_t* buffer);

/**
 * @brief Get the current buffer used for SAADC data.
 *
 * Retrieves the pointer to the current buffer being used for SAADC data.
 *
 * @return nrf_saadc_value_t* Pointer to the current SAADC data buffer.
 */
nrf_saadc_value_t* get_current_buffer(void);

/**
 * @brief Initialize the SAADC module.
 *
 * Prepares the SAADC module for operation, setting up necessary hardware configurations.
 *
 * @return ret_code_t Returns NRF_SUCCESS if initialization is successful,
 *                    otherwise returns an error code indicating the type of failure.
 */
ret_code_t sadc_init(void);

/**
 * @brief Start the SAADC sampling.
 *
 * Begins the SAADC sampling process, using the specified capture-compare value for timing.
 *
 * @param cc_value The capture-compare value for timing SAADC sampling.
 * @return ret_code_t Returns NRF_SUCCESS if the start operation is successful,
 *                    otherwise returns an error code indicating the type of failure.
 */
ret_code_t sadc_start(uint32_t cc_value);

#ifdef __cplusplus
}
#endif

#endif // SADC_DRIVER_H
