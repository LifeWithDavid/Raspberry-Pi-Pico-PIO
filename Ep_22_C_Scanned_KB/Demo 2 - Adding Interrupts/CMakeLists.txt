cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)
include(pico_extras_import_optional.cmake)

project(print_kb_irq C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "2.1.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.1.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# If you want debug output from USB (pass -DPICO_STDIO_USB=1) this ensures you don't lose any debug output while USB is set up
if (NOT DEFINED PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS)
    set(PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS 3000)
endif()

# Initialize the SDK
pico_sdk_init()

# include(example_auto_set_url.cmake)

# added from local cmakelist.txt ********************************
add_executable(${PROJECT_NAME}
        print_kb_irq.c
		print_kb_irq.pio
        )

# pull in common dependencies and additional i2c hardware support
target_link_libraries(${PROJECT_NAME}
        pico_stdlib
        hardware_pio
		hardware_dma
        hardware_irq
		hardware_structs
        )

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/print_kb_irq.pio)		

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# add url via pico_set_program_url
# example_auto_set_url(print_kb_irq)
# **********************************

#[[
function(add_subdirectory_exclude_platforms NAME)
    if (ARGN)
        if (PICO_PLATFORM IN_LIST ARGN)
            message("Skipping ${NAME} example which is unsupported on this platform")
            return()
        endif()
        foreach(PATTERN IN LISTS ARGN)
            string(REGEX MATCH "${PATTERN}" MATCHED "${PICO_PLATFORM}")
            if (MATCHED)
                message("Skipping ${NAME} example which is unsupported on this platform")
                return()
            endif()
        endforeach()
    endif()
    add_subdirectory(${NAME})
endfunction()

# Add blink example
add_subdirectory_exclude_platforms(blink)
add_subdirectory_exclude_platforms(blink_simple)

# Add hello world example
add_subdirectory_exclude_platforms(hello_world)

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        )
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-maybe-uninitialized)
endif()

# Hardware-specific examples in subdirectories:
add_subdirectory(adc)
add_subdirectory(binary_info)
add_subdirectory(bootloaders)
add_subdirectory(clocks)
add_subdirectory(cmake)
add_subdirectory(dcp)
add_subdirectory(divider)
add_subdirectory(dma)
add_subdirectory(flash)
add_subdirectory(gpio)
add_subdirectory(hstx)
add_subdirectory(i2c)
add_subdirectory(interp)
add_subdirectory(multicore)
add_subdirectory(otp)
add_subdirectory(picoboard)
add_subdirectory(pico_w)
add_subdirectory(pio)
add_subdirectory(pwm)
add_subdirectory(reset)
add_subdirectory(rtc)
add_subdirectory(spi)
add_subdirectory(system)
add_subdirectory(timer)
add_subdirectory(uart)
add_subdirectory(universal)
add_subdirectory(usb)
add_subdirectory(watchdog)
add_subdirectory(sha)
add_subdirectory(freertos)
]]
