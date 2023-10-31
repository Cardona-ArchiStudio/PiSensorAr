# PiSensorBLE
In capacitive sensing, particularly with capacitive touch or proximity sensors, a change in capacitance often translates to a change in the energy stored in the sensing capacitor. By monitoring the energy or the associated voltage, we can deduce changes in the capacitance, which can in turn indicate proximity or touch events.

# PiSensorBLE Application Flow Report
An overview of how the `sensor_driver.h`, `sensor_driver.c`, and `main.c`  interact and the general flow of the program.

This report provides an overview of the three code snippets and their interactions. The program's primary objective is to read the sensor data, ensuring its stability, and to provide feedback through RGB intensities via an UART communication. It's modular in design, with clear separations of concerns, making it maintainable and scalable.

## 1. **Sensor Driver Header (`sensor_driver.h`)**:
- **Purpose**: Declares the data structures, constants, and function prototypes required for the sensor driver.
- **Key Structures**:
    - `sensor_data_t`: Holds sensor readings, reference values, and voltage stability status.
    - `sensor_status_t`: Enumerates various states/statuses of the sensor.
    - `sensor_context_t`: Maintains the operational context of the sensor, including calibration status and current min/max values.

## 2. **Sensor Driver Implementation (`sensor_driver.c`)**:
- **Purpose**: Implements the logic and functionalities declared in `sensor_driver.h`.
- **Initialization**: 
    - `sensor_init()`: Prepares the sensor for operation and stores a callback for feedback.
    - `sensor_initialization()`: Resets the sensor data and context to default values.
- **Sensor Reading & Processing**:
    - `sensor_process()`: Determines if the sensor is calibrated. If not, it triggers calibration. If calibrated, it proceeds to normal operation.
    - `calibration_process()`: Obtains initial readings from the sensor, computes averages, and establishes reference boundaries.
    - `operation_process()`: Processes the sensor readings during normal operation.
- **Stability Checks & Auto-Calibration**:
    - `sensor_stability_check()`: Evaluates if the sensor readings are stable and triggers auto-calibration if required.
    - `auto_calibrate()`: Updates the golden reference and adjusts the reference boundaries.
- **Utility Functions**:
    - `convert_to_voltage()`: Transforms ADC values to voltage.
    - `log_sensor_data()`: Logs sensor data for debugging purposes.

## 3. **Main Application (`main.c`)**:
- **Purpose**: Orchestrates the initialization and continuous operation of the sensor driver within the context of the application.
- **Initialization**:
    - Sets up the UART for communication.
    - Initializes the sensor using `sensor_init()`.
    - Sets up the ADC for sampling.
    - Initializes a timer for periodic stability checks.
- **Main Loop**:
    - Continuously checks if new ADC data is ready.
    - Once data is ready, it processes the sensor readings using `sensor_process()`.
    - If the readings are stable, the feedback function (`sensor_feedback()`) is invoked.
        - This function maps the sensor value to RGB intensities, which can then be used to control LEDs or other visual indicators.
        - The RGB intensities are also sent via UART using `set_rgb_intensity()`.
- **Supporting Functions**:
    - Several functions like `timer_setup()`, `event_handler()`, and `adc_start()` assist in ADC sampling and timer management.

## **Interaction**:
- The main application (`main.c`) acts as the orchestrator. 
- Upon startup, it initializes all required peripherals and the sensor driver.
- In its continuous loop, it checks for new sensor data. If available, it passes the data to the sensor driver for processing.
- The sensor driver (`sensor_driver.c`) manages the calibration and continuous readings of the sensor. It assesses the stability of readings and, if necessary, recalibrates the sensor.
- If the sensor readings are stable, the driver invokes the feedback callback provided by the main application, enabling the application to respond to the readings (e.g., by changing the color of an RGB LED).
