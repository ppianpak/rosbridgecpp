cmake_minimum_required(VERSION 3.0)
project(rosbridgecpp)

option(DEBUG "Print debug messages" ON)

add_definitions(-std=c++11)

if(DEBUG)
  add_definitions(-DDEBUG)
endif()

include_directories("${PROJECT_SOURCE_DIR}/Simple-WebSocket-Server")
add_subdirectory(Simple-WebSocket-Server)

include_directories("${PROJECT_SOURCE_DIR}/rapidjson/include")

add_executable(rosbridge_ws_client rosbridge_ws_client.cpp)
target_link_libraries(rosbridge_ws_client simple-websocket-server)
