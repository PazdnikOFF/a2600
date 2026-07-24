# Кросс-компиляция под Zynq UltraScale+ (aarch64-xilinx-linux, PetaLinux/Vitis SDK).
#
# ⚠️ ТОЧНЫЙ ЦЕЛЕВОЙ ТУЛЧЕЙН ПРИБОРА (установлено из update/patch/libhal.so, 2026-07-24):
#   PetaLinux 2019.2 · GCC 8.2.0 · aarch64-xilinx-linux · GLIBCXX_3.4.21 · CXXABI_1.3.8.
#   (путь сборки libhal: /opt/petalinux/2019.2/sysroots/aarch64-xilinx-linux/…)
# СЛЕДСТВИЕ: бинарник, собранный Debian-кроссом (GCC 12, новый glibc), НА ПРИБОРЕ НЕ
# ЗАПУСТИТСЯ — glibc версионируется только вперёд. Для device-runnable сборки нужен
# ЛИБО этот PetaLinux 2019.2 SDK, ЛИБО sysroot прибора (вытащить /usr/lib,/lib с eMMC
# через serial-консоль). libhal.so — C-API (749 Hal_*, extern "C"), линкуется независимо
# от версии GCC; лежит в update/patch/libhal.so (aarch64, не стрипнут, с debug_info).
#
# Использование:
#   1) source /opt/sdk/environment-setup-aarch64-xilinx-linux   # из PetaLinux SDK
#   2) cmake -B build-arm \
#            -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-aarch64-xilinx.cmake \
#            -DCMAKE_BUILD_TYPE=Release
#   3) cmake --build build-arm -j
#
# Толчейн опирается на переменные окружения, которые ставит environment-setup-*
# (SDKTARGETSYSROOT, CC, CXX). Если собираешь без SDK — задай SDKTARGETSYSROOT вручную.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

if(DEFINED ENV{SDKTARGETSYSROOT})
    set(CMAKE_SYSROOT $ENV{SDKTARGETSYSROOT})
else()
    message(WARNING "SDKTARGETSYSROOT не задан — укажи sysroot платы вручную")
endif()

# Компиляторы из SDK (environment-setup-* уже экспортирует CC/CXX)
if(DEFINED ENV{CC})
    string(REGEX REPLACE " .*" "" _cc $ENV{CC})
    set(CMAKE_C_COMPILER ${_cc})
endif()
if(DEFINED ENV{CXX})
    string(REGEX REPLACE " .*" "" _cxx $ENV{CXX})
    set(CMAKE_CXX_COMPILER ${_cxx})
endif()

set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# pkg-config должен смотреть в sysroot
set(ENV{PKG_CONFIG_SYSROOT_DIR} ${CMAKE_SYSROOT})
set(ENV{PKG_CONFIG_LIBDIR} ${CMAKE_SYSROOT}/usr/lib/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig)
