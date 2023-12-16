cmake_minimum_required(VERSION 3.27)

set(AVR_UPLOADTOOL_PORT /dev/ttyUSB0)
set(AVR_PROGRAMMER      arduino)
set(AVR_UPLOAD_SPEED    57600)
set(AVR_MCU             atmega328p)
set(AVR_MCU_SPEED       16000000UL)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/avr-gcc-toolchain.cmake)

project(PlantMonitorFirmware
        LANGUAGES C CXX
        VERSION 0.0.0.1
        DESCRIPTION "MCU firmware for Plant Monitor project"
       )
       
add_avr_executable(${PROJECT_NAME})
add_subdirectory(${CMAKE_SOURCE_DIR}/src)
