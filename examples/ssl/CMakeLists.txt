cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ssl_server server.cpp)
target_link_libraries(ssl_server PRIVATE vsomeip3 vsomeip3-cfg)

add_executable(ssl_client client.cpp)
target_link_libraries(ssl_client PRIVATE vsomeip3 vsomeip3-cfg)

install(TARGETS ssl_server ssl_client RUNTIME DESTINATION bin)

