# Endo X-2600 — состояние проекта и методология (READ FIRST)

> Единый документ для продолжения работы после `/clear`. Прочитай его целиком —
> он заменяет длинную историю чата. Детали архитектуры: `docs/ARCHITECTURE.md`,
> панель 8″: `docs/HMI_PANEL.md`, **полный аудит покрытия + план: `docs/ROADMAP.md`**.
>
> Аудит (сверка всех 491 классов референса): реализовано ~52 класса частично,
> сквозное ядро обработки изображения + БД + отчёты + вспом. конфиги готовы и
> проверены (33 self-test, все PASS). Пробелы и фазы — в ROADMAP.md, позиция — §10.

## 0. Цель

Своё ПО эндоскопической стойки, эквивалентное референсу **SonoScape X-2600**
(ребренд «РуСкейн»/PyCkeun), для идентичного железа. Реимплементация (не копия
бинарников) с максимальной верностью оригиналу, чтобы потом дорабатывать.
Применение: R&D/стенд. Язык — **C++/Qt5** (как оригинал).

## 1. МЕТОДОЛОГИЯ (обязательна для каждой новой функции)

1. **Реверс оригинала** — как это делает `X2000`: `tools/revcalls.sh <mangled>`
   (последовательность вызовов), дизасм регистров (`objdump -d --start/--stop`),
   реальные имена классов/методов.
2. **Поиск файлов** — все конфиги/ассеты в дереве прошивки, что оригинал читает.
3. **Повтор кодом** — C++/Qt5 с ТЕМИ ЖЕ именами классов/методов; значения НЕ
   хардкодить, читать из файлов теми же механизмами (QSettings и т.п.).

Не перескакивать; после каждого шага — проверка рендером/self-test (`ui_preview`).

## 2. Платформа (подтверждено реверсом)

- SoC **Xilinx Zynq UltraScale+ EV** (Cortex-A53 + Mali-400 + VCU H.264/265).
- ОС PetaLinux/Yocto, тулчейн `aarch64-xilinx-linux` GCC 8.2, sysroot
  `/opt/sdk/sysroots/aarch64-xilinx-linux`.
- Видео: V4L2 `/dev/video0` NV12 (Xilinx PL, `a00a0000.v_tpg`, media-ctl);
  дисплей `fd4a0000.zynqmp-display` (kmssink, dual-plane alpha); кодек VCU (omx).
- Стек: Qt5, GStreamer, OpenCV, DCMTK, Boost 1.74, **SQLCipher**, ime_pinyin.
- Двухмониторность: осн. монитор (Qt) + сенс. панель 8″ (отдельный Cortex-M).

## 3. Инструменты реверса (на macOS)

- `objdump` (Apple LLVM) читает aarch64 ELF. `c++filt` НЕ демманглит Itanium —
  парсер длино-префиксов (в скриптах).
- **`tools/revcalls.sh <mangled_sym>`** (в репо) — последовательность bl-вызовов функции.
- Регистры: `objdump -d --start-address --stop-address` + греп `mov/movk x1`, `w2`.
- FAT-образы (lcd_upd): парсинг Python или mount.
- Референс: `update/root/` (= `/home/root` на устройстве). Бинарник `update/root/X2000`
  (not stripped, но DWARF пустой → логику не достать, только имена+типы).

## 4. Сборка и проверка

Qt5 — brew `/opt/homebrew/opt/qt@5` (arm64). Хост arm64 → нужен
`-DCMAKE_OSX_ARCHITECTURES=arm64` (иначе линк x86_64 падает).

```bash
cd app
QT5=/opt/homebrew/opt/qt@5
SCR=<scratchpad>
cmake -B "$SCR/uibuild" -DCMAKE_PREFIX_PATH="$QT5" -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_BUILD_TYPE=Debug
cmake --build "$SCR/uibuild" --target ui_preview -j
ER=<repo>/update/root
# режимы ui_preview:
ENDO_ROOT=$ER QT_QPA_PLATFORM=offscreen "$SCR/uibuild/ui_preview" desktop out.png [SCOPE]   # рендер экрана (+эндоскоп)
ENDO_ROOT=$ER ENDO_EXAM=<dir> ... ui_preview desktop out.png                                 # +список снимков
QT_QPA_PLATFORM=offscreen ui_preview db out.db     # self-test БД
ENDO_ROOT=$ER ui_preview alg                        # self-test гамма+CCM
ENDO_ROOT=$ER ui_preview plreg [SENSOR]             # self-test PL-регистров парам. изобр. + BrightEQ LUT
ENDO_ROOT=<tmp> ui_preview dccu                      # self-test KDccuParam (пишет dccuparam.ini — брать tmp-root!)
ENDO_ROOT=$ER ui_preview filt                        # self-test VIST-матрицы + Denoise LUT
ENDO_ROOT=$ER ui_preview dicom                        # self-test WorklistFieldMap.xml + БД tb_Dcm*
ENDO_ROOT=$ER ui_preview report [out.html]            # self-test шаблон→генератор HTML + БД tb_Report
ENDO_ROOT=$ER ui_preview thesaurus                    # self-test тезауруса (словари диагнозов + отчёт)
ENDO_ROOT=<tmp> ui_preview account [system.ini]       # self-test KAccount(вход/MD5/блок.)+KSystemSet
ENDO_ROOT=$ER ui_preview userset                      # self-test KUserSet (полный парсинг osd.ini)
ENDO_ROOT=$ER ui_preview coldlight                    # self-test источника света (VLS-режимы + LED-парам.)
ENDO_ROOT=$ER ui_preview version                      # self-test матрицы совместимости версий
ENDO_ROOT=$ER ui_preview statistic                    # self-test спеки событий лога
ENDO_ROOT=$ER ui_preview sysstatus                    # self-test KSystemStatus (состояние + сигналы)
ui_preview quickinput [out.db]                        # self-test словарей автозаполнения (tb_QuickInput*)
ENDO_ROOT=$ER ui_preview project                      # self-test продуктовой конфигурации (project/product.ini)
ENDO_ROOT=$ER ui_preview style                        # self-test брендов/стилей (stylelist.ini)
ui_preview examcfg                                    # self-test конфига колонок списка осмотров
ui_preview exam                                       # self-test полного CRUD осмотров (tb_ExamList)
ui_preview filebackup                                 # self-test файлового слоя (copy/delete/size/USB)
ENDO_ROOT=$ER ui_preview videoset                     # self-test оркестрации (Set*Level→параметр+регистр)
ui_preview dsreal                                     # self-test реального датасорса отчёта (БД→документ)
ENDO_ROOT=$ER ui_preview dsdemo                       # self-test демо-датасорса отчёта (превью+DemoImage)
ENDO_ROOT=$ER ui_preview videocal                     # self-test калибровки видео (диапазоны центра + rect roundtrip)
ui_preview update                                     # self-test апдейт-пайплайна (манифест update.ini + решение «что обновлять»)
ENDO_ROOT=$ER ui_preview templetcfg                   # self-test каталога шаблонов отчёта (TempletInfo.xml + департаменты)
ui_preview reportdb                                   # self-test постраничного БД-доступа к отчётам (пагинация+LIKE-запрос)
ui_preview savefile                                   # self-test нумерации файлов снимков/видео (FormatFlowNumber+разбор имён)
ENDO_ROOT=$ER ui_preview osdset                       # self-test OSD-конфига (кнопки→функции osd.ini + режимы)
ui_preview dbservice                                  # self-test сервиса БД (PRAGMA-окружение + бэкап/восстановление .bak)
ui_preview dispparam                                  # self-test валидности элементов отчёта (данные→показ, ref-гейт)
ENDO_ROOT=$ER ui_preview endoinfo                     # self-test конфига сервера выгрузки инфо об эндоскопе (endoinfoserver.ini)
ENDO_ROOT=$ER ui_preview remoteswitch                 # self-test пульта/ножного переключателя + IHb (user.ini)
ENDO_ROOT=$ER ui_preview dcmfmt                       # self-test структуры DICOM-датасета (Mpps*DatasetFormat.xml)
ui_preview pattime                                    # self-test конвертеров дат/времени пациента (возраст, DICOM/БД-формат)
```

