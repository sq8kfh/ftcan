cmake_minimum_required(VERSION 3.7)

project(ftcan VERSION 0.0.0)

set(PROGRAMMER jtag3isp)
set(MMCU atmega32m1)
set(F_CPU 8000000)

set(SOURCE_FILES main.c ft220x.c ft220x.h can.c can.h)

set(CMAKE_SYSTEM_NAME Generic)

set(PATHS "/usr/local/bin/ /opt/local/bin/")

set(FTCAN_VERSION_MAJOR Y)
set(FTCAN_VERSION_MINOR Y)
configure_file(config.h.in config.h)

function(add_additional_clean_file FILENAME)
    get_directory_property(EXTRA_CLEAN_FILES ADDITIONAL_MAKE_CLEAN_FILES)
    set_directory_properties(
        PROPERTIES
        ADDITIONAL_MAKE_CLEAN_FILES "${EXTRA_CLEAN_FILES};${FILENAME}"
    )

    set_source_files_properties(${FILENAME} PROPERTIES GENERATED TRUE)
endfunction(add_additional_clean_file)

find_program(AVR_C_COMPILER avr-gcc PATHS ${PATHS})
find_program(AVR_OBJCOPY avr-objcopy PATHS ${PATHS})
find_program(AVR_SIZE avr-size PATHS ${PATHS})
find_program(AVRDUDE avrdude PATHS ${PATHS})
find_program(SREC_CAT srec_cat PATHS ${PATHS})

set(CMAKE_C_COMPILER ${AVR_C_COMPILER})

if(NOT BUILD_DIRECTORY)
    set(BUILD_DIRECTORY /build CACHE STRING "Setting build directory to /build")
endif(NOT BUILD_DIRECTORY)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}${BUILD_DIRECTORY}")

if(NOT SOURCE_FILES)
    message(FATAL_ERROR "SOURCE_FILES variable is not defined")
endif(NOT SOURCE_FILES)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if(NOT F_CPU)
    message(FATAL_ERROR "F_CPU variable is not defined")
endif(NOT F_CPU)

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

#-save-temps -Wa,-adhlmn=main.lst

set_target_properties(
        ${PROJECT_NAME}
        PROPERTIES
        OUTPUT_NAME ${ELF_FILENAME}
        COMPILE_FLAGS "-mmcu=${MMCU} -I. -gdwarf-2 -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Wall -Wno-unknown-pragmas -Wstrict-prototypes -Wundef -std=gnu99 -Wl,--gc-section"
        LINK_FLAGS "-mmcu=${MMCU} -I. -gdwarf-2 -Os -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -Wall -Wno-unknown-pragmas -Wstrict-prototypes -Wundef -std=gnu99 -Wundef -MD -MP -MF .dep/main.elf.d --output ${ELF_FILE} -Wl,-lc,--entry=main"
)

add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${AVR_OBJCOPY} -O ihex -R .eeprom ${ELF_FILE} ${HEX_FILE}
        COMMAND ${AVR_OBJCOPY} -O ihex -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 ${ELF_FILE} ${EEP_FILE}
        COMMAND ${AVR_SIZE} ${ELF_FILE} --mcu=${MMCU} --format=avr
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
