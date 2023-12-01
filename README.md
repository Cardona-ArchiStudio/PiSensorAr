# PiSensorAr Application v 0.1.12
Capacitive sensing technology, particularly in touch or proximity sensors, is predicated on detecting changes in capacitance. Such changes can indicate various environmental interactions, including touch or proximity events, by monitoring associated shifts in voltage or energy within the sensor's capacitor.

## PiSensorAr Application Flow Report
This report outlines the updated interactions among `sensor_driver.h`, `sensor_driver.c`, `main.c`, and additional components that comprise the PiSensorAr application. The application aims to read sensor data, maintain its stability, and communicate feedback via RGB intensity adjustments through UART communication. The design emphasizes modularity and clear separation of concerns, promoting scalability and maintainability.

### Sensor Driver (`sensor_driver`):
- **Header (`sensor_driver.h`)**:
  - **Purpose**: Declares the sensor driver's necessary data structures, constants, and function prototypes.
  - **Key Structures**:
    - `sensor_data_t`: Stores sensor readings, reference values, and voltage stability indicators.
    - `sensor_status_t`: Lists possible sensor states or statuses.
    - `sensor_context_t`: Contains contextual information about the sensor, such as calibration status and range of readings.
- **Implementation (`sensor_driver.c`)**:
  - **Initialization**:
    - `sensor_init()`: Readies the sensor for use and registers a feedback callback.
    - `sensor_initialization()`: Resets sensor data to default values for a fresh start.
  - **Data Processing**:
    - `sensor_process()`: Analyzes sensor data, calibrating if necessary, or proceeding with standard operations if already calibrated.
    - `calibration_process()`: Establishes initial sensor benchmarks for comparison.
    - `operation_process()`: Handles ongoing sensor data interpretation.
  - **Stability Management**:
    - `sensor_stability_check()`: Monitors for consistent readings to confirm stability or trigger recalibration.
    - `auto_calibrate()`: Refines the sensor's baseline measurements for improved accuracy.
  - **Utilities**:
    - `convert_to_voltage()`: Converts raw ADC readings into a voltage representation.
    - `log_sensor_data()`: Captures and logs sensor data for troubleshooting.

### Main Application (`main.c`):
- **Initialization**:
  - Establishes UART communication.
  - Activates the sensor with `sensor_init()`.
  - Prepares ADC for data collection.
  - Sets up a timer for regular stability assessments.
- **Operation Loop**:
  - Routinely polls for new ADC data.
  - Processes new data with `sensor_process()`.
  - Stable readings trigger `sensor_feedback()`, updating RGB LED intensities and communicating via UART with `set_rgb_intensity()`.
- **Support Functions**:
  - `timer_setup()`, `event_handler()`, and `adc_start()` assist with the operational flow, particularly concerning ADC management and timing mechanisms.

### Interactions and Workflow:
- **Main Application (`main.c`)**: Serves as the command center, initializing components and managing the operation loop.
- **Sensor Driver (`sensor_driver.c`)**: Responsible for interpreting sensor data, calibrating, and ensuring reading stability.
- **Feedback Loop**: Stable sensor data prompts a callback execution, which translates readings into visual feedback via an RGB LED.

### Newly Introduced Components:

#### SADC Driver (`sadc`):
- **Driver (`sadc_driver.c`)** and **Header (`sadc_driver.h`)**:
  - Handle the specifics of the Successive Approximation Analog-to-Digital Converter (SAADC), interfacing directly with the hardware to manage analog sensor inputs.

#### UART Driver (`uart`):
- **Driver (`uart_driver.c`)** and **Header (`uart_driver.h`)**:
  - Manage serial communication, ensuring data is correctly transmitted and received over the UART interface.

#### Log Driver (`logs`):
- **Driver (`log_driver.c`)** and **Header (`log_driver.h`)**:
  - Provide a logging interface, crucial for monitoring application behavior and diagnosing issues.

The interaction among these components results in a cohesive system that can reliably sense environmental changes, process and interpret these changes, and respond with appropriate feedback while maintaining a log of operations for review and analysis.

## Microprocessors:
Nordic Semiconductor nrF52840-DK and Arduino UNO v3 (See PiSensorFeedback.ino).

## SDK:
Version nRF5_SDK_17.0.2_d674dde.
See release_notes.txt for details

## Directory structure:
- main/main.c
- components/sadc/sadc_driver.c
- components/sadc/include/sadc_driver.h
- components/uart/uart_driver.c
- components/uart/include/uart_driver.h
- components/logs/log_driver.c
- components/logs/include/log_driver.h
- components/sens/sensor_driver.c
- components/sens/include/sensor_driver.h
