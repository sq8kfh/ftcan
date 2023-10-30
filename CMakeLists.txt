cmake_minimum_required(VERSION 3.7)
include(CMakeDependentOption)

find_program(AVR_C_COMPILER avr-gcc REQUIRED)
find_program(AVR_CXX_COMPILER avr-g++ REQUIRED)
find_program(AVR_OBJCOPY avr-objcopy REQUIRED)
find_program(AVR_SIZE avr-size REQUIRED)
find_program(AVRDUDE avrdude)
find_program(SREC_CAT srec_cat REQUIRED)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)
set(CMAKE_C_COMPILER ${AVR_C_COMPILER})
set(CMAKE_CXX_COMPILER ${AVR_CXX_COMPILER})

project(ftcan VERSION 1.3)

set(PROGRAMMER jtag3isp)
set(MMCU atmega32m1)
set(FREQ 8000000)

set(SOURCE_FILES main.c ft220x.c ft220x.h can.c can.h)

configure_file(config.h.in config.h)

function(add_additional_clean_file FILENAME)
    get_directory_property(EXTRA_CLEAN_FILES ADDITIONAL_MAKE_CLEAN_FILES)
    set_directory_properties(
        PROPERTIES
        ADDITIONAL_MAKE_CLEAN_FILES "${EXTRA_CLEAN_FILES};${FILENAME}"
    )

    set_source_files_properties(${FILENAME} PROPERTIES GENERATED TRUE)
endfunction(add_additional_clean_file)

if(NOT BUILD_DIRECTORY)
    set(BUILD_DIRECTORY /build CACHE STRING "Setting build directory to /build")
endif(NOT BUILD_DIRECTORY)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}${BUILD_DIRECTORY}")

if(NOT SOURCE_FILES)
    message(FATAL_ERROR "SOURCE_FILES variable is not defined")
endif(NOT SOURCE_FILES)

if(NOT FREQ)
    message(FATAL_ERROR "FREQ variable is not defined")
endif(NOT FREQ)

if(NOT PROGRAMMER)
    message(FATAL_ERROR "PROGRAMMER variable is not defined")
endif(NOT PROGRAMMER)

if(NOT MMCU)
    message(FATAL_ERROR "MMCU variable is not defined")
endif(NOT MMCU)

message(STATUS "Building ${PROJECT_NAME} v${PROJECT_VERSION} for ${MMCU} with frequency ${F_CPU}")
message(STATUS "Source files: ${SOURCE_FILES}")
message(STATUS "Using flags: ${CMAKE_C_FLAGS}")

set(ELF_FILENAME ${PROJECT_NAME}.elf)

set(ELF_FILE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${ELF_FILENAME}")
set(HEX_FILE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.hex")
set(EEP_FILE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.eep")

add_additional_clean_file(${ELF_FILE})
add_additional_clean_file(${HEX_FILE})
add_additional_clean_file(${EEP_FILE})

add_compile_options(
        -mmcu=${MMCU}
        -I.
        -gdwarf-2
        -DF_CPU=${FREQ}UL
        -Os
        -funsigned-char
        -funsigned-bitfields
        -fpack-struct
        -fshort-enums
        -Wall
        -Wno-unknown-pragmas
        -Wstrict-prototypes
        -Wundef
        -std=gnu99
        -Wl,--gc-section
)

if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 12.0)
    add_compile_options(
            --param=min-pagesize=0
    )
    message(STATUS "GCC version >=12 add: --param=min-pagesize=0")
endif(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 12.0)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
        OUTPUT_NAME ${ELF_FILENAME}
        LINK_FLAGS "-mmcu=${MMCU} -I. -gdwarf-2 -DF_CPU=${FREQ}UL -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Wall -Wno-unknown-pragmas -Wstrict-prototypes -Wundef -std=gnu99 -Wundef -MD -MP -MF .dep/main.elf.d --output ${ELF_FILE} -Wl,-lc,--entry=main")

add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${AVR_OBJCOPY} -O ihex -R .eeprom ${ELF_FILE} ${HEX_FILE}
        COMMAND ${AVR_OBJCOPY} -O ihex -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 ${ELF_FILE} ${EEP_FILE}
        COMMAND ${AVR_SIZE} --format=gnu ${ELF_FILE}
        COMMENT "Post processsing"
)

add_custom_target("flash"
        ${AVRDUDE} -p ${MMCU} -c ${PROGRAMMER} -U flash:w:${HEX_FILE}
        DEPENDS ${PROJECT_NAME}
        COMMENT "Flash uploading"
)

add_custom_target("eeprom"
        ${AVRDUDE} -p ${MMCU} -c ${PROGRAMMER} -U eeprom:w:${EEP_FILE}
        DEPENDS ${PROJECT_NAME}
        COMMENT "Eeprom uploading"
        )
