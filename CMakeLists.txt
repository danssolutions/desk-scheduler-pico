# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/user/pico/pico-sdk")

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(scheduler C CXX ASM)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

set(PICO_CXX_ENABLE_RTTI 1)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

#add_executable(scheduler Board_project.cpp buzzer.cpp button.cpp display.cpp WS2812.cpp)
add_executable(scheduler main.cpp buzzer.cpp button.cpp WS2812.cpp)

pico_generate_pio_header(scheduler ${CMAKE_CURRENT_LIST_DIR}/WS2812.pio)

pico_set_program_name(scheduler "scheduler")
pico_set_program_version(scheduler "0.1")

pico_enable_stdio_uart(scheduler 1)
pico_enable_stdio_usb(scheduler 0)

add_subdirectory(pico-ssd1306)

# Add the standard library to the build
target_link_libraries(scheduler
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(scheduler PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

# Add any user requested libraries
target_link_libraries(scheduler 
        hardware_i2c
        hardware_pio
        hardware_pwm
        hardware_adc
        pico_ssd1306
        pico_cyw43_arch_lwip_threadsafe_background
        )

pico_add_extra_outputs(scheduler)

