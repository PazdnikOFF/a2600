# Кросс-компиляция под aarch64 БЕЗ проприетарного Xilinx SDK — через Debian
# multiarch (crossbuild-essential-arm64 + arm64-пакеты Qt5/GStreamer/SQLite).
#
# Назначение: доказать архитектурную портируемость на aarch64 (та же архитектура,
# что у целевого Zynq UltraScale+) и ловить баги, которые x86 маскирует. Для
# НАСТОЯЩЕЙ прошивки платы используйте toolchain-aarch64-xilinx.cmake (PetaLinux
# sysroot); здесь sysroot — обычный Debian arm64, отличие от прибора = подмена
# sysroot, не кода.
#
# Рецепт образа (см. ~/a2600build/Dockerfile.arm64 на hermes):
#   dpkg --add-architecture arm64 && apt install \
#     crossbuild-essential-arm64 cmake ninja-build pkg-config \
#     qtbase5-dev-tools qtchooser \
#     qtbase5-dev:arm64 libqt5sql5-sqlite:arm64 libqt5printsupport5:arm64 \
#     libsqlite3-dev:arm64 \
#     libgstreamer1.0-dev:arm64 libgstreamer-plugins-base1.0-dev:arm64
#   export PKG_CONFIG_LIBDIR=/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/share/pkgconfig
#
# Сборка:
#   cmake -S app -B out-arm64 -G Ninja \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-aarch64-debian-multiarch.cmake \
#         -DCMAKE_BUILD_TYPE=Release
#   cmake --build out-arm64 --target endostation
# Запуск под эмуляцией: qemu-aarch64-static ./out-arm64/endostation
#
# ⚠️ AUTOMOC: нужен НАТИВНЫЙ (x86) moc из qtbase5-dev-tools (/usr/lib/qt5/bin/moc) —
# arm64-moc не запустится на хосте. Пакет ставит именно host-moc, поэтому работает.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Библиотеки/заголовки/пакеты ищем в arm64-sysroot, программы (moc/uic) — на хосте.
set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu /usr/lib/aarch64-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

list(APPEND CMAKE_PREFIX_PATH /usr/lib/aarch64-linux-gnu/cmake)
