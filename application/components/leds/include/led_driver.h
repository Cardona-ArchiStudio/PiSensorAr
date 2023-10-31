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
#ifndef LED_DRIVER_H__
#define LED_DRIVER_H__

#include <stdio.h>
#include <string.h>
#include "nrf_gpio.h"
#include "nrfx_pwm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RED_PIN   NRF_GPIO_PIN_MAP(0, 28)
#define GREEN_PIN NRF_GPIO_PIN_MAP(0, 4)
#define BLUE_PIN  NRF_GPIO_PIN_MAP(0, 3)

// System Feedback Structure 
typedef enum {
    SENSOR_STATUS_SYST_INITZ_0000, // system initialization/No alarm 
    SENSOR_STATUS_ZONE_OUTER_0001, // indicating an outer proximity
    SENSOR_STATUS_ZONE_INNER_0003, // indicating an inner proximity
    SENSOR_STATUS_ZONE_TOUCH_0007, // indicating a touch approach
} sensor_feedback_t;

// Updated function prototype
void pwm_handler(nrfx_pwm_evt_type_t event_type);
void trigger_feedback(sensor_feedback_t new_status);
void set_rgb_color(uint8_t red, uint8_t green, uint8_t blue);

void leds_init(bool debug);
void initial_feedback(bool debug);
void pwm_stop_and_disconnect(void);
void leds_off_directly(void);
void outer_reaction(void);
void inner_reaction(void);
void touch_reaction(void);


#ifdef __cplusplus
}
#endif

#endif //LED_DRIVER_H__
