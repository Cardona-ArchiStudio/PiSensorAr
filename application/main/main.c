#include "sensor_driver.h"
#include "log_driver.h"
#include "nrf_drv_uart.h"
#include "nrf_gpio.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "nrf_drv_timer.h"
//#include "nrf_drv_uart.h"
#include "nrf_uart.h"

#define HIGH_INTENSITY 255
#define LOW_INTENSITY 0

#define SAADC_BUF_COUNT        2
#define SAADC_SAMPLE_FREQUENCY 8000

const nrf_drv_timer_t STABILITY_TIMER = NRF_DRV_TIMER_INSTANCE(0);

// UART instance and configuration
static const nrf_drv_uart_t uart_instance = NRF_DRV_UART_INSTANCE(0);
static nrf_drv_uart_config_t uart_config = NRF_DRV_UART_DEFAULT_CONFIG;

static nrf_saadc_value_t samples[SAADC_BUF_COUNT][SAADC_BUF_SIZE];
static nrfx_saadc_channel_t channel_config = NRFX_SAADC_DEFAULT_CHANNEL_SE(NRF_SAADC_INPUT_AIN0, 0);

volatile bool data_ready_flag = false;
volatile bool is_pwm_active = false;
nrf_saadc_value_t* current_buffer = NULL;

static uint32_t next_free_buf_index(void);
void sensor_feedback(sensor_data_t* sensor_data);
void set_rgb_intensity(uint16_t red, uint16_t green, uint16_t blue);
static void event_handler(nrfx_saadc_evt_t const * p_event);
static void adc_start(uint32_t cc_value);
static uint16_t map_intensity(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max);
static void timer_event_handler(nrf_timer_event_t event_type, void* p_context);
static void timer_setup(void);

// Simple function to provide an index to the next input buffer
// Will simply alernate between 0 and 1 when SAADC_BUF_COUNT is 2
static uint32_t next_free_buf_index(void)
{
    static uint32_t buffer_index = -1;
    buffer_index = (buffer_index + 1) % SAADC_BUF_COUNT;
    return buffer_index;
}
// UART event handler
void uart_event_handler(nrf_drv_uart_event_t * p_event, void* p_context) {
    // We won't do anything specific in the UART callback for the moment
}
// Initialize UART
void uart_init(void) {
    uart_config.pseltxd = NRF_GPIO_PIN_MAP(0,6);  // TX Pin
    uart_config.pselrxd = NRF_GPIO_PIN_MAP(0,8);  // RX Pin (even if we're not using RX, it's a good idea to set it)
    uart_config.baudrate = NRF_UART_BAUDRATE_115200;
    uart_config.interrupt_priority = APP_IRQ_PRIORITY_LOWEST;
    uart_config.p_context = NULL;
    
    ret_code_t err_code = nrf_drv_uart_init(&uart_instance, &uart_config, uart_event_handler);
    APP_ERROR_CHECK(err_code);
}

// Function to set the RGB intensity and send the data via UART
void set_rgb_intensity(uint16_t red, uint16_t green, uint16_t blue)
{
    // Here you can include any hardware-specific code that sets the intensity of the RGB LEDs
    // based on the provided values of red, green, and blue.

    // For now, the function sends the RGB intensity data via UART.
    char message[20];
    sprintf(message, "DATA:%d,%d,%d\n", red, green, blue);
    nrf_drv_uart_tx(&uart_instance, (uint8_t *)message, strlen(message));
}

// Callback function to handle new sensor reading
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
    else if (sensor_value > sensor_data->golden_reference + STABILITY_THRESHOLD)
    {
        // As the sensor_value increases over the golden_reference, the green intensity increases.
        green_intensity = map_intensity(sensor_value,
                                        sensor_data->golden_reference + STABILITY_THRESHOLD, 
                                        sensor_data->top_reference,
                                        LOW_INTENSITY, HIGH_INTENSITY);
    }

    set_rgb_intensity(red_intensity, green_intensity, blue_intensity);
}

static uint16_t map_intensity(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
    
    // Ensure values are within the expected range
    if(x < in_min) return out_min;
    if(x > in_max) return out_max;

    // Map the value linearly
    //return (uint8_t)(((x - in_min) * (out_max - out_min) / (in_max - in_min)) + out_min);
    return (uint8_t)(x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static void event_handler(nrfx_saadc_evt_t const * p_event)
{
    ret_code_t err_code;
    switch (p_event->type)
    {
        case NRFX_SAADC_EVT_DONE:
            current_buffer = p_event->data.done.p_buffer;
            data_ready_flag = true;
            break;

        case NRFX_SAADC_EVT_BUF_REQ:
            // Set up the next available buffer
            err_code = nrfx_saadc_buffer_set(&samples[next_free_buf_index()][0], SAADC_BUF_SIZE);
            APP_ERROR_CHECK(err_code);
            break;
    }
}

static void adc_start(uint32_t cc_value)
{
    ret_code_t err_code;

    nrfx_saadc_adv_config_t saadc_adv_config = NRFX_SAADC_DEFAULT_ADV_CONFIG;
    saadc_adv_config.internal_timer_cc = cc_value;
    saadc_adv_config.start_on_end = true;

    err_code = nrfx_saadc_advanced_mode_set((1<<0), NRF_SAADC_RESOLUTION_10BIT,
                                            &saadc_adv_config,
                                            event_handler);
    APP_ERROR_CHECK(err_code);
                                            
    // Configure two buffers to ensure double buffering of samples, to avoid data 
    // loss when the sampling frequency is high
    err_code = nrfx_saadc_buffer_set(&samples[next_free_buf_index()][0], SAADC_BUF_SIZE);
    APP_ERROR_CHECK(err_code);

    err_code = nrfx_saadc_buffer_set(&samples[next_free_buf_index()][0], SAADC_BUF_SIZE);
    APP_ERROR_CHECK(err_code);

    err_code = nrfx_saadc_mode_trigger();
    APP_ERROR_CHECK(err_code);
}
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
 * @brief Function for application main entry.
 */
int main(void)
{
    log_init();
    //timers_init(); // Defined in sensor_driver.h already
    //NRF_LOG_INFO("Timer count: %lu", app_timer_cnt_get());

    uart_init();
    // Initialize the sensor passing to it the callback function pointer 
    sensor_init(sensor_feedback);

    ret_code_t err_code;
    err_code = nrfx_saadc_init(NRFX_SAADC_CONFIG_IRQ_PRIORITY);
    APP_ERROR_CHECK(err_code);
 
    err_code = nrfx_saadc_channels_config(&channel_config, 1);
    APP_ERROR_CHECK(err_code);

    uint32_t adc_cc_value = 16000000 / SAADC_SAMPLE_FREQUENCY;
    if(adc_cc_value < 80 || adc_cc_value > 2047)
    {
        NRF_LOG_ERROR("SAMPLERATE frequency outside legal range. Consider using a timer to trigger the ADC instead.");
        APP_ERROR_CHECK(false);
    }
    adc_start(adc_cc_value);

    // Timer Initialization and Start
    timer_setup();
    
    while (1)
    {
        if (data_ready_flag) {
            // Process the input based on the saadc event
            sensor_process(current_buffer);
            data_ready_flag = false;
            nrf_delay_ms(SENSOR_READ_DELAY);  // Add a delay to slow down the loop
        }
        // NRF_LOG_FLUSH();
        while(NRF_LOG_PROCESS() != NRF_SUCCESS);
        __WFE();
    }  
}