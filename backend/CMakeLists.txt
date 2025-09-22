cmake_minimum_required(VERSION 3.28.3)
project(srotalekh-backend)
set(CMAKE_CXX_STANDARD 23)

file(GLOB SOURCES src/*.cpp src/**/*.cpp src/**/**/*.cpp)
find_package(cpprestsdk REQUIRED)
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)

set(USE_SYSTEM_JSON ON CACHE BOOL "" FORCE)
message(STATUS "Project build type: ${CMAKE_BUILD_TYPE}")

# Whisper.cpp configurations
# Option to enable GPU support
option(ENABLE_GPU "Enable GPU acceleration for Whisper" OFF)

set(WHISPER_BUILD_TESTS OFF CACHE BOOL "Build whisper tests" FORCE)
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "Build whisper examples" FORCE)
set(WHISPER_BUILD_SERVER OFF CACHE BOOL "Build whisper server" FORCE)

# Conditional GPU configuration
if(ENABLE_GPU)
    message(STATUS "GPU acceleration enabled")
    
    # Enable CUDA support for whisper.cpp
    set(GGML_CUDA ON CACHE BOOL "Enable CUDA support" FORCE)
    
    # Optional: Set specific CUDA architectures
    # set(CMAKE_CUDA_ARCHITECTURES "75;80;86" CACHE STRING "CUDA architectures" FORCE)

    # Enable CUDA language
    enable_language(CUDA)
else()
    message(STATUS "CPU-only build")
    set(GGML_CUDA OFF CACHE BOOL "Disable CUDA support" FORCE)
    set(GGML_BLAS ON CACHE BOOL "Enable OpenBLAS support" FORCE)
    set(GGML_BLAS_VENDOR OpenBLAS CACHE STRING "Vendor for GGML BLAS" FORCE)
endif()

# Include directories for WebSocket++ (header-only)
find_path(WEBSOCKETPP_INCLUDE_DIR websocketpp/config/asio_no_tls.hpp)
if(NOT WEBSOCKETPP_INCLUDE_DIR)
message(FATAL_ERROR "Could not find WebSocket++ headers. Please install websocket++ package.")
endif()

add_executable(srotalekh-backend ${SOURCES})
add_subdirectory(libraries/libdatachannel)
add_subdirectory(libraries/whisper.cpp)

target_link_libraries(srotalekh-backend PRIVATE whisper)
target_link_libraries(srotalekh-backend PRIVATE datachannel)
target_link_libraries(srotalekh-backend PRIVATE cpprestsdk::cpprest)
target_link_libraries(srotalekh-backend PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(srotalekh-backend PRIVATE ${WEBSOCKETPP_INCLUDE_DIR})
target_include_directories(srotalekh-backend PRIVATE include/)
target_include_directories(srotalekh-backend PRIVATE libraries/whisper.cpp/include)
target_include_directories(srotalekh-backend PRIVATE libraries/libdatachannel/include)