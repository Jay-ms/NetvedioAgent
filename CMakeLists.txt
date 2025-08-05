cmake_minimum_required(VERSION 3.10)

# Processing Parameters
set(LIBC_TYPE "uclibc")
set(PROJECT_DIR "/home/mydrivers/NetvedioAgent")
set(PROJECT_NAME "NetvedioAgent")
message(STATUS "Selected libc type: ${LIBC_TYPE}")

# Set the cross-compiler
set(CMAKE_C_COMPILER "/home/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-gcc")
set(CMAKE_CXX_COMPILER "/home/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-g++")

# Project name
project(${PROJECT_NAME})

# Add macro definitions for conditional compilation
add_definitions(-DLUCKFOX_PICO_PRO_MAX)
message(STATUS "Set Device: Luckfox Pico Pro Max")

# Add library directories
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/${LIBC_TYPE})

# Add macro definitions
add_definitions(-DRV1106_1103)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/Config)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/LCD)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/SPI)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/GPIO)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/PWM)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/PID)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/TCPSERVER)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/TCPCLIENT)

# Add subdirectories
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ 3rdparty.out)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/utils/ utils.out)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/Config)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/LCD)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/SPI)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/GPIO)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/PWM)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/PID)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/TCPSERVER)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/TCPCLIENT)

# Set the runtime path for libraries
set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")

# OpenCV 4
set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib/${LIBC_TYPE}/lib/cmake/opencv4")
find_package(OpenCV REQUIRED) 
include_directories(${OpenCV_INCLUDE_DIRS})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED) #  CMake 查找并配置线程支持库

# Define the source files for the project
set(SRC_DIR "${PROJECT_DIR}/src")
file(GLOB SRC_FILES "${SRC_DIR}/*.cc") # Find all .cc files in the src directory
add_executable(${PROJECT_NAME} ${SRC_FILES}) # Create an executable target with the source files

# Link libraries to the executable
target_link_libraries(${PROJECT_NAME}
    ${OpenCV_LIBS}
    rknnmrt
    Threads::Threads
    rockiva
    sample_comm
    rockit
    rockchip_mpp
    rkaiq
    pthread
    rtsp
    rga
    imageutils
    fileutils
    imagedrawing
    Config
    LCD
    SPI
    GPIO
    PWM
    PID
    TCPSERVER
    TCPCLIENT
)

# Include directories for the target
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/common 
    ${CMAKE_CURRENT_SOURCE_DIR}/common/isp3.x   
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rknn
    ${CMAKE_CURRENT_SOURCE_DIR}/include/librga 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/uAPI2
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/common
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/xcore
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/algos
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/iq_parser
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/iq_parser_v2
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rkaiq/smartIr
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rknn
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/librga/include
    ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/allocator/dma
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/Config
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/LCD
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/SPI
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/GPIO
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/PWM
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/PID
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/TCPSERVER
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/TCPCLIENT
    ${OpenCV_INCLUDE_DIRS}
)

# Set the output directory for the executable
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install/${LIBC_TYPE}/${PROJECT_NAME}_pro_max_demo")
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX})
SET(MODEL_DIR "${PROJECT_DIR}/model")
file(GLOB MODEL_FILES "${MODEL_DIR}/*")
install(FILES ${MODEL_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/model)    