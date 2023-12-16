cmake_minimum_required(VERSION 3.27)

target_sources(
    ${PROJECT_NAME} PRIVATE
    bme280.c
    bme280.h
    convert_util.c
    convert_util.h
    ds3231.c
    ds3231.h
    i2c.c
    i2c.h
    main.c
    plant_message_struct.h
    proto.c
    proto.h
    scd40.c
    scd40.h
    time_struct.h
    timer_usart.h
    timers.c
    timers.h
    usart.c
    usart.h
)
