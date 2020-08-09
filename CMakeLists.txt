cmake_minimum_required(VERSION 2.8)

# program names
set(AVRCPP   avr-g++)
set(AVRC     avr-gcc)
set(AVRSTRIP avr-strip)
set(OBJCOPY  avr-objcopy)
set(OBJDUMP  avr-objdump)
set(AVRSIZE  avr-size)
set(AVRDUDE  avrdude)

# Sets the compiler
# Needs to come before the project function
set(CMAKE_SYSTEM_NAME   Generic)
set(CMAKE_CXX_COMPILER  ${AVRCPP})
set(CMAKE_C_COMPILER    ${AVRC})
set(CMAKE_ASM_COMPILER  ${AVRC})

project(ATTiny13Tests)

#Sources

set(SOURCES_DIR ${${PROJECT_NAME}_SOURCE_DIR})

set(AVR_INCLUDES /usr/local/Cellar/avr-gcc/8.3.0/avr/include)
#set(AVR_INCLUDES /usr/local/CrossPack-AVR/avr/include)
#set(AVR_INCLUDES /usr/lib/avr/include)

set(NEW_LINE \n)
set(SPACE " ")

#execute_process(COMMAND find src -type d | sed s/^/x/ )
#execute_process(COMMAND find ${SOURCES_DIR}/src -type d OUTPUT_FILE /Users/ein/kuku)
#execute_process(COMMAND A=`find ${SOURCES_DIR}/src -type d`; echo $A OUTPUT_VARIABLE SELF_INCLUDES)
execute_process(
    COMMAND find ${SOURCES_DIR}/src -type d
    COMMAND tr ${NEW_LINE} ${SPACE}
    OUTPUT_VARIABLE SELF_INCLUDES)

separate_arguments(SELF_INCLUDES)

set(SOURCE_FILES
        src/m-toolbox/MissingCPP.h
        src/m-toolbox/MissingCPP.cpp

        src/m-toolbox/Macro.h
        src/m-toolbox/BitAccess.h

        src/m-toolbox/InputPin.h
        src/m-toolbox/OutputPin.h

        src/m-app/main.cpp)

# Project setup
#include_directories(BEFORE ${AVR_INCLUDES} ${SELF_INCLUDES})
include_directories(BEFORE ${AVR_INCLUDES})

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}.elf")

# Variables regarding the AVR chip
set(MCU   attiny13a)
set(F_CPU 9600000)
add_definitions(-DF_CPU=${F_CPU})

# Compiler flags
set(CSTANDARD "-std=gnu99")
#set(CDEBUG    "-gstabs -g -ggdb")
set(CWARN     "-Wall -Wno-unknown-pragmas")
set(CTUNING   "-Wl,--gc-sections -Wl,--relax -fshort-enums -ffunction-sections -fdata-sections -fno-threadsafe-statics")
set(COPT      "-Os -lm -lprintf_flt")
set(CMCU      "-mmcu=${MCU}")
set(CDEFS     "-DF_CPU=${F_CPU}")

set(CFLAGS   "${CMCU} ${CDEBUG} ${CDEFS} ${COPT} ${CWARN} ${CTUNING} ${CSTANDARD}")
set(CXXFLAGS "${CMCU} ${CDEBUG} ${CDEFS} ${COPT} ${CWARN} ${CTUNING} -fno-exceptions")

set(CMAKE_C_FLAGS   "${CFLAGS}")
set(CMAKE_CXX_FLAGS "${CXXFLAGS}")
set(CMAKE_ASM_FLAGS   "${CFLAGS}")

# Compiling targets
#add_custom_target(strip ALL     ${AVRSTRIP} "${PROJECT_NAME}.elf" DEPENDS ${PROJECT_NAME})
#add_custom_target(hex   ALL     ${OBJCOPY} -O ihex -R .eeprom "${PROJECT_NAME}.elf" "${PROJECT_NAME}.hex" DEPENDS strip)
add_custom_target(hex   ALL     ${OBJCOPY} -O ihex -R .eeprom "${PROJECT_NAME}.elf" "${PROJECT_NAME}.hex" DEPENDS ${PROJECT_NAME})

#add_custom_target(eeprom        ${OBJCOPY} -j .eeprom --change-section-lma .eeprom=0 -O ihex "${PROJECT_NAME}.elf" "${PROJECT_NAME}.eeprom" DEPENDS strip)
#add_custom_target(disassemble   ${OBJDUMP} -S "${PROJECT_NAME}.elf" > "${PROJECT_NAME}.lst" DEPENDS strip)
add_custom_target(disassemble   ${OBJDUMP} -S "${PROJECT_NAME}.elf" > "${PROJECT_NAME}.lst" DEPENDS ${PROJECT_NAME})

