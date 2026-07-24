#!/bin/sh
# Снятие манифеста окружения прибора (Endo X-2600) через serial-консоль.
# Запускать НА ПРИБОРЕ (root@…), вывод — текстовый, скопировать целиком и прислать.
# Сети на приборе нет, поэтому только stdout; крупные файлы — отдельно (USB/base64).
#
# Цель: узнать точный sysroot прибора (glibc/Qt/GStreamer/Xilinx-плагины), чтобы
# собрать device-runnable aarch64-бинарник endostation, совместимый по ABI.

echo "===== SYSTEM ====="
uname -a
cat /etc/os-release 2>/dev/null; cat /etc/*release 2>/dev/null
echo "petalinux: "; cat /etc/petalinux/version 2>/dev/null

echo "===== GLIBC / LIBSTDC++ (ABI, критично для запуска моей сборки) ====="
ls -l /lib/libc.so* /lib/aarch64-linux-gnu/libc.so* 2>/dev/null
( /lib/libc.so.6 2>/dev/null || /lib/aarch64-linux-gnu/libc.so.6 2>/dev/null ) | head -1
ls -l $(find / -name 'libstdc++.so*' 2>/dev/null | head -3) 2>/dev/null
strings $(find / -name 'libstdc++.so.6*' 2>/dev/null | head -1) 2>/dev/null | grep -E '^GLIBCXX_[0-9]' | sort -V | tail -3

echo "===== ЧТО НУЖНО X2000 (список зависимостей) ====="
ldd /home/root/X2000 2>/dev/null | sort
echo "--- версии Qt5 (по .so) ---"
ls -l $(find / -name 'libQt5Core.so.5*' 2>/dev/null | head) 2>/dev/null

echo "===== GSTREAMER (версия + Xilinx-плагины VCU/OMX) ====="
gst-launch-1.0 --version 2>/dev/null | head -1
gst-inspect-1.0 2>/dev/null | grep -iE 'omx|vcu|xlnx|zynq|v4l2' | head -20

echo "===== ВИДЕО-ТРАКТ (FPGA/камера) ====="
media-ctl -p 2>/dev/null | head -60
v4l2-ctl -d /dev/video0 --all 2>/dev/null | head -40
echo "--- switchvideoformat.sh ---"
cat $(find / -name 'switchvideoformat.sh' 2>/dev/null | head -1) 2>/dev/null

echo "===== МК-ПЛАТА (serial) ====="
ls -l /dev/tty* 2>/dev/null | grep -iE 'ttyPS|ttyUL|ttyS'
dmesg 2>/dev/null | grep -iE 'ttyPS|uart|serial|mcu|hmi' | head -20

echo "===== USB (можно ли таскать файлы флешкой) ====="
ls -l /dev/sd* 2>/dev/null; mount 2>/dev/null | grep -iE 'usb|sd[a-z]'

echo "===== DONE ====="
