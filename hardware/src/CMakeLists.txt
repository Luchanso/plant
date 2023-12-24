cmake_minimum_required(VERSION 3.27)

target_sources(
    ${PROJECT_NAME} PRIVATE
    bme280.cpp
    bme280.h
    convert_util.cpp
    convert_util.h
    ds3231.cpp
    ds3231.h
    i2c-avr.cpp
    i2c.h
    main.cpp
    plant_message_struct.h
    proto.cpp
    proto.h
    scd40.cpp
    scd40.h
    time_struct.h
    timer_usart.h
    timers.cpp
    timers.h
    usart.cpp
    usart.h
    avr-new.h
    avr-new.cpp
)
