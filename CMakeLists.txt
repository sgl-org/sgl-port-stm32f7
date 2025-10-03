#
# MIT License
#
# Copyright(c) 2023-present All contributors of SGL
# Document reference link: docs directory
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

cmake_minimum_required(VERSION 3.9)
project(stm32f7xx VERSION "1.0" LANGUAGES ASM C)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set(STM32_HEADER_PATH ${CMAKE_SOURCE_DIR}/include)

configure_file(
    ${CMAKE_MODULE_PATH}/config.h.in
    ${CMAKE_BINARY_DIR}/generated/config.h
)

configure_file(
    ${CMAKE_MODULE_PATH}/sgl_config.h.in
    ${CMAKE_BINARY_DIR}/generated/sgl_config.h
)

add_definitions(
    "-fno-builtin"
    "-ffreestanding"
    "-ffunction-sections"
    "-fdata-sections"
    "-D CORE_CM7"
    "-D STM32F765xx"
)

option(ENABLE_EXAMPLES "Build examples" OFF)
set(STM32_CMSIS_PATH ${CMAKE_SOURCE_DIR}/drivers/CMSIS)
set(STM32_DEVICE_PATH ${STM32_CMSIS_PATH}/Device/ST/STM32F7xx)
set(STM32_HAL_PATH ${CMAKE_SOURCE_DIR}/drivers/STM32F7xx_HAL_Driver)
set(STM32_LDS_PATH ${CMAKE_SOURCE_DIR}/ldscripts)
set(STM32_SGL_PATH ${CMAKE_SOURCE_DIR}/sgl/source)

FILE(GLOB_RECURSE SRC_HEADER "include/*.h")
FILE(GLOB_RECURSE SRC_SOURCE "src/*.c")

FILE(GLOB_RECURSE DRIVERS_HEADER "drivers/*.h")
FILE(GLOB_RECURSE DRIVERS_SOURCE "${STM32_HAL_PATH}/*.c")

set(DRIVERS_SOURCE
    ${DRIVERS_SOURCE}
    ${STM32_DEVICE_PATH}/Source/Templates/gcc/startup_stm32f765xx.s
    ${STM32_DEVICE_PATH}/Source/Templates/system_stm32f7xx.c
)

set(PROJECT_FILES
    ${SRC_HEADER}
    ${SRC_SOURCE}
    ${DRIVERS_HEADER}
    ${DRIVERS_SOURCE}
    ${STM32_SGL_PATH}/core/sgl_log.c
    ${STM32_SGL_PATH}/core/sgl_math.c
    ${STM32_SGL_PATH}/core/sgl_core.c
    ${STM32_SGL_PATH}/core/sgl_event.c
    ${STM32_SGL_PATH}/core/sgl_anim.c
    ${STM32_SGL_PATH}/draw/sgl_draw_arc.c
    ${STM32_SGL_PATH}/draw/sgl_draw_bar.c
    ${STM32_SGL_PATH}/draw/sgl_draw_circle.c
    ${STM32_SGL_PATH}/draw/sgl_draw_icon.c
    ${STM32_SGL_PATH}/draw/sgl_draw_line.c
    ${STM32_SGL_PATH}/draw/sgl_draw_pie.c
    ${STM32_SGL_PATH}/draw/sgl_draw_rect.c
    ${STM32_SGL_PATH}/draw/sgl_draw_ring.c
    ${STM32_SGL_PATH}/draw/sgl_draw_text.c
    ${STM32_SGL_PATH}/draw/sgl_draw.c
    ${STM32_SGL_PATH}/fonts/sgl_ascii_song23.c
    ${STM32_SGL_PATH}/mm/other/sgl_mm.c
    ${STM32_SGL_PATH}/widgets/arc/sgl_arc.c
    ${STM32_SGL_PATH}/widgets/button/sgl_button.c
    ${STM32_SGL_PATH}/widgets/checkbox/sgl_checkbox.c
    ${STM32_SGL_PATH}/widgets/circle/sgl_circle.c
    ${STM32_SGL_PATH}/widgets/icon/sgl_icon.c
    ${STM32_SGL_PATH}/widgets/label/sgl_label.c
    ${STM32_SGL_PATH}/widgets/line/sgl_line.c
    ${STM32_SGL_PATH}/widgets/listview/sgl_listview.c
    ${STM32_SGL_PATH}/widgets/msgbox/sgl_msgbox.c
    ${STM32_SGL_PATH}/widgets/rectangle/sgl_rectangle.c
    ${STM32_SGL_PATH}/widgets/ring/sgl_ring.c
    ${STM32_SGL_PATH}/widgets/slider/sgl_slider.c
    ${STM32_SGL_PATH}/widgets/switch/sgl_switch.c
    ${STM32_SGL_PATH}/widgets/textbox/sgl_textbox.c
    ${STM32_SGL_PATH}/widgets/textline/sgl_textline.c
    ${STM32_SGL_PATH}/widgets/numberkbd/sgl_numberkbd.c
)

include_directories(${CMAKE_BINARY_DIR}/generated)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${STM32_SGL_PATH})
include_directories(${STM32_SGL_PATH}/include)

include_directories(${STM32_CMSIS_PATH}/Include)
include_directories(${STM32_DEVICE_PATH}/Include)
include_directories(${STM32_HAL_PATH}/Inc)
include_directories(${STM32_HAL_PATH}/Inc/Legacy)

set(MCPU "-march=armv7e-m -mcpu=cortex-m7 -mthumb")
set(MFLOAT_ABI "-mfpu=fpv5-d16 -mfloat-abi=hard")
set(LINKER_SCRIPT ${STM32_LDS_PATH}/STM32F765VITX_FLASH.ld)

set(CMAKE_C_FLAGS "${MCPU} ${MFLOAT_ABI} -std=gnu11 --specs=nano.specs")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp")

set(CMAKE_EXE_LINKER_FLAGS
    "-T ${LINKER_SCRIPT} \
     --specs=nosys.specs \
     -u _printf_float \
     -Wl,--gc-sections \
     -Wl,--print-memory-usage \
     -Wl,-Map=${CMAKE_PROJECT_NAME}.map"
)

add_library(${CMAKE_PROJECT_NAME} OBJECT ${PROJECT_FILES})
if(ENABLE_EXAMPLES)
    add_subdirectory(examples)
endif()
