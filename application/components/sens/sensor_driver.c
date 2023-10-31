#include <math.h>
#include <limits.h>
#include <float.h>
#include <stdio.h>
#include <string.h>

#include "boards.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "app_error.h"

#include "sensor_driver.h"

/*
 * Global variables:
 */ 
static bool debug = true;
// Circular buffer to store sensor readings:
static int sensor_readings[SENS_BUFFER_SIZE] = {0}; 
// Object holding the status of the sensor:
static sensor_status_t sensor_status;
// Callback function to handle sensor reading events:
static sensor_callback_t sensor_callback_ref = NULL;
// Object holding the sensor readings and stability data:
static sensor_data_t sensor_data;
// Object holding the sensor driver context data:
static sensor_context_t sensor_ctx;

/*
 * Prototypes for internal functions:
 */ 
static void calibration_process(nrf_saadc_value_t *samples);
static void operation_process(nrf_saadc_value_t *samples);
static sensor_status_t process_results(nrf_saadc_value_t *samples);
static void sensor_initialization();
static void auto_calibrate(int sensor_value, int average, int min_reading, int max_reading);
static float convert_to_voltage(uint16_t adc_value);


void sensor_init(sensor_callback_t callback) 
{
    // Store the function callback reference
    sensor_callback_ref = callback;
    sensor_initialization();
}
void sensor_process(nrf_saadc_value_t *samples) {

    if (sensor_ctx.is_calibrated) {
        operation_process(samples);
    } else {
        calibration_process(samples);
    }
}
static void calibration_process(nrf_saadc_value_t *samples)
{
    int total = 0;
    uint16_t min_reading = UINT16_MAX;
    uint16_t max_reading = 0;

    NRF_LOG_INFO("Starting initial calibration...");

    for(int i = 0; i < SENS_BUFFER_SIZE; i++)
    {
        // Cache the raw ADC value
        uint16_t sensor_reading = (uint16_t)samples[i];
        // Store the current reading in the buffer at the current buffer index
        sensor_readings[i] = sensor_reading;

        total += sensor_reading;
        min_reading = (sensor_reading < min_reading) ? sensor_reading : min_reading;
        max_reading = (sensor_reading > max_reading) ? sensor_reading : max_reading;       

    }

    int average = total / SENS_BUFFER_SIZE;
    // Sets the sensor's data initial averages values.
    sensor_data.average_reading   = average;
    sensor_data.previous_average  = average; 
    sensor_data.sensor_reading    = average;
    sensor_data.golden_reference  = average;
    
    // Set the reference boundaries with a margin of < 10% > for stability.
    sensor_data.low_reference = min_reading - (INITIAL_MIN_THRESHOLD * average);
    sensor_data.top_reference = max_reading + (INITIAL_MAX_THRESHOLD * average);

    // Move to the next position in the circular buffer
    sensor_ctx.buffer_index = (sensor_ctx.buffer_index + 1) % SENS_BUFFER_SIZE;

    sensor_ctx.current_min_value  = min_reading;
    sensor_ctx.current_max_value  = max_reading;
    sensor_ctx.sensor_voltage     = convert_to_voltage(average);

    log_sensor_data(&sensor_data);
    sensor_ctx.is_calibrated = true;

    NRF_LOG_INFO("The initial calibration is finished!");

}

static void operation_process(nrf_saadc_value_t *samples)
{
    // Process the ADC result
    sensor_status_t status = process_results(samples);

    if(status == SENSOR_ERROR) {
        NRF_LOG_WARNING("Error processing the ADC results.");
    } else if(status == SENSOR_PROCESSED) {
        // No troubles, do nothing!
    } 
}

