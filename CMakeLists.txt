cmake_minimum_required(VERSION 3.1)
project(dpdk_tcp)

set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)
include_directories(network/include)
include_directories(queue/include)



set(link_libs ${LINK_LIBS} Threads::Threads dl rt m numa)
add_compile_options(-msse4)
file(GLOB tcp_src
        network/src/*.cc
        queue/src/*.cc)
add_library(tcp_lib ${tcp_src})

SET(EXAMPLES
        hello
        try_tcp
        try_epoll)
FOREACH(EXAMPLE ${EXAMPLES} )
add_executable(${EXAMPLE} test/${EXAMPLE}.cc)
target_link_libraries(${EXAMPLE} PRIVATE ${link_libs} tcp_lib
        -Wl,--whole-archive
        -lbsd)
ENDFOREACH(EXAMPLE)