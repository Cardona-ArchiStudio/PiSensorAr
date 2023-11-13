/**
 * @file main.c
 * @brief The main application (main.c) acts as the orchestrator.  
 * 
 * Serves as the command center, initializing components and managing the 
 * operation loop.
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
#include "sensor_driver.h"
#include "log_driver.h"
#include "uart_driver.h"
#include "sadc_driver.h"

#include "app_error.h"
#include "nrf_drv_timer.h"
#include "nrf_delay.h"

#define HIGH_INTENSITY 255
#define LOW_INTENSITY 0

// Sets up a timer for regular stability assessments.
const nrf_drv_timer_t STABILITY_TIMER = NRF_DRV_TIMER_INSTANCE(0);

/*
 * Prototypes for internal functions:
 */ 
void sensor_feedback(sensor_data_t* sensor_data);
void set_rgb_intensity(uint16_t red, uint16_t green, uint16_t blue);
static uint16_t map_intensity(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max);
uint8_t calculate_checksum(uint8_t* data, uint8_t length);
static void timer_event_handler(nrf_timer_event_t event_type, void* p_context);
static void timer_setup(void);

/**
 * @brief Calculate checksum for a given set of data.
 *
 * Computes a checksum by summing all the bytes in the data array. This checksum
 * is used for error-checking in communications.
 *
 * @param data Pointer to the data array.
 * @param length Length of the data array.
 * @return uint8_t The calculated checksum value.
 */
uint8_t calculate_checksum(uint8_t* data, uint8_t length) {
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < length; ++i) {
        checksum += data[i];
    }
    return checksum;
}

/**
 * @brief Set the intensity of RGB LEDs and send the data via UART.
 *
 * Constructs a message with the specified RGB intensities and transmits it over UART.
 * The message format includes a start byte, RGB values, checksum, and stop byte.
 *
 * @param red Intensity of the red LED component.
 * @param green Intensity of the green LED component.
 * @param blue Intensity of the blue LED component.
 */
void set_rgb_intensity(uint16_t red, uint16_t green, uint16_t blue) {
    uint8_t message[6];
    message[0] = START_BYTE;
    message[1] = (uint8_t)(red & 0xFF); // assuming 8-bit color intensity
    message[2] = (uint8_t)(green & 0xFF);
    message[3] = (uint8_t)(blue & 0xFF);
    message[4] = calculate_checksum(&message[1], 3); // checksum of RGB values
    message[5] = STOP_BYTE;
    // TOSO: Currently we are only sensing data using the UART to Arduino
    uart_send(message, sizeof(message));
}
/**
 * @brief Feedback handler for new sensor readings.
 *
 * Processes new sensor data and adjusts RGB LED intensity based on sensor values.
 * This function is called as a callback from the sensor driver.
 *
 * @param sensor_data Pointer to the latest sensor data structure.
 */
void sensor_feedback(sensor_data_t* sensor_data)
{
    uint16_t sensor_value = (uint16_t)sensor_data->sensor_reading;
    
    uint16_t red_intensity = LOW_INTENSITY;
    uint16_t green_intensity = LOW_INTENSITY;
    uint16_t blue_intensity = LOW_INTENSITY;

    // If within STABILITY_THRESHOLD of golden_reference, keep all intensities at 0.
    if (abs(sensor_value - sensor_data->golden_reference) <= STABILITY_THRESHOLD)
    {
        set_rgb_intensity(LOW_INTENSITY, LOW_INTENSITY, LOW_INTENSITY);
        return;
    } 

    if (sensor_value < sensor_data->golden_reference - STABILITY_THRESHOLD)
    {
        // As the sensor_value decreases below the golden_reference, the blue intensity increases.
        blue_intensity = map_intensity(sensor_value,
                                       sensor_data->low_reference, 
                                       sensor_data->golden_reference - STABILITY_THRESHOLD,
                                       HIGH_INTENSITY, LOW_INTENSITY);
    }
    else if (sensor_value > sensor_data->golden_reference + (STABILITY_THRESHOLD *2)) // TODO: For stability *2 for now
    {
        // As the sensor_value increases over the golden_reference, the green intensity increases.
        green_intensity = map_intensity(sensor_value,
                                        sensor_data->golden_reference + STABILITY_THRESHOLD, 
                                        sensor_data->top_reference,
                                        LOW_INTENSITY, HIGH_INTENSITY);
    }

    set_rgb_intensity(red_intensity, green_intensity, blue_intensity);
}

