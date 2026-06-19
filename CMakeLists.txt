cmake_minimum_required(VERSION 3.20)
project(MavlinkTelemetryVisualizer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Export compile commands for IDEs (clangd) to find includes
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-deprecated-declarations")
set(CMAKE_AUTOMOC_PATH_PREFIX ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts Network)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/mavlink_c_library_v2
    /usr/include/x86_64-linux-gnu/qt6
    ${CMAKE_SOURCE_DIR}/include/transport
    ${CMAKE_SOURCE_DIR}/include/history
    ${CMAKE_SOURCE_DIR}/include/ui
    ${CMAKE_SOURCE_DIR}/include/parser
    ${CMAKE_SOURCE_DIR}/include/model
)

set(HEADERS
    include/common/telemetry_types.h
    include/transport/udp_transport.h
    include/parser/mavlink_parser.h
    include/model/telemetry_model.h
    include/history/history_manager.h
    include/ui/main_window.h
    include/ui/chart_widget.h
)

add_executable(mav_visualizer
    ${HEADERS}
    src/main.cpp
    src/common/config.cpp
    src/transport/udp_transport.cpp
    src/parser/mavlink_parser.cpp
    src/model/telemetry_model.cpp
    src/history/history_manager.cpp
    src/ui/main_window.cpp
    src/ui/chart_widget.cpp
)

target_link_libraries(mav_visualizer PRIVATE 
Qt6::Core 
Qt6::Widgets 
Qt6::Charts 
Qt6::Network
)

# Copy config.json to build directory
add_custom_command(TARGET mav_visualizer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/config.json
    $<TARGET_FILE_DIR:mav_visualizer>
)