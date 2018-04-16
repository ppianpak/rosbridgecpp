cmake_minimum_required(VERSION 3.0)
project(rosbridgecpp)

add_definitions(-std=c++11)

include_directories("${PROJECT_SOURCE_DIR}/Simple-WebSocket-Server")
add_subdirectory(Simple-WebSocket-Server)

add_executable(rosbridge_client_ws rosbridge_client_ws.cpp)
target_link_libraries(rosbridge_client_ws simple-websocket-server)