/**
 * @brief Map a sensor value to a specified intensity range.
 *
 * Linearly maps a sensor reading from one range to another range, typically used
 * for mapping sensor readings to LED intensities.
 *
 * @param x Sensor reading to map.
 * @param in_min Minimum value of the sensor's range.
 * @param in_max Maximum value of the sensor's range.
 * @param out_min Minimum value of the intensity range.
 * @param out_max Maximum value of the intensity range.
 * @return uint16_t Mapped intensity value.
 */
static uint16_t map_intensity(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
    
    // Ensure values are within the expected range
    if(x < in_min) return out_min;
    if(x > in_max) return out_max;

    // Map the value linearly
    //return (uint8_t)(((x - in_min) * (out_max - out_min) / (in_max - in_min)) + out_min);
    return (uint8_t)(x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/**
 * @brief Timer event handler.
 *
 * Handles timer events. When the timer event occurs, it triggers the sensor stability
 * check function.
 *
 * @param event_type Type of the timer event.
 * @param p_context Context for the timer event (unused).
 */
static void timer_event_handler(nrf_timer_event_t event_type, void* p_context)
{
    switch (event_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
            // Perform sensor's stability check.
            sensor_stability_check();
            break;

        default:
            // Do nothing.
            break;
    }
}

/**
 * @brief Setup the timer for sensor stability checks.
 *
 * Initializes and configures a timer to periodically trigger sensor stability checks.
 * The timer generates events at a defined interval.
 */
static void timer_setup(void)
{
    uint32_t err_code;

    // Configure and initialize the timer
    nrf_drv_timer_config_t timer_cfg = NRF_DRV_TIMER_DEFAULT_CONFIG;
    err_code = nrf_drv_timer_init(&STABILITY_TIMER, &timer_cfg, timer_event_handler);
    APP_ERROR_CHECK(err_code);

    // Convert the desired time interval to timer ticks
    uint32_t time_ms = STABILITY_DURATION; // Time(in miliseconds) between consecutive compare events.
    uint32_t time_ticks = nrf_drv_timer_ms_to_ticks(&STABILITY_TIMER, time_ms);

    // Configure the timer to generate a compare event after the interval has passed
    nrf_drv_timer_extended_compare(
         &STABILITY_TIMER, 
         NRF_TIMER_CC_CHANNEL0, 
         time_ticks, 
         NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, 
         true);

    // Start the timer
    nrf_drv_timer_enable(&STABILITY_TIMER);
}

/**
 * Main application entry point.
 * Initializes various modules and enters the main loop, processing sensor data.
 */
int main(void)
{
    // Initialize the logging module
    log_init();
    
    // Initialize UART for communication
    uart_init();
    
    // Initialize the sensor with a callback function for processing sensor data
    sensor_init(sensor_feedback);
    
    // Initialize the SAADC module
    sadc_init();

    // Calculate the capture-compare value for SAADC sampling based on desired frequency
    uint32_t adc_cc_value = 16000000 / SAADC_SAMPLE_FREQUENCY;
    if(adc_cc_value < 80 || adc_cc_value > 2047)
    {
        NRF_LOG_ERROR("Sample rate frequency outside legal range.");
        APP_ERROR_CHECK(false);
    }
    // Starts the SAADC module  
    sadc_start(adc_cc_value);

    // Setup and start a timer for regular sensor stability checks
    timer_setup();
    
    // Main loop
    while (1)
    {
        // Check if new sensor data is ready and process it
        if ( get_data_ready_flag() ) {
            sensor_process( get_current_buffer() );
            set_data_ready_flag(false);
            nrf_delay_ms(SENSOR_READ_DELAY);  // Delay to manage loop frequency
        }
        
        // Process log messages
        NRF_LOG_PROCESS();
        while(NRF_LOG_PROCESS() != NRF_SUCCESS);
        
        // Wait for event
        __WFE();
    }  
}