- `ui_preview` — Qt-only цель (Core/Gui/Widgets/Sql), собирается и проверяется на Mac.
- `endostation` — полное приложение, требует **GStreamer** (нет на Mac) → собирается
  только на устройстве/в sysroot. GStreamer-зависимые классы там.
- Рендеры смотреть Read-ом PNG. Отладочный вывод — `qInfo/qWarning` в лог.

## 5. Структура проекта (`app/src/`, имена = оригинал)

```
app/
├── CMakeLists.txt            # 2 цели: ui_preview (Qt-only), endostation (+GStreamer)
├── cmake/toolchain-aarch64-xilinx.cmake
└── src/
    ├── main.cpp, app/Application       # bootstrap
    ├── ctrl/KMainCtrlThread            # управляющий поток (Init-последовательность)
    ├── ctrl/KPlControl                 # регистры FPGA (гамма/CCM/ColorEnh/BrightEQ/AEC-AGC/AWB/VIST/Denoise)
    ├── ctrl/KDccuParam                 # параметры DCCU (dccuparam.ini, AEC/AGC/AWB/RB/Gamma/Zoom)
    ├── ctrl/KColdLightConfig           # источник холодного света (VLS-режимы + LED-параметры)
    ├── hal/KMemDevice                  # низкоуровневый доступ к PL (/dev/mem mmap + read-back), реф. kmemdevice.cpp
    ├── video/KVideoProxy               # камера/FPGA, InitCamera, снимок, RBC, ApplyImageParams
    ├── video/KViewSoftEndo             # вьювер видео
    ├── video/KVideoParam               # держатель видеопараметров
    ├── video/KVideoSet                 # оркестрация Set*Level → KVideoParam + PL-регистры
    ├── video/KSaveVideoFile            # запись MP4 (VCU), миниатюры, лимит
    ├── ui/KUIDesktop                   # главный экран (конфиг-вёрстка)
    ├── ui/KDisplayOption               # layout из ini по монитору/эндоскопу
    ├── ui/KImgList                     # список снимков (QTableWidget)
    ├── ui/Theme                        # загрузка style.qss из прошивки
    ├── sys/KSystem                     # пути, GetSystemResolution
    ├── sys/KUserSet (+_KUserConf)      # osd.ini
    ├── sys/KAccount                    # вход/роли/MD5-пароли/блокировка (system.ini)
    ├── sys/KSystemSet                  # системные настройки (Common/*, Account/*)
    ├── sys/KUpdateConf                 # матрица совместимости версий (matchedversion.ini)
    ├── sys/KVersionConfig              # установленные версии (data/protected/version.ini) + проверка совместимости
    ├── sys/KStatisticConfig            # спека событий лога (statistic.ini: time_/dcnt_/info_)
    ├── sys/KSystemStatus               # центральный синглтон состояния (freeze/VlsMode/… + сигналы)
    ├── sys/KProjectSet                 # продуктовая конфигурация (project.ini + per-модель product.ini)
    ├── sys/KStyleConfig                # бренды/стили (stylelist.ini: SonoScapeCN/PyCkeun/…)
    ├── endo/KSoftEndoParam             # video.ini (per эндоскоп)
    ├── alg/AlgParaManager              # гамма-LUT/CCM/ColEnh из videoconf
    ├── db/KEntityManage                # БД пациент/осмотр (Qt5::Sql/SQLCipher)
    ├── db/KEntityQuickInput            # словари автозаполнения (tb_QuickInput*: value/Count/date)
    ├── db/KEntityExam                  # полный CRUD осмотров (tb_ExamList + пагинация)
    ├── db/KFileBackup                  # файловый слой: копирование/удаление/размер/USB
    ├── db/KExamListConfigHandler       # видимость колонок списка осмотров + экспорт
    ├── dicom/KDicomFieldMap            # парсер XML-маппинга датасет→колонки БД
    ├── dicom/KEntityDicom              # БД DICOM (tb_DcmWorklist/tb_DcmStore)
    ├── dicom/KDicomInterface           # сеть DCMTK (device-only HAVE_DCMTK): STORE/ECHO/worklist/MPPS/commit
    ├── report/KReportTemplate          # дерево шаблона отчёта из XML (TempletInfo/SubContent)
    ├── report/KReportDataSource        # источник данных (PATIENT/PERIPHERAL) по DataSrc
    ├── report/KDocumentGenerator       # генерация документа отчёта (HTML) по шаблону+данным
    ├── report/KEntityReport            # БД отчётов (tb_Report)
    ├── report/KThesaurusOpt            # тезаурус шаблонов диагнозов (thesaurus/<lang>/<Scope>.xml)
    ├── report/KRTDataSourceReal        # реальный источник данных отчёта (БД→датасорс)
    ├── report/KRTDataSourceDemo        # демо-источник (образец+DemoImage) для превью шаблонов
    └── hal/Hal                         # фасад libhal
```

