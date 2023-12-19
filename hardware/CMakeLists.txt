cmake_minimum_required(VERSION 3.27)

#maybe should be set externally
set(AVR_UPLOADTOOL_PORT /dev/ttyUSB0)

#This stuff most likely won't change
set(AVR_PROGRAMMER      arduino)
set(AVR_UPLOAD_SPEED    57600)
set(AVR_MCU             atmega328p)
set(AVR_MCU_SPEED       16000000UL)


set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/avr-gcc-toolchain.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# Just like C++ STL (Standard Template Library), but with focus on embedded
# devices.
FetchContent_Declare(etl
                     GIT_REPOSITORY "https://github.com/etlcpp/etl"
                     GIT_TAG 20.38.10
                     GIT_PROGRESS TRUE
                    )
FetchContent_MakeAvailable(etl)

# AVR-GCC usually do not have any standard C++ libs included, this means it's
# better to disable STL support for ETL to avoid compilation errors.
add_definitions(-DETL_NO_STL)

project(PlantMonitorFirmware
        LANGUAGES C CXX
        VERSION 0.0.0.1
        DESCRIPTION "MCU firmware for Plant Monitor project"
       )

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

add_avr_executable(${PROJECT_NAME})
add_subdirectory(${CMAKE_SOURCE_DIR}/src)

avr_target_link_libraries(${PROJECT_NAME} etl)
