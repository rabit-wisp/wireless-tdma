set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm_cortex-a7_neon-vfpv4)

set(TOOLCHAIN_DIR "${CMAKE_CURRENT_LIST_DIR}/../../arm-buildroot-linux-musleabihf_sdk-buildroot/")

set(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/bin/arm-linux-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/bin/arm-linux-g++)
set(CMAKE_AR ${TOOLCHAIN_DIR}/bin/arm-linux-ar)
set(CMAKE_RANLIB ${TOOLCHAIN_DIR}/bin/arm-linux-ranlib)

set(CMAKE_SYSROOT ${TOOLCHAIN_DIR}/arm-buildroot-linux-musleabihf/sysroot/)
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -s")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -s")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")

set(CMAKE_EXE_LINKER_FLAGS "-static")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")

set(ENV{PKG_CONFIG_PATH} "")
set(ENV{PKG_CONFIG_LIBDIR} "${CMAKE_SYSROOT}/usr/lib/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${CMAKE_SYSROOT}")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)