Ресурсы НЕ копируются — читаются из прошивки (`ENDO_ROOT`=update/root; устройство /home/root).
Бренд РуСкейн = `PyCkeun`, модель `X-2600` (из stylelist.ini / display/project.ini).

## 6. СТАТУС по подсистемам

Легенда: ✅ реализовано+проверено · ⚙️ реализовано (проверка на устройстве) · ⬜ TODO

| Подсистема | Классы | Статус |
|---|---|---|
| Главный экран + конфиг-вёрстка | KUIDesktop, KDisplayOption, KSystem, Theme | ✅ рендер |
| Видеотракт (захват) | KVideoProxy (InitSensorRegs, StartCapture V4L2/GStreamer) | ⚙️ |
| Вьювер | KViewSoftEndo | ✅ рендер |
| FPGA-регистры | KPlControl (Write/ReadValueToPL, SetGammaLut, SetCCM0Matrix) | ⚙️ |
| Управление | KMainCtrlThread (Init 1:1, CameraButtonAct) | ⚙️ |
| Параметры: osd.ini→держатель→OSD | KUserSet(_KUserConf)→KVideoParam→KUIDesktop | ✅ рендер |
| Эндоскоп/раскладка | KSoftEndoParam (video.ini videoSize→IMG-layout) | ✅ рендер |
| Снимок/запись (VCU) | KSaveVideoFile, KVideoProxy::ImageSavePreset | ⚙️ |
| Архив снимков | KImgList | ✅ рендер |
| БД пациент/осмотр | KEntityManage (tb_Patient/tb_Exam) | ✅ self-test |
| DICOM БД (worklist/очередь/study/series) | KEntityDicom (tb_DcmWorklist/Store/Study/Series), KDicomFieldMap | ✅ self-test (`dicom`) |
| DICOM сеть (STORE/ECHO/worklist/MPPS) | KDicomInterface + KStore/Worklist/Commit/MppsScu | ⬜ device-only (DCMTK, HAVE_DCMTK) |
| Отчёты (шаблон→документ) | KReportTemplate, KReportDataSource, KDocumentGenerator, KEntityReport | ✅ self-test (`report`) |
| Тезаурус диагнозов | KThesaurusOpt (thesaurus/<lang>/<Scope>.xml → автозаполнение отчёта) | ✅ self-test (`thesaurus`) |
| Аккаунты/вход | KAccount (роли/MD5/блокировка) | ✅ self-test (`account`) |
| Системные настройки | KSystemSet (Common/*, Account/*) | ✅ self-test (`account`) |
| Обработка гамма+CCM+ColEnh→FPGA | AlgParaManager + KPlControl | ✅ self-test |
| Параметры изображения→PL | KPlControl (ColorEnh/ImageEnh/Tone/BrightEQ/AEC-AGC/AWB/AwbCut) | ✅ self-test (`plreg`) |
| BrightEQ LUT→PL | KPlControl::SetBrightEQLut + AlgParaManager::LoadBrightEqPara | ✅ self-test (`plreg`, 530 записей) |
| AEC/AGC (камера-I2C + PL) | KVideoProxy::SetAEC/AGCValue→SetCameraAEC/AGC (I2C) / SetEndoAEC/AGC (PL) | ⚙️ (реверс 1:1, проверка на устройстве) |
| Параметры DCCU | KDccuParam (dccuparam.ini, ключи 1:1) | ✅ self-test (`dccu`, roundtrip) |
| VIST/SFI-матрица | KPlControl::SetVistMatrix/Switch + AlgParaManager::LoadVistMatrix | ✅ self-test (`filt`) |
| Шумоподавление | KPlControl::SetDenoiseLevel/SetDenoiseLut + AlgParaManager::LoadDenoisePara | ✅ self-test (`filt`, 1293 записи) |
| Режим WL/EWL/SFI/VIST + AWB per режим | KVideoProxy::SetOperationMode, AlgParaManager::LoadAwbGains | ⚙️ (реверс, конфиги по режимам) |
| Col RBC | KVideoProxy::RBCValueSet→KVideoParam гейны→PL (SetColorR/B/C) | ✅ self-test |

## 7. Ключевые факты реверса (справочник)

**АРХИТЕКТУРА доступа к железу (подтверждено реверсом libhal.so + kmemdevice.cpp):**
- Запись PL-регистра: `KPlControl::WriteValueToPL` → `KMemDevice::WriteDevRegister` →
  `WriteRegister(fd,addr,val)`: `mmap` страницы `/dev/mem` по `addr&~(pg-1)`, запись 32-бит,
  **чтение обратно для верификации** (Ok/Fail), `munmap`. fd `/dev/mem` — лениво (OPenDevice).
  ISP/видео-тракт библиотеку НЕ использует — только `/dev/mem`. Наш KMemDevice повторяет 1:1.
- **libhal.so** (`update/patch/libhal.so`, НЕ стрипнут, 4302 симв., debug_info!) — HAL всего
  семейства приборов (УЗИ/ECG/ATC3DG-трекер и т.п.). Категории Hal_*: Power/Usb/Pim/Pcie/Ecg/
  Bt/Wifi/Dvd/Finger/Lcd/принтер/Net/Iot. Для видео есть лишь `Hal_Card*Iris*` (диафрагма) и
  `Hal_*Display*`/`Hal_Change_Video_PN_*` (внешний монитор/PAL-NTSC). Gamma/CCM/AWB/Denoise/VIST —
  библиотеки НЕТ, только PL-регистры. Вывод: реимплементация через KMemDevice/dev/mem — верна.
- **KPlControl — 72 метода** (полный register-API оригинала). Реализовано ~47: Gamma(LUT/Enable),
  CCM0/CCM1(+Matrix), ColorEnh, ImageEnh, Tone(R/B/C), BrightEQ(Enable/Lut), AEC/AGC, AWB(+Cut),
  VIST(Switch/Matrix), Denoise(Level/Lut), Zoom, CHb, SensorR/G/BLut, RbcLut, KneeLut, IrisTable,
  FreezeStatus, VideoDisplay, FreezeScaler(In/Out/Ratio), CutPara, RealtimeVideoState, ApmAreaDisplay,
  VideoTest, GetFpga1/2/3Version, GetFpga2System, ReadAWBValue, StartAWB, ReadIrisValue, SetAuroraOffset,
  SetVideoCaptureArea, ReadBrightnessHistogramValue, SetCameraIrisType, **SetVideoArea(→resize)**.
  ГРАНИЦА: остаток (~25) — НЕ чистые PL-регистры, требуют устройства/доп. реверса, не гадать:
  • Геометрия маски поля зрения: CornerCutWay → AlgParaManager::SetCutCornerPara → SetRoundPara/
    SetOctagonPara (растеризация круга/октагона с плав. точкой, маска 1080 рег. 0xa18c8000) — off-device не верифицируется.
  • FPGA-I2C к сенсору: SetVideoCentorPoint (центрирование через 0xa0048010/70 строб-последовательность).
  • Runtime-зависимые: SetEndoIrisType (GetEndoScope()->GetSensorType/GetEndoType).
  • Tail-call/неясные: PLInit, GlassType, VLS(→SetVlsMode), Demoire, Lens/SetLensSize, SetEnhanceSize,
    SetFreezeVideoLoc, AuroraTxReset, InitColorEnhPara.

**Регистры PL (FPGA), через KPlControl → KMemDevice /dev/mem:**
- Init сенсора (KVideoProxy::InitCamera): регион `0xa004xxxx` (0x8070/0x8074=FPGA-I2C
  data/trigger, 0xa020/0xa030=управление). 17 операций (часть — цикл по таблице).
- Гамма (SetGammaLut): база `0xa1830000`, R/G/B на `+0x000/+0x800/+0x1000`, шаг 4Б.
- Gamma enable (SetGammaEnable): `0xa1830000`. Zoom (SetZoomValue): `0xa18d0004`.
- CCM0 (SetCCM0Matrix): регион `0xa1860000`, enable `0xa1860014`=0x14.
- CCM1 (SetCCM1/Matrix): регион `0xa1880000`, enable `0xa1880000`, коэф. парами `0xa1880004..`.
- CHb (SetChbStatus, усиление гемоглобина): `status==0`→выкл `0xa1900008=0`; иначе→вкл `=1`
  + запись CHb-значения в `0xa1900018`. Значение = **4-е** из `CHb/<SENSOR>_<res>.txt` (4 hex,
  реф. LoadCHBPara → AlgParaManager+0x7a3c). (Ранее ошибочно описан как строб — исправлено.)
- Версии FPGA (GetFpga1Version): чтение `0xa004a044` (Fpga2/3 — соседние).
- Freeze (SetFreezeStatus): `0xa180002c`. VideoDisplay: `0xa0080028`.
- Freeze scaler (SetFreezeScalerIn/Out/Ratio): `0xa191000c`/`0010`/`0008` = a|(b<<16).
- CutPara (SetCutPara): `0xa1860018` = b|(a<<16).
- Sensor LUT (SetSensorR/G/BLut): `0xa1820800`/`0xa1821000`/`0xa1821800`, пары значений
  (1024/канал → 512 записей); конфиг `sensor_lut/<SENSOR>_<res>/sensor_lut_{r,g,b}.txt`.
- Iris table (SetIrisTable): регион `0xa18a8000`, **8 значений на регистр** — (data[i]>>shift) в
  ниббл i*4 (значения 1/3/7 = план размера апертуры, shift 0..2); 8040 значений → 1005 записей.
  Конфиг `IRIS/<SENSOR>_<res>[_<SCOPE>].txt`. APM area display: `0xa18a0008`.
- Corner-cut маска (SetCornerCutWay): регион `0xa18c8000..90e0` (из AlgParaManager::SetCutCornerPara).
- RBC LUT (SetRbcLut): 3 канала в регион `0xa1878000` — Hb→`0xa1878200`, Hr→`0xa1878100`,
  S→`0xa1878000` (31 значение/канал, шаг 4); конфиг `rbc_lut/<SENSOR>_<res>/colrbc_{Hb,Hr,S}.txt`.
- RealtimeVideoState (SetRealtimeVideoState): `0xa0080024`. ApmAreaDisplay: `0xa18a0008`.
- VideoTest (тест-картинка FPGA): `0xa004a040` = mode<<2.
- Knee LUT (SetKneeLut): значения 10-бит парами → 3 банка `0xa1930800`/`0xa1931000`/`0xa1931800`
  (одно значение во все три), финализация `0xa1930000 |= 0x2`; конфиг `Knee/<SENSOR>_KneeLut[_<SCOPE>].txt`
  (hex, 1024). Перед записью — ожидание готовности (poll бита 2 в `0xa1930000`).
- Чтения версий/статуса FPGA: GetFpga2Version `0xa1000000`, GetFpga3Version `0xa0060000`,
  GetFpga2System `0xa1000008`, ReadAWBValue `0xa1840014` (r=(v>>16)&0xffff, b=v&0x1ffff).
- ReadIrisValue: `0xa18a0004` (младший байт). SetAuroraOffset: `0xa004a02c` = a|(b<<8).
- SetVideoCaptureArea: `0xa18d0008`, точка знак-величиной enc(v)=((|v|*2)|(v<0?0x100:0))&0x1ff, y<<16.
- ReadBrightnessHistogramValue: триггер `0xa18a0010=1`, 256 бинов из `0xa18a9000` (2 бина/слово).
- SetCameraIrisType: `0xa18a0000`, кодировка type 0→0x530/1→0x431/2→(2|(subtype==5?0x100:0x200)|0x30).
  SetEndoIrisType — то же поле, но индекс зависит от GetEndoScope()->GetSensorType/GetEndoType (device).
- ColorEnh (SetColorEnhParam): enable `0xa18f0008`, значение `0xa18f0024` (из colenh_level.txt).
- ImageEnh (SetImageEnhValue): `0xa1850058` (из ImgEnh/level_*.txt).
- RBC-тон (SetColorC/R/B, SetToneValue): `0xa1870000`=C(тон), `+0x4`=R-гейн, `+0x8`=B-гейн.
- BrightEQ enable (SetBrightEQEnalbe): `0xa1950000`. LUT (SetBrightEQLut): `0xa1950004..004c`
  (18 записей, пары 15-бит — gaussian_filter_hex.txt, 36 знач.) + `0xa1958000..8800`
  (512 записей, пары 12-бит — lumaGainLut_<disable/low/middle/high>_hex.txt, ~1024 знач.).
  Индекс LUT = clamp(level,1,3)-1 (level 1/2/3 → disable/low/middle; high не выбирается).
  Конфиги: `videoconf/Bright_EQ/<SENSOR>/`.
- AEC+AGC (SetAECAndAGCValue): `0xa0048020` = AEC[15:0] | AGC[31:16].
- AWB гейны (SetAWBValue): `0xa184000c` = B[16:0] | R[31:16]; строб `0xa100019c` 1→(10мкс)→0.
- AwbCut (SetAwbCut): `0xa1840018` = low[15:0] | high[31:16].
- AEC/AGC к сенсору (SetCameraAEC/AGCValue): через FPGA-I2C `0xa0048074`(data)/`0xa0048070`(trig=0x1014),
  usleep(200) между шагами. AEC: младший байт `|0xc00`, старший `|0xd00|бит31`. AGC:
  младший `|0xa00`, старшие 3 бита `|0xb00|бит31`. Ветвление KVideoProxy::SetAEC/AGCValue
  по [this+0x24]: ==2 → камера-I2C; иначе → PL AE-блок `0xa0048020` (SetEndoAEC/AGC→SetAECAndAGCValue).
  Кэш дедупа: AEC@[+0x30], AGC@[+0x2c]; retry-счётчик до 0xbc.
- **VIST/SFI** (SetVistSwitch/SetVistMatrix): switch `0xa18e0000`=en + `0xa1840000`=!en;
  матрица (9 знач.) — пары → `0xa18e0004..0010`, хвост `0xa18e0014`. GetVistValue: режим из
  GetSystemStatus[0x3c] (2=SFI→конфиг offset 0x44, 3=VIST→0x20). Конфиг: `Vist/V1/<MODE>/<SENSOR>_<res>.txt`.
- **Denoise** (SetDenoiseLevel/SetDenoiseLut): уровень → `0xa1940008`. LUT регион `0xa1941000`:
  4 dpc → `0xa194x010`; kernelG(41) → `0xa1941600` (4 банка +0x1000); kernelRB(25) → `0xa1941500`;
  Lut(256) → `0xa1941100`. Конфиг: `Denoise/<MODE>/<SENSOR>_Denoise[_<SCOPE>].ini`
  (секции [denoiseLevel_N]: dpc_thd_t1/t2, kernelG, kernelRB, Lut_0..15).
- **Awb per режим**: `Awb/V1/<MODE>/<SENSOR>_<res>[_<SCOPE>].txt` (6 знач.) → SetAWBValue/SetAwbCut.
- **Режимы WL/EWL/SFI/VIST** = поддиректории videoconf (Vist/Denoise/Awb); enum 0/1/2/3.
- **KDccuParam** (`presetdata/syspreset/dccuparam.ini`, генерится в рантайме): ключи 1:1 —
  `AEC/Control|AutoAEC|Min|Max|GradpUp|GradpDown|Gradthresh*|ManualOV2740|ManualOV9734`,
  `AGC/Control|Manual|AutoAGC|Min|Max|Gradp*|Gradthresh*|3AMaxAGC`,
  `AWB/IsCorrectionOn|IsAssessOn|Up|Down`, `RB/IsOpen|Hb|Hr|Satiation`,
  `CCM/IsOpen|Matrix`, `ElectronicFilter/IsOpen|Matrix`, `Gamma/IsOpen|Bp|Ratio`, `Zoom/IsOpen|Ratio`.
- Прямой init FPGA: `/dev/mem`, маркер `/dev/fpgainit.ini`, `switchvideoformat.sh`.

**Конфиги прошивки (`/home/root/system/`):**
- `display/IMG<imgW><imgH>-UI<uiW><uiH>.ini` — layout (@Rect через QSettings).
  Секция [KUIDesktop] РАЗЛИЧАЕТСЯ по IMG! Дефолт без эндоскопа = IMG1280960.
- `display/project.ini` — модель/серия. `style/stylelist.ini` — бренды.
- `style/X-2600/PyCkeun/qss/` — LogoTitle.png, фон PyCkeun_*.bmp.
- `style/X-2600/PyCkeun/scope/video.ini` — per эндоскоп (hex-имена секций,
  "EC-X20"→"45432d583230"): videoSize=@Rect (→IMG-layout), sensorType, endoType.
- `presetdata/userpreset/[X-2600/X-2600B/]osd.ini` — видеопараметры (полное покрытие KUserSet):
  [Iris]Mode, [Level]Zoom/ImgDenoise/BrightnessEQ, [Color]Mode/R/B/C (пары "a,b"), [Operation]Mode,
  [Zoom]Level1-3=@Variant(float 1.0/1.2/1.4), [Contrast]Level, [ButtomA/B/M]LongPress/ShortPress +
  [FootSwitch]Switch1/2 (коды действий кнопок), [ImgEnhType]Type, [ImEnhGear]EnhType1-3,
  [ImgEnhStrA/B/Edge]EnhLevel1-3 (силы усиления по гирам), [Dehaze]Switch, [HDR]Switch.
- `videoconf/<Категория>/` — Gamma/Ccm/ColEnh/Denoise/Awb/Bright_EQ/CHb/Knee/ImgEnh/IRIS.
  - Gamma/`<SENSOR>_GammaParam[_<SCOPE>].ini`: [V01] bp/gamma/inputmax.
  - Ccm/V1/`<SENSOR>_<WxH>_<SCOPE>.txt`: 9 hex = матрица 3×3 (знак 16-бит, Q9 1.0=512).
  - ColEnh/`<sensor>/colenh_para.txt`: LUT hex.

**DICOM (`/home/root/system/presetdata/userpreset/dicom/`):**
- Классы: KSysDicom (UI настроек), KDicomInterface (фасад DCMTK), KEntityDicom (БД),
  SCU/SCP: KStoreScu, KWorklistScu, KCommitScu, KMppsScu, KEchoScp.
- Таблицы БД: tb_DcmWorklist, tb_DcmStore, tb_DcmStudy, tb_DcmSeries, tb_DcmMpps,
  tb_DcmCommit, tb_DcmCommand, tb_DcmCodeSequence, tb_DcmPerformedProcedureStep.
- Конфиг-XML: WorklistFieldMap.xml (Field dbname↔DatasetPath, format=date/time — 46 полей),
  WorklistDatasetFormat.xml (структура датасета C-FIND), Mpps{Create,Set}{FieldMap,DatasetFormat}.xml,
  dicom.dic (словарь DCMTK). Настройки узла — struct DicomSetting (LocalAETitle/CalledAE/IP/Port/Timeout).
- Операции KDicomInterface: DicomStore/DicomStorePicture (C-STORE Secondary Capture),
  DicomEcho/Ping/CheckNetwork (C-ECHO), DownloadWorklist (C-FIND worklist→tb_DcmWorklist по FieldMap),
  DicomMPPS (N-CREATE/N-SET), DicomCommit (Storage Commitment N-ACTION).
- Прогресс отправки: KStoreScu_ProgressCallback (state, callbackCount, progressBytes, totalBytes).

**Отчёты (`/home/root/system/presetdata/syspreset/mainapp/patient/report/`):**
- Иерархия XML: `config/TempletInfo.xml` (список шаблонов NP-1x4/2x2/4x1/2x3/nx3) →
  `template/ReportTemplateNP-<L>.xml` (порядок Ref) → `template/SubContent/<name>.xml`
  (дерево `<Content>/<Item>`). Item: Name/Title(TR_*)/Type/ShowTitle/Column/DataSrc.
- Типы блоков: RT_TITLE_TABLE_BLOCK, RT_TABLE_BLOCK, RT_TEXT_BLOCK, RT_IMAGE_BLOCK.
- Источники (DataSrc="SOURCE,FIELD"): RT_DATASOURCE_PATIENT (RT_PATIENT_NAME/AGE/GENDER/ID,
  RT_DIAGNOSIS/RT_SURGERY_FINDING/RT_SUGGESTION/RT_BIOPSY/RT_HP…), RT_DATASOURCE_PERIPHERAL
  (RT_TEST_IMAGE<N>/RT_TEXT_MARK<N>), а также REPORT/MEASURE/LOCAL/SUBITEM и мед. пакеты
  (TIRADS/BIRADS/EFW/GA/…). ItemConfig — раскладка (LineHeight/ImageWidth/AlignH/Section).
- SubContent-блоки NP-2x2: HospitalTop, PatientInfo, Image2x2, ExamInfo, Signature, Addition.
- **ItemConfig** (секция в SubContent-XML, ключ = путь "/A/B/C"): раскладка элемента —
  ImageWidth(px), AlignH(Center/Left/Right), FontType(ThirdTitle), Section(Header/Body),
  LineHeight1/2, RefColumn, SynColumnID. Применяется в KDocumentGenerator (width/text-align/
  font-weight/data-section). Реф. KReportEditUIConfig/KReportDisplayParam.
- Классы: KReportTemplateManager/KTemplateCfg (шаблоны), KReportEditDataSource (данные),
  KDocumentGenerator (документ; оригинал → QTextDocument/PDF), KEntityReport (tb_Report).
- **measure**: вендоренный pugixml (`measure::pugi::*`) для XML измерительных пакетов.
- **Тезаурус** (`patient/thesaurus/<lang>/<Scope>.xml`, язык `ch`): словари шаблонов диагнозов
  по типу эндоскопии (Gastroscopy/Colonoscopy/Duodenoscope/Bronchoscope/Choledochoscopy/
  Noselarynxscope). `<record>`: diseagroup/checkedItemname/diseaname/examfinding/diagresult/grid.
  Класс KThesaurusOpt (реф.: GetFileNMameByEndoscopeType, AddDiseaseContent, DelDiseaseContentByGrid,
  MakeChild). Выбор шаблона → автозаполнение RT_DISEASE_NAME/RT_SURGERY_FINDING/RT_DIAGNOSIS.
  UI: KThesaurusWidgetUi/KThesaurusSaveUi (Widgets — device).

**Аккаунты/настройки:**
- KAccount: роли admin/manu (реф. GetAdmin="admin"; логины "adm"/"manu"). Пароли — MD5
  (реф. ConvertPasswordToMD5 → base::MD5String, hex). Дефолт админа = MD5("admin")=
  `21232f297a57a5a743894a0e4a801fc3`. Политика — GetPasswordRegExp (запрет CJK-пунктуации).
  Блокировка после N неудач (реф. GetConstantLoginFailTimes/LockStatus, очередь попыток).
  Персист — QSettings (ключи Account/AdminPassword, Account/FailTimes_/LockStatus_).
- KSystemSet (system.ini, генерится в рантайме): Common/Language, Common/DateFormat,
  Common/InputMethod, Common/Brightness/Volume, Account/autologin, Account/forcelogout,
  Account/forcelogoutTime. UI: KSystemSetDlg, KGeneralSetDlg (Widgets — device).

**Источник холодного света (`system/coldlight/`):** LED-осветитель со спектральными режимами.
- Per-продукт `<SERIES>/<MODEL>/coldlight.ini` [V01]: VLSConfigNum + VLSConfig0..N — комбинации
  спектральных режимов (WL/EWL/SFI/VIST, NULL=пустой слот) для кнопки VLS. X-2600B: 4 комбо.
- `coldlightCommPara.ini` [V01]: "<lightModel>\<mode>" → 26 float (интенсивности LED-каналов +
  коэффициенты режима). Модели света: 10-100-201, EB-X20, EC-2200… + DefaultParam.
- Классы: KColdLightConfig (g_pColdLightConfig; GetVLSConfigDisplayList, SetUserVLSConfig),
  ColdLight (device-comm: NT_QueryLightTemperature, VLSLightError), KColdlightAdjust (яркость).
- ВАЖНО QSettings: значения "a,b,c" авто-разбиваются (toStringList), '\' в ключе = разделитель
  групп (childGroups→childKeys). VLS-режим связан с SetVistSwitch/SetVistMatrix (спектр. обработка).

**Совместимость версий (`syspreset/matchedversion.ini`):** матрица версий прошивки.
- [MatchedVersion] NUM=14 + <компонент>=<верс1>[,<верс2>] (допустимые версии; запятая →
  альтернативы). Компоненты: kernel, hmi, panel, pap, pas, papp00..80, lcd, camera.
- Класс KUpdateConf::GetMatchedVersion(component) → toStringList (реф.). KVersionConfig
  читает установленные версии; апдейт-пайплайн сверяет установленную ∈ matched.
  Формат версии: 6 полей "56.21.00.00.01.17". Реализован KUpdateConf (IsVersionMatched).

**Статистика/события (`syspreset/statistic.ini`):** спека извлечения событий из лога.
Секции [Common]/[Video]/[Special], ключи с префиксом: time_<name> (событие с меткой:
power_on/off, endo_insert/remove/connect, examine_start, video_start/stop), dcnt_<name>
(счётчик: frame lost), info_<name> (температура). Значение = строка-паттерн в логе.
Реализован KStatisticConfig (Load/EventsOfKind/PatternFor); KStatisticInfo — UI-виджет
отображения (device). Закомментированные строки (#) QSettings пропускает.

**Сенсоры:** IMX274 (осн., жёсткие), OV2740, OH01A, OV6946, OCHFA_OAH0428.
**Файлы (снимок/видео):** JPEG `<exam>/<N>.jpeg`, миниатюра `<exam>/thumb/`, видео
`<timestamp>.mp4` (omxh264enc→h264parse→mp4mux→filesink). БД поля: PatientName/
PatientSex/PatientAge/patientid/birthday, AccessionNumber/ExamDate/ExamTime.
**libhal:** см. блок «АРХИТЕКТУРА доступа к железу» выше. Кратко: `update/patch/libhal.so`
(4302 симв., debug_info) — HAL семейства; видео/ISP-регистры идут НЕ через Hal, а через
KMemDevice→/dev/mem. Hal используется для принтера/аудио/USB/BT/дисплей-маяка/диафрагмы(Card).
NEEDED X2000: libhal, DCMTK (dcmnet/dcmdata/…), libsqlcipher, OpenCV 4.3, GStreamer, libMali,
Qt5, boost 1.74, libcrypto.

## 8. Панель управления 8″ (см. docs/HMI_PANEL.md)

Отдельная Cortex-M прошивка (RAM 0x10000000, flash 0x1a040000, FAT12). Иконки в
`lcd_upd_*.bin` (FAT12-образ = диск F:/), формат BGR888 (4Б заголовок 0x05 + W·H·3).
Реверс раскладки готов. РЕШЕНИЕ по коду (МК-прошивка vs 2-й Qt-экран) — НЕ принято.
Модель МК определять по чипу. Иконки НЕ конвертировать — использовать FAT12 как есть.

## 9. ОЧЕРЕДЬ следующих шагов (по порядку, по методологии)

1. ✅ ЧАСТИЧНО (сделано): PL-регистры параметров изображения реверснуты 1:1 и в коде —
   ColorEnhance (0xa18f0008/0024), ImageEnhance (0xa1850058), RBC-тон/гейны
   (0xa1870000/4/8: C/R/B), BrightEQ enable (0xa1950000), AEC+AGC (0xa0048020),
   AWB гейны+строб (0xa184000c + 0xa100019c), AwbCut (0xa1840018). Значения — из
   videoconf (colenh_level.txt / ImgEnh level_*.txt) через AlgParaManager, не хардкод.
   Обёртки KVideoProxy: SendColorEnhanceValue/SendImageEnhanceValue/SetBrightEQLevel,
   RBCValueSet теперь пишет гейны в PL. Проверено self-test `plreg` (14 записей 1:1).
   ✅ ДОБАВЛЕНО (2-й заход): SetBrightEQLut (530 записей, gaussian+lumaGainLut из
   Bright_EQ/), AEC/AGC полная цепочка (SetCameraAEC/AGC — FPGA-I2C 0xa0048074/70,
   SetEndoAEC/AGC — PL 0xa0048020, ветвление по aeRouteMode_, дедуп по кэшу),
   KDccuParam (dccuparam.ini, все ключи AEC/AGC/AWB/RB/Gamma/Zoom/CCM 1:1).
   Проверено `plreg` (+BrightEQ LUT) и `dccu` (roundtrip). Пункт 1 закрыт.
   ХВОСТЫ (на устройстве/по мере надобности): K3ADimming (авто-режимы, что зовёт
   SetAEC/AGCValue по статистике яркости), retry-троттлинг записей, ManualAEC per-сенсор.
2. ✅ СДЕЛАНО: VIST/SFI-матрица (SetVistMatrix/Switch, LoadVistMatrix), Denoise
   (SetDenoiseLevel + SetDenoiseLut LUT из .ini: kernelG/RB/Lut/dpc), режимы
   WL/EWL/SFI/VIST (enum + выбор конфигов), AWB per-режим (LoadAwbGains),
   KVideoProxy::SetOperationMode/SetImageDenoiseLevel/ApplyVistMatrix.
   Проверено self-test `filt` (VIST 7 записей, Denoise 1293). ХВОСТЫ: точная 4-банковая
   раскладка denoise-LUT (сейчас плоские массивы+нулевая добивка банков 2-4),
   SFI-специфика, привязка режима к GetSystemStatus на устройстве.
3. ✅ ЧАСТИЧНО (БД-слой готов): KDicomFieldMap (парсер WorklistFieldMap.xml),
   KEntityDicom (tb_DcmWorklist config-driven из XML + tb_DcmStore очередь отправки,
   методы CreateEntity/GetEntity*/UpdateStoreStatus/GetNumber/Delete), скелет
   KDicomInterface + struct DicomSetting (реверснутый API: DicomStore/Echo/DownloadWorklist/
   MPPS/Commit). Проверено self-test `dicom` (XML 46 полей→42 колонки, роундтрип БД).
   ОСТАЛОСЬ (device-only, DCMTK/HAVE_DCMTK): реальная сеть — KStoreScu::SendFile (C-STORE),
   KWorklistScu::FindData (C-FIND), KEchoScp, KMppsScu, KCommitScu; генерация .dcm Secondary
   Capture из JPEG+пациент/осмотр; парсинг Worklist/MppsDatasetFormat.xml для построения датасета.
   Пересекается с проектом PACS Интер-Мед (можно переиспользовать DICOM-код там).
