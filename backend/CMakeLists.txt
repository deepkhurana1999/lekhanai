cmake_minimum_required(VERSION 3.28.3)
project(hikki-backend)
set(CMAKE_CXX_STANDARD 23)

file(GLOB SOURCES src/*.cpp src/**/*.cpp)
find_package(cpprestsdk REQUIRED)
find_package(Threads REQUIRED)


# Include directories for WebSocket++ (header-only)
find_path(WEBSOCKETPP_INCLUDE_DIR websocketpp/config/asio_no_tls.hpp)
if(NOT WEBSOCKETPP_INCLUDE_DIR)
message(FATAL_ERROR "Could not find WebSocket++ headers. Please install websocket++ package.")
endif()

add_executable(hikki-backend ${SOURCES})
add_subdirectory(libraries/libdatachannel)
add_subdirectory(libraries/whisper.cpp)
add_subdirectory(libraries/json)

target_link_libraries(hikki-backend PRIVATE whisper)
target_link_libraries(hikki-backend PRIVATE datachannel)
target_link_libraries(hikki-backend PRIVATE cpprestsdk::cpprest)
target_link_libraries(hikki-backend PRIVATE json)
target_include_directories(hikki-backend PRIVATE ${WEBSOCKETPP_INCLUDE_DIR})
target_include_directories(hikki-backend PRIVATE include/)
target_include_directories(hikki-backend PRIVATE libraries/whisper.cpp/include)
target_include_directories(hikki-backend PRIVATE libraries/libdatachannel/include)
target_include_directories(hikki-backend PRIVATE libraries/json/nlohmann/include)