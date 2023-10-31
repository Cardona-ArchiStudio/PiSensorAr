#ifndef SENSOR_DRIVER_H
#define SENSOR_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "nrfx_saadc.h"
#include "app_timer.h"

#define SAADC_BUF_SIZE              100
#define SENS_BUFFER_SIZE            50  // Size of the circular buffer for sensor readings

// Define the threshold for stability checks:
#define STABILITY_THRESHOLD 20 

// Define constants and macros
#define INITIAL_MAX_THRESHOLD 0.1 
#define INITIAL_MIN_THRESHOLD 0.1   
// Define the size of ADC buffer (2 samples per buffer):
#define SAADC_SAMPLES_IN_BUFFER 2
// Define the ADC channel for sensor readings:
#define SENSOR_CHANNEL NRF_SAADC_INPUT_AIN0

// Time duration for checking stability (ms):
#define STABILITY_DURATION 10000
// Define the delay between sensor readings (50ms):
#define SENSOR_READ_DELAY 100

// Structures for configuration customization
typedef struct {
    int sensor_reading; // The current sensor value  
    int golden_reference; // Stable reference voltage for comparison  
    int average_reading; // The current calculated average sensor value 
    int top_reference; // Maximum sensor reading observed
    int low_reference; // Minimum sensor reading observed
    int previous_average; // The previous calculated average sensor value
    bool is_voltage_stable; // Boolean indicating a stable sensor value
} sensor_data_t;

// Type definitions
typedef enum {
    SENSOR_OK = 0,
    SENSOR_ERROR,
    SENSOR_INITIALIZATION,
    SENSOR_CALIBRATION,
    SENSOR_PROCESSED,
} sensor_status_t;

typedef struct {
    bool is_calibrated;
    int buffer_index;
    int current_min_value; 
    int current_max_value; 
    float sensor_voltage;
} sensor_context_t;

// Callback Prototype: 
typedef void (*sensor_callback_t)(sensor_data_t* sensor_data);
void sensor_init(sensor_callback_t callback);
void sensor_process(nrf_saadc_value_t *samples);

sensor_status_t get_sensor_status(void);
sensor_data_t get_sensor_data(void);
sensor_context_t get_sensor_context(void) ;

extern void log_sensor_data(const sensor_data_t* data);
extern void sensor_stability_check(void);
//void timers_init(void);



#ifdef __cplusplus
}
#endif

#endif // SENSOR_DRIVER_H