# Flashing targets

set(DUDE_PROGRAMMER stk500v1)
set(DUDE_USBPORT /dev/cu.usbmodem14101)
set(DUDE_BAUD 9600)
set(DUDE_PART ATTINY13)

set(DUDE_ARGS -c ${DUDE_PROGRAMMER} -p ${DUDE_PART} -P ${DUDE_USBPORT} -b ${DUDE_BAUD})

add_custom_target(flash ${AVRDUDE} ${DUDE_ARGS} -F -U flash:w:${PROJECT_NAME}.hex DEPENDS hex)

add_custom_target(size ${AVRSIZE} ${PROJECT_NAME}.elf DEPENDS ${PROJECT_NAME})

#add_custom_target(flash_usbtiny ${AVRDUDE} -c usbtiny -p ${MCU} -U flash:w:${PROJECT_NAME}.hex DEPENDS hex)
#add_custom_target(flash_usbasp  ${AVRDUDE} -c usbasp -p ${MCU} -U flash:w:${PROJECT_NAME}.hex DEPENDS hex)
#add_custom_target(flash_ardisp  ${AVRDUDE} -c avrisp -p ${MCU} -b 19200 -P ${DUDE_USBPORT} -U flash:w:${PROJECT_NAME}.hex DEPENDS hex)
#add_custom_target(flash_109     ${AVRDUDE} -c avr109 -p ${MCU} -b 9600 -P ${DUDE_USBPORT} -U flash:w:${PROJECT_NAME}.hex DEPENDS hex)
#add_custom_target(flash_eeprom  ${AVRDUDE} -c ${DUDE_PROGRAMMER} -p ${MCU} ${DUDE_ARGS} -U eeprom:w:${PROJECT_NAME}.hex DEPENDS eeprom)

# Fuses (For ATMega328P-PU, Calculated using http://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega328p)
#add_custom_target(reset         ${AVRDUDE} -c ${DUDE_PROGRAMMER} -p ${MCU} ${DUDE_ARGS} -e)
#add_custom_target(fuses_1mhz    ${AVRDUDE} -c ${DUDE_PROGRAMMER} -p ${MCU} ${DUDE_ARGS} -U lfuse:w:0x62:m)
#add_custom_target(fuses_8mhz    ${AVRDUDE} -c ${DUDE_PROGRAMMER} -p ${MCU} ${DUDE_ARGS} -U lfuse:w:0xE2:m)
#add_custom_target(fuses_16mhz   ${AVRDUDE} -c ${DUDE_PROGRAMMER} -p ${MCU} ${DUDE_ARGS} -U lfuse:w:0xFF:m)
#add_custom_target(fuses_uno     ${AVRDUDE} -c ${DUDE_PROGRAMMER} -p ${MCU} ${DUDE_ARGS} -U lfuse:w:0xFF:m -U hfuse:w:0xDE:m -U efuse:w:0x05:m)
#add_custom_target(set_eeprom_save_fuse   ${AVRDUDE} -c ${DUDE_PROGRAMMER} -p ${MCU} ${DUDE_ARGS} -U hfuse:w:0xD1:m)
#add_custom_target(clear_eeprom_save_fuse ${AVRDUDE} -c ${DUDE_PROGRAMMER} -p ${MCU} ${DUDE_ARGS} -U hfuse:w:0xD9:m)

# Fuses for attiny13a 9.6mhz no brownout
add_custom_target(attiny13a_fuses ${AVRDUDE} -c ${DUDE_PROGRAMMER} -p ${MCU} ${DUDE_ARGS} -U lfuse:w:0x7a:m -U hfuse:w:0xff:m)

# Utilities targets
#add_custom_target(avr_terminal  ${AVRDUDE} -c ${DUDE_PROGRAMMER} -p ${MCU} ${DUDE_ARGS} -nt)

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${PROJECT_NAME}.hex;${PROJECT_NAME}.eeprom;${PROJECT_NAME}.lst")

# Config logging
message("* ")
message("* Project Name:\t${PROJECT_NAME}")
message("* Project Source:\t${SRC_PATH}")
message("* Project Include:\t${INC_PATH}")
message("* Library Include:\t${LIB_INC_PATH}")
message("* ")
message("* Project Source Files:\t${SRC_FILES}")
message("* Library Source Files:\t${LIB_SRC_FILES}")
message("* ")
message("* C Flags:\t${CMAKE_C_FLAGS}")
message("* ")
message("* CXX Flags:\t${CMAKE_C_FLAGS}")
message("* ")
