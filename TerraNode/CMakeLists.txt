cmake_minimum_required(VERSION 3.13)
project(stm32l031_project C ASM)

# ----------------------- KONFIGURACJA MCU -----------------------
set(MCU_CPU "cortex-m0plus")
set(MCU_FPU_FLAGS "") # brak FPU
set(MCU_DEFINES "STM32L031xx;USE_HAL_DRIVER")
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/STM32L031K6TX_FLASH.ld")

# ----------------------- USTAWIENIA KOMPILATORA -----------------
set(CPU_FLAGS "-mcpu=${MCU_CPU} -mthumb ${MCU_FPU_FLAGS}")

set(CMAKE_C_FLAGS "${CPU_FLAGS} -Os -g3 -Wall -fdata-sections -ffunction-sections -fno-common")
set(CMAKE_ASM_FLAGS "${CPU_FLAGS} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS "${CPU_FLAGS} -Wl,--gc-sections -specs=nano.specs -specs=nosys.specs")

add_compile_definitions(${MCU_DEFINES})

# ----------------------- PLIKI ŹRÓDŁOWE -------------------------
file(GLOB_RECURSE HAL_SOURCES
  "${CMAKE_SOURCE_DIR}/Drivers/STM32L0xx_HAL_Driver/Src/*.c"
)

set(PROJECT_SOURCES
  ${CMAKE_SOURCE_DIR}/Src/main.c
  ${CMAKE_SOURCE_DIR}/Src/stm32l0xx_hal_msp.c
  ${CMAKE_SOURCE_DIR}/Src/stm32l0xx_it.c
  ${CMAKE_SOURCE_DIR}/Src/system_stm32l0xx.c
  ${CMAKE_SOURCE_DIR}/Src/syscalls.c
  ${CMAKE_SOURCE_DIR}/Src/sysmem.c
  ${CMAKE_SOURCE_DIR}/Src/BH1750.c
  ${CMAKE_SOURCE_DIR}/Src/comm.c
  ${CMAKE_SOURCE_DIR}/Src/flash_manage.c
  ${CMAKE_SOURCE_DIR}/Src/hal_power.c
  ${CMAKE_SOURCE_DIR}/Src/hal_rtc.c
  ${CMAKE_SOURCE_DIR}/Src/packet.c
  ${CMAKE_SOURCE_DIR}/Src/sensors.c
  ${CMAKE_SOURCE_DIR}/Src/SX1278_hw.c
  ${CMAKE_SOURCE_DIR}/Src/SX1278.c
  ${CMAKE_SOURCE_DIR}/startup_stm32l031k6tx.s
)

# ----------------------- INCLUDE DIRECTORIES --------------------
include_directories(
  ${CMAKE_SOURCE_DIR}/Inc
  ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
  ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32L0xx/Include
  ${CMAKE_SOURCE_DIR}/Drivers/STM32L0xx_HAL_Driver/Inc
  ${CMAKE_SOURCE_DIR}/Drivers/STM32L0xx_HAL_Driver/Inc/Legacy
)

# ----------------------- TWORZENIE BINARÓW ----------------------
add_executable(${PROJECT_NAME}.elf ${PROJECT_SOURCES} ${HAL_SOURCES})

target_link_options(${PROJECT_NAME}.elf PRIVATE
  "-T${LINKER_SCRIPT}"
  "-Wl,-Map=${PROJECT_NAME}.map,--cref"
  "-Wl,--print-memory-usage"
)

# ----------------------- POST-BUILD -----------------------------
find_program(ARM_OBJCOPY arm-none-eabi-objcopy)
find_program(ARM_SIZE arm-none-eabi-size)

if(ARM_OBJCOPY)
  add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${ARM_OBJCOPY} -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND ${ARM_OBJCOPY} -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMENT "Generowanie plików BIN i HEX"
  )
endif()

if(ARM_SIZE)
  add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${ARM_SIZE} ${PROJECT_NAME}.elf
    COMMENT "Rozmiar programu:"
  )
endif()