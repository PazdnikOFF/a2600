# EndoStation

Реконструкция ПО эндоскопической стойки (референс — X-2000, Zynq UltraScale+ EV).
Архитектура и полное ТЗ: [../docs/ARCHITECTURE.md](../docs/ARCHITECTURE.md).

Текущий этап — **M1**: скелет Qt5-приложения с живым видео.

## Что уже есть

- Qt5-приложение (`endostation`), окно с виджетом видео и статус-строкой.
- `GstVideoPipeline` — захват через GStreamer:
  - на приборе: `v4l2src /dev/video0` (NV12) → RGB → appsink;
  - на десктопе (нет `/dev/video0`): `videotestsrc` — для отладки логики/UI.
- `hal::init()` — фасад над `libhal.so` (заглушка, если собрано без неё).
- CMake + toolchain-файл для кросс-сборки под `aarch64-xilinx-linux`.

## Сборка на десктопе (отладка)

Нужны Qt5 (Widgets), GStreamer 1.x с `gstreamer-app-1.0`/`gstreamer-video-1.0` и плагины (`videotestsrc`, `videoconvert`, `appsink`).

```bash
cd app
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/endostation      # покажет тестовый источник
```

## Кросс-сборка под прибор (Zynq US+ EV)

```bash
cd app
source /opt/sdk/environment-setup-aarch64-xilinx-linux   # PetaLinux/Vitis SDK
cmake -B build-arm \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-aarch64-xilinx.cmake \
      -DCMAKE_BUILD_TYPE=Release
cmake --build build-arm -j
```

Скопировать `build-arm/endostation` на прибор и запустить:

```bash
# сначала настроить видеотракт (пример 1280x960):
xmedia-ctl-nv12 -d /dev/media0 -V '"a00a0000.v_tpg":0 [fmt:VYYUYY8_1X24/1280x960 field:none]'
v4l2-ctl -d /dev/video0 --set-fmt-video width=1280,height=960,pixelformat=NV12

QT_QPA_PLATFORM=eglfs ./endostation
```

## До этого — «hello video» (M0, без нашего кода)

Проверить весь тракт железо→экран одной командой:

```bash
gst-launch-1.0 v4l2src device=/dev/video0 io-mode=dmabuf ! \
  video/x-raw,format=NV12,width=1280,height=960 ! \
  kmssink bus-id=fd4a0000.zynqmp-display
```

## Дальше (по дорожной карте)

- **M2** — production-рендер: kmssink на DRM-видеоплоскости + UI-оверлей по alpha.
- **M3** — снимок/запись через VCU (`omxh264enc`).
- **M4** — архив снимков + БД (SQLCipher) + пациенты.
- **M5** — обработка (OpenCV), 3A-диммирование (`libhal`).
- **M6** — DICOM Secondary Capture / PACS (DCMTK).

## Структура

```
app/
├── CMakeLists.txt
├── cmake/toolchain-aarch64-xilinx.cmake
└── src/
    ├── main.cpp
    ├── app/        Application — инициализация, выбор источника
    ├── ui/         MainWindow — каркас окна осмотра
    ├── video/      GstVideoPipeline, VideoWidget
    ├── hal/        Hal — фасад над libhal
    └── config/     (чтение ini-конфигов, далее)
```