4. ✅ ЧАСТИЧНО (движок готов): KReportTemplateManager (парсинг TempletInfo/ReportTemplateNP-*/
   SubContent XML → дерево ReportItem), KReportDataSource (привязки DataSrc), KDocumentGenerator
   (шаблон+данные → HTML-документ), KEntityReport (tb_Report: заключение/находки/рекомендации).
   Проверено self-test `report` (5 шаблонов, NP-2x2 → 6 блоков, HTML с сеткой 2x2, БД-роундтрип).
   ✅ ДОБАВЛЕНО: тезаурус (KThesaurusOpt — словари диагнозов thesaurus/<lang>/<Scope>.xml →
   автозаполнение полей отчёта; парсинг/запросы/add-del, self-test `thesaurus`).
   ✅ ДОБАВЛЕНО: ItemConfig-раскладка (KReportTemplate парсит <ItemConfig> по пути элемента →
   ImageWidth/AlignH/FontType/Section; KDocumentGenerator применяет в HTML, self-test `report`).
   ОСТАЛОСЬ: KReportEditUi (редактор — Widgets/UI), рендер в PDF/QTextDocument (device/печать).
   Measure/pugixml — в эндоскопе отдельной измерительной подсистемы НЕТ (measure::pugi = вендор-lib).
5. ✅ СДЕЛАНО (логика): KAccount (роли admin/manu, MD5-пароли, дефолт MD5("admin"),
   валидатор пароля, блокировка после 5 неудач, сброс, смена пароля), KSystemSet
   (Common/*, Account/* через QSettings). Проверено self-test `account`.
   ОСТАЛОСЬ: UI-диалоги (KSystemSetDlg/KGeneralSetDlg/KPUserLoginDlg — Widgets, device),
   force-logout по таймеру, autologin-поток, серверный ID/лицензия (GetServerId).
6. Панель 8″ — после решения по подходу.
7. Живое видео на устройстве (нужен доступ к прибору) + сборка endostation в sysroot.

## 10. Как продолжать (для новой сессии после /clear)

**ТЕКУЩАЯ ПОЗИЦИЯ (обновлять!):** реализовано ~52/491 класса, **33 self-test-режима**
(все PASS), off-device-ядро ROADMAP Фаз A/B/C/D в основном закрыто. За эту сессию +12 классов (+ расширения KDicomFieldMap мульти-record):
+KVideoCal (A), +KUpdateManifest (D), +KSysReportTempletCfg каталог+библиотека шаблонов (C),
+KReportDBTableHandler-пагинация в KEntityReport (C), +KSaveFile нумерация файлов (B),
+KUserOsdSet OSD-конфиг кнопок (A), +KEntityService PRAGMA+бэкап БД (B),
+KReportDisplayParam валидность элементов (C), +KEndoInfoServerConfig облачный конфиг +
KRemoteSwitchConfig пульт (MISC), +KDicomDatasetFormat структура датасета (DICOM),
+KPatientTimeOperation конвертеры дат (CORE), +MD5-верификация пакета (D).
**РУБЕЖ 1: у каждого конфига прошивки (presetdata) теперь есть off-device-ридер с self-test
(unread=0).**

**ТЕКУЩАЯ АКТИВНОСТЬ (уточнённая цель пользователя): аудит существующих реализаций
KPlControl против ДИЗАССЕМБЛЕРА X2000 — «брать логику из бинарника, меньше фантазировать».**
Метод: дизасм каждого `KPlControl::Set*/Read*` (`objdump -d --start/--stop`), сверка
адреса регистра + битовой упаковки; проверка значений через `KPlControl::EnableTrace`
(на десктопе нет /dev/mem — записи логируются в trace, self-test `plreg` их сверяет).
За последний заход НАЙДЕНО+ИСПРАВЛЕНО 4 фантазии: ReadAWBValue (маски 0xffff/0x1ffff →
14-бит 0x3fff), SetCCM1Matrix (упускал хвост data[8]→0xa1880014), SetCCM0Matrix (писал
9 распак. регистров + перепутал адрес 0x14 с enable → переписан на пары+хвост, добавлен
SetCCM0), ReadBrightnessHistogramValue (не сбрасывал триггер 0xa18a0010=0). ДЕКОДИРОВАНЫ:
SetFreezeVideoLoc/SetLensSize/SetDemoireEN (+ пустые SetEnhanceSize/SetContrastLevel).
LUT-БАНКИ (заход аудита): НАЙДЕНА+ИСПРАВЛЕНА фантазия SetGammaLut — банки были
base+0/0x800/0x1000 без упаковки/маски/защёлки; по дизасму (void, читает AlgPara[word 0x545])
это ПАРЫ (v0&0x3ff)|((v1&0x3ff)<<16) в 3 банка 0xa1830800/1000/1800 (512 записей) +
защёлка 0xa1830000|=0x2. Переписано, добавлена plreg-проверка (1537 записей).
SetKneeLut — добавлен кламп count≤1024 (реф. min([0x352c],1024)).
ПОДТВЕРЖДЕНЫ верными: SetCameraIrisType, SetVideoCaptureArea, SetTone/Aurora/Chb/AwbCut/
AWBValue/ImageEnh/ColorEnh/FreezeScaler, SetSensorR/G/BLut (базы 0xa1820800/1000/1800, пары),
SetRbcLut (банки 0xa1878200/8100/8000; реф. void читает AlgPara[word 0x4e8/0x507/0x526]×31),
SetKneeLut (банки 0xa1930800/1000/1800, пары &0x3ff, защёлка |=2).
ПРИМЕЧАНИЕ: реф. Gamma/Knee/Rbc — void, читают из AlgParaManager (raw int-массив по word-
офсетам); у нас data-source вынесен в структурные лоадеры AlgParaManager (архит. выбор),
аудит сверял РЕГИСТРОВУЮ логику. Реф. также poll-ждёт бит2 в ctrl-регистре (device-timing,
на десктопе не воспроизводим — опущено).
**ПРОДОЛЖИТЬ АУДИТ:** SetVistMatrix/SetVistSwitch, SetDenoiseLut,
ReadIrisValue, SetIrisTable (сложная битовая упаковка ириса из AlgPara[0x7a48]),
SetCornerCutWay (стрим 1080-элем. LUT round/octagon из AlgPara[0x7a50/0x7a58]).
Приём поиска нереализованного: `comm -23 <методы-бинарника> <наши>` (см. историю сессии).
ВАЖНО: собирать+гонять `plreg` ДО коммита (был один поспешный коммит — регрессию поймал).

Репозиторий на GitHub (github.com/PazdnikOFF/a2600), git чистый. **План — `docs/ROADMAP.md`.**

1. Прочитать этот файл + **docs/ROADMAP.md** (фазы/приоритеты) + при нужде ARCHITECTURE/HMI_PANEL.
2. Взять следующий пункт: сначала из ROADMAP (Фазы A→B→D→C off-device), потом §9, или запрос пользователя.
3. По методологии §1: `tools/revcalls.sh <mangled>` → дизасм регистров → найти конфиги в
   `update/root` → написать код с ТЕМИ ЖЕ именами классов/методов (не хардкод — читать из файлов).
4. Проверить: собрать `ui_preview` (см. §4), добавить/прогнать self-test-режим в
   `src/ui/preview_main.cpp`, убедиться PASS. Прогнать все 33 режима на регрессии
   (список ниже). Для аудита KPlControl — расширять секции в блоке `plreg`.
5. Дописать статус (§6), факты (§7), обновить ROADMAP (отметить сделанное) + эту позицию (§10).
6. Закоммитить+запушить (git на ветке main, remote origin). **НЕ коммитить `update/` (прошивка) и
   `docs/ref/*.pdf` — они в .gitignore (проприетарный референс SonoScape).**

**Полный список 33 self-test-режимов** (в §4): plreg, filt, dicom, report, account, thesaurus,
userset, coldlight, version, project, statistic, sysstatus, quickinput, style, examcfg, exam,
filebackup, videoset, dsreal, dsdemo, videocal, update, templetcfg, reportdb, savefile, osdset, dbservice, dispparam, endoinfo, remoteswitch, dcmfmt, pattime.

**Остаток ROADMAP (Фазы E/F) — device-bound:** HW (KEndoScope/K3ADimming/KLcdProxy/принтер),
UI (131 Widgets-класс), DCMTK-сеть, GStreamer live-video, панель 8″ (§8 — нужно решение по подходу).
Требуют физического прибора или архитектурных решений пользователя.
