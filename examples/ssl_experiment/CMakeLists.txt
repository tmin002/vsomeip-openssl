cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(bench_server src/bench_server.cpp)
target_link_libraries(bench_server PRIVATE vsomeip3 vsomeip3-cfg)

add_executable(bench_client src/bench_client.cpp)
target_link_libraries(bench_client PRIVATE vsomeip3 vsomeip3-cfg)

install(TARGETS bench_server bench_client RUNTIME DESTINATION bin)

