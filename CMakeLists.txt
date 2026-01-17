cmake_minimum_required(VERSION 3.20)
project(tdma-test LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/binaries/)

include(FetchContent)


# Fetch docopt
FetchContent_Declare(
  docopt
  GIT_REPOSITORY https://github.com/docopt/docopt.cpp.git
  GIT_SHALLOW TRUE
  GIT_TAG v0.6.3
)
FetchContent_MakeAvailable(docopt)


set(ZLIB_USE_STATIC_LIBS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZLIB REQUIRED zlib)
pkg_check_modules(ZMQ REQUIRED libzmq)
pkg_check_modules(PCAP REQUIRED libpcap)
pkg_check_modules(LIBNL REQUIRED libnl-3.0 libnl-genl-3.0 libnl-route-3.0)

add_executable(tdma-temp src/temp.cpp)
target_include_directories(tdma-temp PRIVATE
                                     ${docopt_SOURCE_DIR}
                                     ${LIBNL_INCLUDE_DIRS}
                                     ${PCAP_INCLUDE_DIRS})

target_link_libraries(tdma-temp PRIVATE 
                                        ${PCAP_LIBRARIES}
                                        ${LIBNL_LIBRARIES}
                                        nl-route-3
                                        docopt_s)




add_executable(tdma-test src/main.cpp)
target_include_directories(tdma-test PRIVATE
                                     ${docopt_SOURCE_DIR}
                                     ${LIBNL_INCLUDE_DIRS}
                                     ${PCAP_INCLUDE_DIRS})

target_link_libraries(tdma-test PRIVATE 
                                        ${PCAP_LIBRARIES}
                                        ${LIBNL_LIBRARIES}
                                        nl-route-3
                                        docopt_s)



