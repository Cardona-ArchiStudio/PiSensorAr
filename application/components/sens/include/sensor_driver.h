#ifndef SENSOR_DRIVER_H
#define SENSOR_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "nrfx_saadc.h"
#include "app_timer.h"

// Size of the circular buffer for sensor readings
#define SENS_BUFFER_SIZE 50  

// Threshold for stability checks
#define STABILITY_THRESHOLD 10 

// Constants for sensor operation
#define INITIAL_MAX_THRESHOLD 0.1 // Threshold for maximum reading
#define INITIAL_MIN_THRESHOLD 0.1 // Threshold for minimum reading
#define SAADC_SAMPLES_IN_BUFFER 2  // Number of samples per SAADC buffer

// Timing constants
#define STABILITY_DURATION 10000  // Duration for checking stability in milliseconds
#define SENSOR_READ_DELAY 100     // Delay between sensor readings in milliseconds
#define FINE_STRUCTURE 390        // Static value for the Golden Reference

/**
 * @brief Structure to hold sensor data and status.
 */
typedef struct {
    int sensor_reading;      // Current sensor value
    int golden_reference;    // Stable reference voltage for comparison
    int average_reading;     // Current calculated average sensor value
    int top_reference;       // Maximum sensor reading observed
    int low_reference;       // Minimum sensor reading observed
    int previous_average;    // Previous calculated average sensor value
    bool is_voltage_stable;  // Indicates whether the sensor voltage is stable
} sensor_data_t;

/**
 * @brief Enumerations for sensor status.
 */
typedef enum {
    SENSOR_OK = 0,
    SENSOR_ERROR,
    SENSOR_INITIALIZATION,
    SENSOR_CALIBRATION,
    SENSOR_PROCESSED
} sensor_status_t;

/**
 * @brief Structure for managing sensor context.
 */
typedef struct {
    bool is_calibrated;      // Indicates if the sensor is calibrated
    int buffer_index;        // Index for the circular buffer
    int current_min_value;   // Current minimum sensor value
    int current_max_value;   // Current maximum sensor value
    float sensor_voltage;    // Sensor voltage
} sensor_context_t;

/**
 * @brief Callback function type for sensor events.
 */
typedef void (*sensor_callback_t)(sensor_data_t* sensor_data);

/**
 * @brief Initialize the sensor with a specified callback.
 *
 * Sets up the sensor driver and initializes the sensor context. The callback is used
 * to handle sensor data once it's processed.
 *
 * @param callback The callback function to be called with the sensor data.
 */
void sensor_init(sensor_callback_t callback);

/**
 * @brief Process the sensor data.
 *
 * Handles the sensor data processing. This includes calibration if the sensor is not
 * yet calibrated, and normal data processing otherwise.
 *
 * @param samples Pointer to the array of SAADC samples.
 */
void sensor_process(nrf_saadc_value_t *samples);

/**
 * @brief Get the current status of the sensor.
 *
 * Retrieves the current status of the sensor, indicating if it's OK, in error,
 * in calibration, or in other states.
 *
 * @return sensor_status_t The current status of the sensor.
 */
sensor_status_t get_sensor_status(void);

/**
 * @brief Get the current sensor data.
 *
 * Retrieves the current sensor data, including sensor readings, stability status,
 * and other relevant metrics.
 *
 * @return sensor_data_t The current sensor data.
 */
sensor_data_t get_sensor_data(void);

/**
 * @brief Get the current sensor context.
 *
 * Retrieves the current context of the sensor, including calibration status and
 * min/max values.
 *
 * @return sensor_context_t The current sensor context.
 */
sensor_context_t get_sensor_context(void) ;

/**
 * @brief Log the sensor data.
 *
 * Outputs the current sensor data to the log, which is useful for monitoring and
 * debugging purposes.
 *
 * @param data Pointer to the sensor data to be logged.
 */
extern void log_sensor_data(const sensor_data_t* data);

/**
 * @brief Check and update the stability status of the sensor.
 *
 * Evaluates the stability of the sensor readings and updates the status
 * accordingly. This can involve auto-calibration if the readings are stable.
 */
extern void sensor_stability_check(void);


#ifdef __cplusplus
}
#endif

#endif // SENSOR_DRIVER_H