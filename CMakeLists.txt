cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME Clion_Test)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx) #将此目录作为子项目引入，递归解析CMakeLists

# Link directories setup
# 有些库不开源，不提供.c源文件，仅给出.a或者.so的链接文件，通过此命令查找链接文件的文件夹，与下文引入cubemx的区别是，下文那个库是受到CMake管理的
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
#   如果子目录的 CMakeLists.txt 中用 add_library 定义了库（如 stm32cubemx 库）
#则主项目必须通过 target_link_libraries 显式链接该库，否则主项目无法使用库中的函数或变量。
#这是使用了库管理的方法，如果没有定义库，直接target_sources就不用link，但一般不这样做
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfloat-abi=soft") # 此处是软件浮点；硬件浮点通道也有特定变量
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -U _printf_fLoath")