static sensor_status_t process_results(nrf_saadc_value_t *samples) 
{
    // Check if sensor context has been initialized
    if (!sensor_ctx.is_calibrated) {
        return SENSOR_ERROR;
    }

    // Initialize stats variables
    int total = 0;
    int min_value = UINT16_MAX;
    int max_value = 0;

    // Cache the raw ADC value
    uint16_t sensor_reading = (uint16_t)samples[0];
    // Store the current reading in the buffer at the current buffer index
    sensor_readings[sensor_ctx.buffer_index ] = sensor_reading;

    // Compute buffer statistics: total, min, and max readings
    for (int i = 0; i < SENS_BUFFER_SIZE; i++)
    {
        total += sensor_readings[i];
        if (sensor_readings[i] < min_value)
            min_value = sensor_readings[i];
        
        if (sensor_readings[i] > max_value)
            max_value = sensor_readings[i];
    }

    // Calculate average sensor reading
    int average = total / SENS_BUFFER_SIZE;

    // Update min/max reference values
    if(sensor_reading > sensor_data.top_reference) {
        sensor_data.top_reference = sensor_reading;
    }
    if(sensor_reading < sensor_data.low_reference) {
        sensor_data.low_reference = sensor_reading;
    }
    // Update sensor data
    sensor_data.sensor_reading = sensor_reading;
    sensor_data.average_reading = average;

    sensor_ctx.current_min_value = min_value;
    sensor_ctx.current_max_value = max_value;
    // Move to the next position in the circular buffer
    sensor_ctx.buffer_index = (sensor_ctx.buffer_index + 1) % SENS_BUFFER_SIZE;

    log_sensor_data(&sensor_data);

    // Provide feedback
    if(sensor_callback_ref != NULL) {
        sensor_callback_ref(&sensor_data);
    } else {
        return SENSOR_ERROR;
    }
    return SENSOR_PROCESSED;
}
// Check and update voltage stability status
void sensor_stability_check()
{
    int sensor_value = sensor_data.sensor_reading;
    int current_average = sensor_data.average_reading;
    int previous_average = sensor_data.previous_average;
    int current_min_voltage = sensor_ctx.current_min_value;
    int current_max_voltage = sensor_ctx.current_max_value;

    // Check if the sensor value exceeds a certain threshold.
    if(sensor_value < sensor_data.low_reference) 
    {
        // TODO: Take appropriate action (e.g., turn on an LED, send a BLE notification, etc.)
        sensor_data.is_voltage_stable = false;
        return;

    } else if (sensor_value > sensor_data.top_reference)    
    {
        // TODO: Take appropriate action (e.g., turn on an LED, send a BLE notification, etc.)
        sensor_data.is_voltage_stable = false;
        return;

    } else if (abs(current_average - previous_average) <= STABILITY_THRESHOLD) 
    {
        sensor_data.previous_average = current_average;
        sensor_data.is_voltage_stable = true;
        auto_calibrate(sensor_value, current_average, current_min_voltage, current_max_voltage);
    } 
}
static void auto_calibrate(int sensor_value, int average, int min_reading, int max_reading)
{
    // Update the goldenReference value when the stability conditions are met.
    sensor_data.golden_reference = sensor_value;
    // Reset the reference boundaries with a margin of < 10% > for stability.
    sensor_data.low_reference = min_reading - (INITIAL_MIN_THRESHOLD * average);
    sensor_data.top_reference = max_reading + (INITIAL_MAX_THRESHOLD * average);
}
static void sensor_initialization() {
    
    sensor_data.sensor_reading      = 0;
    sensor_data.previous_average    = 0;
    sensor_data.average_reading     = 0;
    sensor_data.is_voltage_stable   = false;
    sensor_data.top_reference       = 0;
    sensor_data.low_reference       = 0;
    sensor_data.golden_reference    = 0;

    sensor_ctx.is_calibrated        = false;
    sensor_ctx.buffer_index         = 0;
    sensor_ctx.current_min_value    = 0; 
    sensor_ctx.current_max_value    = 0; 
    sensor_ctx.sensor_voltage       = 0.0;
}

sensor_data_t get_sensor_data(void) 
{
    return sensor_data;
}
sensor_context_t get_sensor_context(void) 
{
    return sensor_ctx;
}
sensor_status_t get_sensor_status(void) 
{
    if (!sensor_ctx.is_calibrated) {
        return SENSOR_ERROR;
    } else {
        return SENSOR_OK;
    }  
}
static float convert_to_voltage(uint16_t adc_value) {
    // Vref - reference voltage of ~0.6V.
    float voltage = (adc_value / 1023.0) * 0.6;
    return voltage;
}

/*
 * SENSOR   - Current Sensor Value,
 * GOLDEN   - The stable voltage reference,
 * STABLE   - Is Voltage Stable ?
 * CURAVE   - Current Average value,
 * LOWREF   - Last MIN voltage,
 * TOPREF   - Last Max voltage.
 */
void log_sensor_data(const sensor_data_t* data) 
{
    if(debug) 
    {

        // Static variable to check if the header has been logged
        static bool header_logged = false;

        // If the header hasn't been logged, log it and set the flag
        if (!header_logged) {
            NRF_LOG_INFO("SENSOR, GOLDEN, STABLE, CURAVE, LOWREF, TOPREF");
            header_logged = true;
        }
        NRF_LOG_INFO("%d, %d, %s, %d, %d, %d", 
                      data->sensor_reading,
                      data->golden_reference,
                      data->is_voltage_stable ? "TRUE" : "FALSE",
                      data->average_reading,
                      data->low_reference,
                      data->top_reference
                    );
 
        //NRF_LOG_INFO("Voltage: " NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(data->sensor_voltage));
        //NRF_LOG_INFO("Is voltage stable? %s", data->is_voltage_stable ? "TRUE" : "FALSE")

        NRF_LOG_FLUSH();
    }
}
/**
 * @brief Function for the Timer initialization.
 * @details Initializes the timer module. This creates and starts application timers.
 */
void timers_init(void)
{
    // Initialize the timer module.
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    // Optionally, if we've defined a timer instance: #define APP_TIMER_DEF(m_my_timer_id);
    // We can create a timer:
    // err_code = app_timer_create(&m_my_timer_id, APP_TIMER_MODE_REPEATED, timer_timeout_handler);
    // APP_ERROR_CHECK(err_code);
}