/**
 * Project: PiSensorBLE (v 0.1)
 *
 * Copyright (c) 2023 Henry Cardona <henry@cardona.se>
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
 *
 */

// Header Inclusions and Definitions:

#include "app_error.h"
#include "log_driver.h"
#include "led_driver.h"
#include "nrf_delay.h"

#define PWM_INSTANCE 0 // Define a PWM instance.

static const nrfx_pwm_t m_pwm0 = NRFX_PWM_INSTANCE(PWM_INSTANCE);
static void test_leds_directly(void);

// Configuration and Initialization:

nrfx_pwm_config_t const pwm0_config = {
    .output_pins = {
        RED_PIN,
        GREEN_PIN,
        BLUE_PIN,
        NRFX_PWM_PIN_NOT_USED
    },
    .irq_priority = APP_IRQ_PRIORITY_LOWEST,
    .base_clock   = NRF_PWM_CLK_1MHz,
    .count_mode   = NRF_PWM_MODE_UP,
    .top_value    = 255,
    .load_mode    = NRF_PWM_LOAD_INDIVIDUAL,
    .step_mode    = NRF_PWM_STEP_AUTO
};

//Setting the RGB Color:
static nrf_pwm_values_individual_t seq_values;
static nrf_pwm_sequence_t const seq = {
    .values.p_individual = &seq_values,
    .length              = NRF_PWM_VALUES_LENGTH(seq_values),
    .repeats             = 0,
    .end_delay           = 0
};

/**
 * @brief Function for initializing leds.
 *
 */
void leds_init(bool debug)
{
    ret_code_t err_code;

    err_code = nrfx_pwm_init(&m_pwm0, &pwm0_config, pwm_handler);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("LEDs initialized.");
    initial_feedback(debug);
}
void set_rgb_color(uint8_t red, uint8_t green, uint8_t blue) {
    seq_values.channel_0 = blue;
    seq_values.channel_1 = red;
    seq_values.channel_2 = green;
    
    nrfx_pwm_simple_playback(&m_pwm0, &seq, 1, NRFX_PWM_FLAG_LOOP);
    leds_off_directly();
}

void pwm_handler(nrfx_pwm_evt_type_t event_type) {
    // For now, do nothing in the handler.
}

/**
 * @brief Provides visual feedback during system initialization.
 * 
 */
void initial_feedback(bool debug) // 3
{
    NRF_LOG_INFO("SYSTEM_INITIALIZATION_0000");
    if(debug) {
        test_leds_directly();
    }
}
/**
 * @brief Provides visual feedback when sensor values are between golden_reference 
 *        and top_reference.
 */
void outer_reaction(void) // 1
{
    NRF_LOG_INFO("ZONE_OUTER_TRIGGERED_0001");
}
/**
 * @brief Provides visual feedback when sensor values are in a range between
 *        golden_reference and low_reference indicating an inner proximity.
 */
void inner_reaction(void) // 2
{
    NRF_LOG_INFO("ZONE_INNER_TRIGGERED_0003");
}
/**
 * @brief Provides visual feedback when sensor values are in a range between
 *        golden_reference and low_reference indicating a touch approach.
 */
void touch_reaction(void) // 5
{
    NRF_LOG_INFO("ZONE_TOUCH_TRIGGERED_0007");
}
void trigger_feedback(sensor_feedback_t new_status) {

    switch(new_status) {
        case SENSOR_STATUS_SYST_INITZ_0000:
            initial_feedback(false);
            break;
        case SENSOR_STATUS_ZONE_OUTER_0001:
            outer_reaction();
            break;
        case SENSOR_STATUS_ZONE_INNER_0003:
            inner_reaction();
            break;
        case SENSOR_STATUS_ZONE_TOUCH_0007:
            touch_reaction();
            break;
        default:
            return;
    }
}
static void test_leds_directly(void) {
    // Configure LED pins as output
    nrf_gpio_cfg_output(RED_PIN);
    nrf_gpio_cfg_output(GREEN_PIN);
    nrf_gpio_cfg_output(BLUE_PIN);

    // Turn on the Red LED
    nrf_gpio_pin_clear(RED_PIN);
    nrf_delay_ms(1000); // Wait for 1 second
    nrf_gpio_pin_set(RED_PIN);

    // Turn on the Green LED
    nrf_gpio_pin_clear(GREEN_PIN);
    nrf_delay_ms(1000); // Wait for 1 second
    nrf_gpio_pin_set(GREEN_PIN);

    // Turn on the Blue LED
    nrf_gpio_pin_clear(BLUE_PIN);
    nrf_delay_ms(1000); // Wait for 1 second
    nrf_gpio_pin_set(BLUE_PIN);
}
void pwm_stop_and_disconnect(void) {
    // Stop the PWM playback
    nrfx_pwm_stop(&m_pwm0, true);  // True means the function will wait until the playback is actually stopped.

    // Disconnect the LED pins from the PWM module and configure them as GPIO outputs.
    nrf_gpio_cfg_output(RED_PIN);
    nrf_gpio_cfg_output(GREEN_PIN);
    nrf_gpio_cfg_output(BLUE_PIN);
}

void leds_off_directly(void) {
    nrf_gpio_pin_clear(RED_PIN);
    nrf_gpio_pin_clear(GREEN_PIN);
    nrf_gpio_pin_clear(BLUE_PIN);
}