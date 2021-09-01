cmake_minimum_required(VERSION 3.1)
project(dpdk_tcp)

set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)
include_directories(network)
include_directories(middleware)

#INCLUDE_DIRECTORIES($ENV{RTE_SDK}/build/network)
#LINK_DIRECTORIES($ENV{RTE_SDK}/build/lib)

set(link_libs ${LINK_LIBS} Threads::Threads dl rt m numa )
#set(dpdk_libs "-Wl,--whole-archive -ldpdk -Wl,--no-whole-archive")
add_compile_options(-msse4)
file(GLOB tcp_src
        src/*.cc)
add_library(tcp_lib ${tcp_src})

add_executable(hello test/hello.cc)
target_link_libraries(hello PRIVATE ${link_libs} ${dpdk_libs} tcp_lib)

add_executable(try_tcp test/try_tcp.cc)
target_link_libraries(try_tcp PRIVATE ${link_libs} ${dpdk_libs} tcp_lib)