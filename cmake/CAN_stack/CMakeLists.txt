cmake_minimum_required(VERSION 3.22)
# Enable CMake support for ASM and C languages
enable_language(C ASM)

# CAN_Stack include paths
set(CAN_Stack_Include_Dirs
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/CAN_Stack/INCLUDE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/CAN_Stack/INCLUDE/Configs
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/CAN_Stack/INCLUDE/General

        ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/CAN_Stack/utils
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/CAN_Stack/GenData
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/CAN_Stack/Cfg
)

set(CAN_Stack_Src
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/CAN_Stack/Cfg/util_platform_adapter.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/CAN_Stack/PduR/PduR.c
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Middlewares/CAN_Stack/Cfg/PduR_Lcfg.c
)

add_library(CAN_Stack OBJECT)
target_include_directories(CAN_Stack PUBLIC  ${CAN_Stack_Include_Dirs})
target_sources(CAN_Stack PRIVATE ${CAN_Stack_Src})

target_link_libraries(CAN_Stack PUBLIC stm32cubemx)