cmake_minimum_required(VERSION 3.20)
project(tc-tdma LANGUAGES CXX)
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

add_executable(tc-tdma src/main.cpp
                       src/qdisc.cpp
                       src/tdma.cpp
                       src/beacon.cpp)

target_include_directories(tc-tdma PRIVATE ${docopt_SOURCE_DIR}
                                           ${LIBNL_INCLUDE_DIRS}
                                           ${PCAP_INCLUDE_DIRS})

target_link_libraries(tc-tdma PRIVATE ${PCAP_LIBRARIES}
                                        ${LIBNL_LIBRARIES}
                                        nl-route-3
                                        docopt_s)



