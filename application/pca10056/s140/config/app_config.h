
#ifndef APP_CONFIG_H
#define APP_CONFIG_H
// <<< Use Configuration Wizard in Context Menu >>>\n

#define NRF_LOG_DEFAULT_BACKENDS NRF_LOG_BACKEND_RTT

#define BLE_NUS_ENABLED 0            // Enable the NUS service. 
#define NRF_SDH_BLE_VS_UUID_COUNT 0  // Number of VS UUIDs used. 

#define PWM_ENABLED 1 // PWM peripheral driver - legacy layer
#define PWM0_ENABLED 1 // PWM0_ENABLED  - Enable PWM0 instance
#define NRFX_PWM_ENABLED 1 // nrfx_pwm - PWM peripheral driver

#define NRF_LOG_BACKEND_UART_ENABLED 0 // nrf_log_backend_uart - Log UART backend
//#define NRF_LOG_BACKEND_UART_TX_PIN 6 // P0.06 is used for TX (transmit)
//#define NRF_LOG_BACKEND_UART_BAUDRATE 30801920 // 30801920=> 115200 baud 

#define NRF_LOG_ALLOW_OVERFLOW 1  // Allows processing of log messages in idle state
#define NRF_LOG_STR_PUSH_BUFFER_SIZE 128  // Buffer size for strings
#define NRF_LOG_BUFSIZE 1024 
#define NRF_LOG_DEFAULT_LEVEL 4  // Severity level: ERROR, WARNING, INFO, and DEBUG

#define NRF_LOG_BACKEND_RTT_ENABLED 1
#define NRF_LOG_BACKEND_RTT_TEMP_BUFFER_SIZE 64
#define NRF_LOG_ENABLED 1
#define NRF_LOG_DEFERRED 1

// Conflict: /nRF5_SDK/components/libraries/button/app_button.c:54: warning: "NRF_LOG_LEVEL" redefined
// #define NRF_LOG_LEVEL 1

// SAADC_ENABLED - nrf_drv_saadc - SAADC peripheral driver - legacy layer
#define SAADC_ENABLED 1

// 0=> 8 bit 256, 1=> 10 bit 1024, 2=> 12 bit 4096, 3=> 14 bit 16384
#define SAADC_CONFIG_RESOLUTION 2

// Priorities 0,1,4,5 (nRF52) are reserved for SoftDevice
#define SAADC_CONFIG_IRQ_PRIORITY 6

#define NRFX_SAADC_ENABLED 1
#define NRFX_SAADC_CONFIG_LOG_ENABLED 1 // Enables logging in the module.


#define NRFX_SAADC_ENABLED 1 // nrfx_saadc - SAADC peripheral driver

// 0=> 8 bit, 1=> 10 bit, 2=> 12 bit, 3=> 14 bit 
#define NRFX_SAADC_CONFIG_RESOLUTION 2 // Resolution


#define NRFX_SAADC_CONFIG_LP_MODE 0 // Enabling low power mode

// 0=> 0 (highest), 1, 2, 3, 4, 5, 6, 7 
#define NRFX_SAADC_CONFIG_IRQ_PRIORITY 6 // Interrupt priority

// 0=> Off, 1=> Error, 2=> Warning, 3=> Info, 4=> Debug 
#define NRFX_SAADC_CONFIG_LOG_LEVEL 3 // Default Severity level

// 0=> 32768 Hz, 1=> 16384 Hz, 3=> 8192 Hz, 7=> 4096 Hz, 15=> 2048 Hz, 31=> 1024 Hz 
//#define APP_TIMER_CONFIG_RTC_FREQUENCY 0
//#define APP_TIMER_CONFIG_RTC_FREQUENCY APP_TIMER_FREQ_32kHz

#define NRFX_TIMER_ENABLED 1    // nrfx_timer - TIMER periperal driver
#define NRFX_TIMER0_ENABLED 1   //  Enable TIMER0 instance
#define NRFX_TIMER1_ENABLED 1   // Enable TIMER1 instance    
#define NRFX_TIMER2_ENABLED 0
#define NRFX_TIMER3_ENABLED 0
#define NRFX_TIMER4_ENABLED 0

// 0=> 16 MHz 1=> 8 MHz 2=> 4 MHz 3=> 2 MHz 4=> 1 MHz 5=> 500 kHz 6=> 250 kHz 7=> 125 kHz 8=> 62.5 kHz 9=> 31.25 kHz 
#define NRFX_TIMER_DEFAULT_CONFIG_FREQUENCY 0 // Timer frequency if in Timer mode

// 0=> Timer 1=> Counter 
#define NRFX_TIMER_DEFAULT_CONFIG_MODE 0 // Timer mode or operation

// 0=> 16 bit 1=> 8 bit 2=> 24 bit 3=> 32 bit 
#define NRFX_TIMER_DEFAULT_CONFIG_BIT_WIDTH 0 // Timer counter bit width

// 0=> 0 (highest) ... 7=> 7 
#define NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY 6 // Interrupt priority

#define NRFX_TIMER_CONFIG_LOG_ENABLED 0 // Enables logging in the module.

// 0=> Off 1=> Error 2=> Warning 3=> Info 4=> Debug 
#define NRFX_TIMER_CONFIG_LOG_LEVEL 3 // Default Severity level
 
// 0=> Default 
#define NRFX_TIMER_CONFIG_INFO_COLOR 0
// 0=> Default 
#define NRFX_TIMER_CONFIG_DEBUG_COLOR 0

#define TIMER_ENABLED 1 // nrf_drv_timer - TIMER periperal driver - legacy layer

// 0=> 16 MHz 1=> 8 MHz 2=> 4 MHz 3=> 2 MHz 4=> 1 MHz 5=> 500 kHz 6=> 250 kHz 7=> 125 kHz 8=> 62.5 kHz 9=> 31.25 kHz 
#define TIMER_DEFAULT_CONFIG_FREQUENCY 0 // Timer frequency if in Timer mode

// 0=> Timer 1=> Counter 
#define TIMER_DEFAULT_CONFIG_MODE 0 // Timer mode or operation

// 0=> 16 bit 1=> 8 bit 2=> 24 bit 3=> 32 bit 
#define TIMER_DEFAULT_CONFIG_BIT_WIDTH 3 // Timer counter bit width

// Priorities 0,1,4,5 (nRF52) are reserved for SoftDevice
// 0=> 0 (highest) ... 7=> 7 
#define TIMER_DEFAULT_CONFIG_IRQ_PRIORITY 6 // Interrupt priority
#define TIMER0_ENABLED 1 // Enable TIMER0 instance
#define TIMER1_ENABLED 1 // Enable TIMER1 instance
#define TIMER2_ENABLED 0
#define TIMER3_ENABLED 0
#define TIMER4_ENABLED 0

#endif // APP_CONFIG_H