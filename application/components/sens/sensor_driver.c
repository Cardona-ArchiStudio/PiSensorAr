/**
 * @file sensor_driver.c
 * @brief Sensor driver responsible for reading and interpreting sensor data.
 * 
 * This module manages the calibration and continuous readings of the sensor. 
 * It assesses the stability of readings and, if necessary, recalibrates to ensure 
 * reading stability.
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
static bool debug = true;  // Flag to enable or disable debug logging
static int sensor_readings[SENS_BUFFER_SIZE] = {0}; // Circular buffer to store sensor readings
static sensor_status_t sensor_status; // Object holding the status of the sensor
static sensor_callback_t sensor_callback_ref = NULL; // Callback function to handle sensor reading events:
static sensor_data_t sensor_data; // Object holding the sensor readings and stability data
static sensor_context_t sensor_ctx; // Object holding the sensor driver context data

/*
 * Prototypes for internal functions:
 */ 
static void calibration_process(nrf_saadc_value_t *samples);
static void operation_process(nrf_saadc_value_t *samples);
static sensor_status_t process_results(nrf_saadc_value_t *samples);
static void sensor_initialization();
static void auto_calibrate(int sensor_value, int average, int min_reading, int max_reading);
static float convert_to_voltage(uint16_t adc_value);

/**
 * @brief Initialize the sensor.
 *
 * Initializes the sensor driver and stores the provided callback function.
 * This function must be called before performing any sensor operations.
 *
 * @param callback The callback function to be called on sensor event.
 */
void sensor_init(sensor_callback_t callback) 
{
    // Store the function callback reference
    sensor_callback_ref = callback;
    // Perform sensor initialization
    sensor_initialization();
}

/**
 * @brief Process sensor data.
 *
 * Processes sensor data based on the current state (calibration or operation).
 * Calls the appropriate processing function for the current state.
 *
 * @param samples Pointer to the buffer containing sensor samples.
 */
void sensor_process(nrf_saadc_value_t *samples) {

    if (sensor_ctx.is_calibrated) {
        // Process sensor data in operational mode
        operation_process(samples);
    } else {
        // Perform sensor calibration
        calibration_process(samples);
    }
}

/**
 * @brief Calibration process for sensor data.
 *
 * Performs initial calibration of the sensor using the provided samples.
 * Calculates the average, minimum, and maximum readings for calibration.
 *
 * @param samples Pointer to the buffer containing sensor samples.
 */
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
    sensor_data.golden_reference  = FINE_STRUCTURE;
    
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

/**
 * @brief Perform operational processing of sensor readings.
 *
 * Processes ADC samples during normal sensor operation. 
 * Calls `process_results` to handle the sensor data which evaluates 
 * the sensor readings and updates the sensor data accordingly.
 *
 * @param samples Pointer to the array of ADC samples to process.
 */
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

/**
 * @brief Process and analyze the sensor results.
 *
 * Analyzes the ADC samples to update sensor readings and status. Checks for
 * calibration status and computes statistics like average, min, and max readings.
 * Returns the status of the sensor based on the analysis.
 *
 * @param samples Pointer to the array of ADC samples to analyze.
 * @return sensor_status_t The status of the sensor after processing the results.
 */
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

/**
 * @brief Check and update voltage stability status.
 *
 * Evaluates if the sensor's voltage is stable based on the current and historical readings.
 * Performs auto-calibration if stability conditions are met.
 */
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
        //auto_calibrate(sensor_value, current_average, current_min_voltage, current_max_voltage);
    } 
}

/**
 * @brief Automatically calibrate the sensor based on current readings.
 *
 * Updates the sensor's reference values based on the current sensor readings
 * to maintain accuracy and stability.
 *
 * @param sensor_value Current sensor value.
 * @param average Current average reading.
 * @param min_reading Minimum reading in the current set.
 * @param max_reading Maximum reading in the current set.
 */
static void auto_calibrate(int sensor_value, int average, int min_reading, int max_reading)
{
    // Update the goldenReference value when the stability conditions are met.
    sensor_data.golden_reference = sensor_value;
    // Reset the reference boundaries with a margin of < 10% > for stability.
    sensor_data.low_reference = min_reading - (INITIAL_MIN_THRESHOLD * average);
    sensor_data.top_reference = max_reading + (INITIAL_MAX_THRESHOLD * average);
}

/**
 * @brief Initializes sensor context and data to default values.
 *
 * Sets up the initial state for the sensor context and data, preparing the sensor
 * for operation.
 */
static void sensor_initialization() {
    
    sensor_data.sensor_reading      = 0;
    sensor_data.previous_average    = 0;
    sensor_data.average_reading     = 0;
    sensor_data.is_voltage_stable   = false;
    sensor_data.top_reference       = 0;
    sensor_data.low_reference       = 0;
    sensor_data.golden_reference    = FINE_STRUCTURE;

    sensor_ctx.is_calibrated        = false;
    sensor_ctx.buffer_index         = 0;
    sensor_ctx.current_min_value    = 0; 
    sensor_ctx.current_max_value    = 0; 
    sensor_ctx.sensor_voltage       = 0.0;
}

/**
 * @brief Get the current sensor data.
 *
 * Retrieves the current sensor data, including sensor readings, stability status,
 * and other relevant metrics.
 *
 * @return sensor_data_t The current sensor data.
 */
sensor_data_t get_sensor_data(void) 
{
    return sensor_data;
}

/**
 * @brief Get the current sensor context.
 *
 * Retrieves the current context of the sensor, including calibration status and
 * min/max values.
 *
 * @return sensor_context_t The current sensor context.
 */
sensor_context_t get_sensor_context(void) 
{
    return sensor_ctx;
}

/**
 * @brief Get the current status of the sensor.
 *
 * Retrieves the current status of the sensor, indicating if it's OK, in error,
 * in calibration, or in other states.
 *
 * @return sensor_status_t The current status of the sensor.
 */
sensor_status_t get_sensor_status(void) 
{
    if (!sensor_ctx.is_calibrated) {
        return SENSOR_ERROR;
    } else {
        return SENSOR_OK;
    }  
}

/**
 * @brief Convert ADC value to voltage.
 *
 * Converts a raw ADC value to its corresponding voltage value based on
 * the reference voltage.
 *
 * @param adc_value The ADC value to convert.
 * @return float The corresponding voltage value.
 */
static float convert_to_voltage(uint16_t adc_value) {
    // Vref - reference voltage of ~0.6V.
    float voltage = (adc_value / 1023.0) * 0.6;
    return voltage;
}
/**
 * @brief Log sensor data.
 *
 * Logs the sensor data for debugging purposes. Only logs if the debug flag is enabled.
 *
 * @param data Pointer to the sensor_data_t structure containing the data to log.
 */
void log_sensor_data(const sensor_data_t* data) 
{
    if(debug) 
    {

        // Static variable to check if the header has been logged
        static bool header_logged = false;

        // If the header hasn't been logged, log it and set the flag
        if (!header_logged) {
           /*
            * SENSOR   - Current Sensor Value,
            * GOLDEN   - The stable voltage reference,
            * STABLE   - Is Voltage Stable ?
            * CURAVE   - Current Average value,
            * LOWREF   - Last MIN voltage,
            * TOPREF   - Last Max voltage.
            */
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