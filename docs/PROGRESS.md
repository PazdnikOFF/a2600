# Endo X-2600 — состояние проекта и методология (READ FIRST)

> Единый документ для продолжения работы после `/clear`. Прочитай его целиком —
> он заменяет длинную историю чата. Детали архитектуры: `docs/ARCHITECTURE.md`,
> панель 8″: `docs/HMI_PANEL.md`, **полный аудит покрытия + план: `docs/ROADMAP.md`**.
>
> Аудит (сверка всех 491 классов референса): реализовано ~52 класса частично,
> сквозное ядро обработки изображения + БД + отчёты + вспом. конфиги готовы и
> проверены (35 self-test, все PASS). Пробелы и фазы — в ROADMAP.md, позиция — §10.

## 0a. Среда реверса hermes (НАСТРОЕНА 2026-07-20 — ИСПОЛЬЗОВАТЬ)

Есть SSH-доступ по ключу к хосту **hermes** (Debian 13 trixie, x86_64, 4 ядра,
sudo без пароля) с ПОЛНЫМ GNU-тулчейном. Это снимает ограничения Mac.
Обёртка — **`tools/rev.sh`** (мне и субагентам):
- `tools/rev.sh sym <regex>` — греп деманглированных символов (ЛОКАЛЬНО, мгновенно;
  карты в `tools/symmaps/*.symmap`, в .gitignore — это полный API-срез проприетарного
  референса, НЕ публиковать). 16 635 функций с ПОЛНЫМИ сигнатурами и типами аргументов.
- `tools/rev.sh dec <bin> <func|addr>` — Ghidra-декомпиляция в C-псевдокод (на hermes).
- `tools/rev.sh dis <bin> <start> <stop>` — чистый aarch64-дизасм (cross-objdump).
- `tools/rev.sh run <bin> [args]` — запуск standalone-бинарника под qemu-aarch64-static
  (Monitor/Simulator/Video исполняются; X2000 — .so, не запускается напрямую).
- `tools/rev.sh filt` — РАБОЧИЙ c++filt (на Mac он молча возвращал вход — проблема была
  в macOS-утилите, НЕ в бинарнике; отсюда все прошлые ручные разборы манглинга).

КЛЮЧЕВОЙ СДВИГ МЕТОДОЛОГИИ (было причиной ошибок — item+0x20 vs +0x60, длина префикса):
1. Сигнатуры — НЕ парсить манглинг руками. `tools/rev.sh sym` даёт готовое имя+типы.
2. Логику функции — читать ДЕКОМПИЛЯТОРОМ (`dec`), а не сырым asm. Дизасм — только для
   сверки констант, когда декомпилятор скрыл иммедиат.
3. Поведение standalone-бинарников — ПРОВЕРЯТЬ ЗАПУСКОМ (`run`) под qemu, а не только чтением.
4. Референс `update/root/*` лежит на hermes в `~/a2600ref/bin/` (перенесён; проприетарный,
   в git не коммитить — как и локально).

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
ui_preview cornercut                                 # self-test геометрии обрезки углов (круг/8-угол) + стрим 0xa18c8000
ENDO_ROOT=$ER ui_preview scopecut                    # self-test scope-info video.ini (defaultRound/OctangleCut, hex-имена)
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
ENDO_ROOT=$ER ui_preview fxpt                         # self-test фикс.-точки + AEC/AGC/getAecValue/FreezeCalResolution KVideoProxy
ENDO_ROOT=$ER ui_preview language                     # self-test мультиязычности (mutilanguageinfo.ini + languageConfig)
ENDO_ROOT=$ER ui_preview unicodetext                  # self-test карты экранной клавиатуры (multi_language_unicode_2_text.xml + KLoadUnicodeText)
ENDO_ROOT=$ER ui_preview encstyle                     # self-test совместимости эндоскопов/камер (matchedScope.ini + KEncStyle)
ui_preview recfiles                                   # self-test пересчёта файлов записи осмотра (KExamListRecordFileUpdate::GetFiletypeNumFromPath)
ENDO_ROOT=<tmp> ui_preview dcmconf                    # self-test конфига DICOM-сервисов (link-dicom.json — пишет файл, брать tmp-root!)
ui_preview kconfig                                    # self-test INI-движка ядра (KConfig: формат True/False, '#'-комменты, парсинг)
ENDO_ROOT=<tmp> ui_preview listsetup                  # self-test конфигов списков пациентов/worklist (пишет файлы, брать tmp-root!)
ENDO_ROOT=$ER ui_preview meaxml                       # self-test фундамента XML (KMeaXMLBase+KEnvConfig на реальном Template(NP-1x4).xml)
ENDO_ROOT=$ER ui_preview templatecfg                  # self-test загрузчика шаблонов отчёта (KTemplateCfg, ветка FullTemplate, 5 файлов)
ui_preview strutil                                    # self-test строковых утилит (KMeaStringUtil: split/trim/конверсии — неинтуитивная семантика)
ENDO_ROOT=<tmp> ui_preview examno                     # self-test генератора номеров осмотра (KExamNoGenerate — пишет файл, брать tmp-root!)
ENDO_ROOT=$ER ui_preview templatelib                  # self-test библиотеки шаблонов (KTemplateLibCfg + report_template::Merge*/ID — 11 блоков, 5 групп)
ui_preview templateparam                              # self-test параметров шаблона (KTemplateParamCfg — в прошивке мёртвый класс, фикстура своя)
ENDO_ROOT=<tmp> ui_preview manupwd                    # self-test доступа производителя (KManuPwdMng: пароли от даты + лицензия + countdown; пишет [Manu], tmp-root!)
ui_preview dbfileop                                   # self-test файловых/дисковых утилит (KDbFileOperation: ФС + statfs-ёмкость)
ENDO_ROOT=<tmp> ui_preview controlini                 # self-test слоя ini машинного контроля (KControlINI, control.ini — пишет файл, tmp-root!)
ui_preview patstr                                     # self-test строковых/DICOM-утилит (KPatientStringOperation+KDbStringOperation: split/charset/SOP UID)
ui_preview stopwatch                                  # self-test экранного секундомера (KStopWatch: конечный автомат старт/пауза/стоп, offscreen)
ui_preview patient                                    # self-test сущности/CRUD пациента (KEntityPatient+KPatientListDBTableHandler, tb_PatientList)
ui_preview doctor                                     # self-test сущности/CRUD врача (KEntityDoctor+KDoctorDBTableHandler, tb_Doctor + недавние по time/count)
ui_preview dbstr                                      # self-test построителя SQL-условий (KDbStrHandler) + факт SQLCipher-ключа
ui_preview session                                    # self-test состояния облачной сессии (KSessionInfo: Manu/Service каналы + NAM)
ENDO_ROOT=$ER ui_preview encset                       # self-test зашифрованных списков моделей (KEncSettings: bitwise NOT CSV, реальный genc.ini)
ui_preview textblock                                  # self-test модели текстового блока отчёта (KTextBlock: текст из шаблона+данных + стиль)
ENDO_ROOT=$ER ui_preview imageblock                   # self-test модели блока-картинки отчёта (KImageBlock: Url/Width/Heigth/align)
ui_preview examdata                                   # self-test перечислителя файлов обследования (PatientExamData: image/video/pdf/all по каталогу)
ui_preview kobject                                    # self-test in-process шины сообщений (KObject/KMessage/KPublishManager: pub/sub+send+request, синхронно)
ui_preview xmlparser                                  # self-test обёртки XML-документа (XmlParser: load/save/root/декларация на QDomDocument)
ENDO_ROOT=$ER ui_preview reporttmplmgr                 # self-test капстоуна отчётного модуля (KReportTemplateManager: синглтон+InitModule+части)
ui_preview kcontrolproc                               # self-test машинного контроля (KControlProc: DES-крипто "ZXYuio12", LicenseFileName, Cipher2Plain, дедлайн)
ui_preview templetsave                                # self-test записи каталога шаблонов (KSysReportTempletCfg: реф.-схема TempletInfo.xml, map-семантика depts)
ui_preview des                                         # self-test DES (yxyDES2: канонический вектор 85E813540F0AB405, 2 слота ключа, AnyLength)
ui_preview dbsqlite                                    # self-test низкоуровневой обёртки SQLite (KDbSqlite ЯДРО: open/close/exec/error на libsqlite3; шифрование опущено)
ui_preview tableblock                                 # self-test модели табличного блока отчёта (KTableBlock: сетка/бордюр/ячейка→под-элемент)
ui_preview titletableblock                            # self-test таблицы-с-заголовком (KTitleTableBlock: наследник, +строка заголовка/Title/SetTitle)
ui_preview reporttmpl                                 # self-test свободных функций report_template (map⇄строка, ConvertToSourceID, GenerateIDByString, IsPatientInfoTitleBold)
ui_preview templetmodel                               # self-test модели+контроллера каталога шаблонов (KSysReportTempletModel/Control: квирки реф., кэш cfg, выбор департамента/шаблона)
ENDO_ROOT=<tmp> ui_preview dimmer                     # self-test ЗАПИСИ параметров авто-диммирования (KColdLightConfig::Set*DimmerParam — пишет ini, tmp-root!)
ENDO_ROOT=<tmp> ui_preview printdata                  # self-test персиста принтеров (KSysPrintData: XML-кэш, дефолты, url→driver — пишет XML, tmp-root!)
ENDO_ROOT=$ER ui_preview legalnotice                  # self-test чтения уведомлений о стороннем ПО (KThirdPartyLegalNotices::ReadLegalNoticeText)
ENDO_ROOT=<tmp> ui_preview autotest                   # self-test парсера скриптов автотеста (X2000Simulator: INIFileCaseExec + таблица кодов клавиш — пишет скрипты, tmp-root!)
```

**Регрессия всех режимов одной командой** (`tools/selftest.sh`, режимы, пишущие файлы,
сами получают временный ENDO_ROOT):

```bash
tools/selftest.sh "$SCR/uibuild/ui_preview"     # → "PASS: 79  FAIL: 0"
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
    ├── report/KSysReportTempletModel   # рабочая модель каталога шаблонов (инфо+cfg-кэш+delList)
    ├── report/KSysReportTempletControl # синглтон-контроллер выбора (департамент/шаблон)
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

**Нумерация файлов снимков/видео — расхождение снято (KSaveFile vs KExamDataFileNameGenerator):**
- `KSaveFile::FormatFlowNumber` @0x6a89b8 (0x350) — **чистый zero-pad до 3 знаков** через
  `std::stringstream` (`width(3)` + `fill('0')`, `ss << n; ss >> str`). В теле НЕТ константы
  0x3e7 и НЕТ ни одной ссылки на .rodata → внутри функции ни потолка 999, ни маркера '^'.
- Маркер переполнения добавляет **вызывающий** — `KSaveFile::GetFileFlowNumber` @0x6a99d8:
  .rodata `"999^"` @0x8695f8 и `"001"` @0x869600; вставка `"999^"` (len 4) в позицию 0 перед
  результатом `FormatFlowNumber(n)` → имена вида `999^001`. `LogPrintf` рядом получает
  литеральную 999 (`mov w3, #0x3e7`), сравнение круга — `cmp x22, #0x3e6`.
- `KSaveFile::CheckIsFileNumberUseUp` @0x6a92c8 сравнивает с .rodata `"999^999"` @0x8695d0 —
  последнее допустимое имя. `FindMaxFileFlowNumber` @0x6ab6d0 распознаёт обе формы (regex
  `([0-9]{2}[1-9]{1}|...)`, `(.jpg|(_[0-9]{3}.mp4))`, `([\^]{1})`, `999`).
- Итог: схема нумерации **одна**, и она совпадает с `KExamDataFileNameGenerator::GetFileSerialNum`
  @0x48BEA8 (`"%d^%03d"`, первый `%d` — литеральная 999). Ранняя реимплементация
  `FormatFlowNumber(1000) → "999^"` была **ошибочной**; исправлено, формат — `999^001`.

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

**ТЕКУЩАЯ ПОЗИЦИЯ (обновлять!):** **106 self-test-режимов** (все PASS, регрессия —
`tools/selftest.sh`).

**⚠️ ВЫВОД СЕССИИ 2026-07-21: ЧИСТЫЙ OFF-DEVICE-БАКЕТ ИСЧЕРПАН.** После закрытия
KDocumentGenerator + всей render-подсистемы прощупаны следующие кандидаты ROADMAP — ВСЕ
оказались device/UI/ловушки (детали в ROADMAP §C/§D):
- `KEntityBase` (B3) — абстрактный базовый класс, все методы-заглушки (ICF-folded), IDatabase
  не реверсирован, ERR_NOT_SUPPORT рантайм-.bss, подклассы коллизируют → ЛОВУШКА (ROADMAP C4).
- `KUpdateAction`/`KUpdatePrepare` (B2) — оба Qt-ДИАЛОГИ (setupUi/keyPressEvent).
- `KFunTest` (B5) — Qt-диалог; `KTimeMng` (B5) — QObject с QTimer (рантайм).
- `KDataFileOpr` (B3) — USB/fs-утилиты (GetUsb*, device); чистые fs-методы (rm -r) — низкая
  реверс-ценность.
- IRIS/*.txt (D3) — читаются через `KIrisSetting`/`KIrisMenu` (OSD-меню панели, device UI).
**✅ НАЧАТА UI-ФАЗА (решение пользователя 2026-07-21: «UI-порт Qt Widgets»).** Workflow
установлен: реверс `Ui_*::setupUi` (дерево виджетов) → сборка диалога на Qt Widgets →
превью-ветка в preview_main (`w = new Dialog`) → скриншот `ui_preview <mode> <out.png>`
(харнесс `w->grab().save`) → визуальная верификация чтением PNG. НЕ self-test (нет PASS/FAIL),
регрессия не затрагивается. ПЕРВЫЙ ПОРТ: **`KUpdatePrepare`** (`app/src/ui/KUpdatePrepare.*`,
режим `updateprepare`) — диалог подготовки обновления. Раскладка восстановлена из
Ui_KUpdatePrepare::setupUi @0x6e2410: QGridLayout, разрежённые строки, frame_update
(StyledPanel, min 300×50) с кнопкой btn_update (фикс 150×40, TR_Ugde), label_msg,
progress_rar (max-w 600, скрыт в initWidget), спейсеры центрируют. Скриншот СВЕРЕН — кнопка
TR_Ugde в центрированной рамке. Реф. база — КАСТОМНЫЙ `KDialog` (SetTitle/SetKStyle/MoveLayout/
close-abort/keyPressEvent, @0x68ad60) ЕЩЁ НЕ ПОРТИРОВАН — подставлен QDialog (клиентская
раскладка совпадает); хендлеры StartUpdate/StartDecompress (USB/unrar) — заглушки (device).
**✅ БАЗОВЫЙ `KDialog` ПОРТИРОВАН** (`app/src/ui/KDialog.*`, реф. @0x68ad60). KDialog : QDialog,
безрамочное окно поверх остальных; хром: KFBackground (тёмный фон rgba(26,26,26)+рамка) →
KFUpbar (титул-бар h=35, #0f1218) с label_title + btn_close (✕) + область контента ContentArea()
для подклассов. SetTitle (windowTitle+лейбл), SetKStyle (_KDLG_STYLE: пресеты ширины 320/480/640/
700/1024, 1=fullscreen/без рамки, 2=460), keyPressEvent(Esc→close), сигнал readyToClose.
objectName-ы (KFBackground/KFUpbar/label_title/btn_close) 1:1 с прошивочным qss; стили заданы
значениями из style.qss (не зависим от файла). Реф. хром в отдельном KLayOut + overlay
m_titleLabel — у нас встроен в KDialog (визуально эквивалентно, помечено). **KUpdatePrepare
ретрофичен под KDialog** (контент в ContentArea, SetKStyle(FULLSCREEN)+SetTitle из ctor).
СКРИНШОТ СВЕРЕН: тёмный титул-бар с TR_Ugde + ✕, контент-кнопка по центру.
СЛЕДУЮЩЕЕ ПО UI: остальные диалоги поверх KDialog. ✅ KImportRules ПОРТИРОВАН (режим `importrules`):
label(китайская подсказка)+cb_rules+textEdit+btn_import(Import), абсолютная геометрия 588×420,
в ContentArea; скриншот сверен. USB-логика (GetRulesFile/ReadFileToTextEdit/OnClickImport) —
заглушки (device). ✅ KUpdateAction ПОРТИРОВАН (режим `updateaction`): 12 разделов-чекбоксов (app/hmi/pap/papp00-07 без 05/papp80/lcd) + версии + Start/PowerOff + прогресс, корневой QGridLayout с вложенными frame_item/frame_btn; скриншот сверен. KUpdateConf/версии/видимость — заглушки (device). ✅ KFunTest ПОРТИРОВАН (режим `funtest`): два QListView (Use Case Library/Execute Test Cases) + splitter >>/<< + Count/Speed + Start + checklog(OPEN/CLOSE) + ImportCheckRules; абсолютная геометрия 640×480, SetKStyle(W640); скриншот сверен. Автотест-движок/загрузка кейсов — заглушки (device). ПОРТИРОВАНО 4 диалога + база KDialog (workflow конвейерный). ✅ KMessageBox ПОРТИРОВАН (режим `messagebox`): QDialog (НЕ KDialog!) с кастомным титул-баром + встроенным настоящим QMessageBox; статические information/question/warning/confirm; кнопки перестилены (TR_Yes/No/OK/Ccl + тёмный градиент); скриншот сверен. ⚠️ ЛОВУШКА C5: KFactoryOptions коллизирует с sys/KFactoryOptions (см. ROADMAP). УРОК: грепать class <Name> ДО UI-порта. ✅ KWaitProgressBar ПОРТИРОВАН (режим `waitprogress`): тонкая KDialog-обёртка (480×160, WindowModal, Esc не закрывает) вокруг дочернего KProgressBar (крутилка/текст/cancel/таймер — в НЁМ, реф. отдельный класс x604a50, Ui_KProgressBar::setupUi x605090 — заглушка QProgressBar, СЛЕДУЮЩИЙ реверс); скриншот сверен. Портировано 6 диалогов + база KDialog. ✅ KProgressBar ПОРТИРОВАН (виджет-содержимое KWaitProgressBar): QWidget, label_Text(TR_Prpng) + label_icomprogress(крутилка 40×40, 21-кадровый PNG-спрайт qss/black/bar/progressbar/*.png по QTimer 200мс) + ProgBar(скрыт) + btn_Cancel(TR_Ccl); вошёл в KWaitProgressBar; скриншот сверен. Worker OperateWaitThread (задача/текст/_KProgressType) — device-стуб. Портировано 6 диалогов + KDialog + KProgressBar. ✅ KVersion ПОРТИРОВАН (режим `version2`): диалог версий, сетка 19 строк × 3 колонки (check-иконка | подпись TR_PModel/Rlse/Cmplte/App/Krnl/HMI/Pnl/PAP/PAS/PAPP00-07/PAPP80/LCD/CAM | значение) + Exit; значения из KVersionConfig::GetVersion (off-device); SetKStyle(W460); скриншот сверен. check-иконки/ролевая видимость — device. Портировано 7 диалогов + KDialog + KProgressBar. ✅ KSysDicom ПОРТИРОВАН (режим `sysdicom`): диалог настроек DICOM, 3 группы (Basic: 4 поля SnName/Prt/LATitle/Tmt; Storage: 2 чекбокса; Server list: QTableWidget + 8 кнопок Add/Edit/Del/Ping/Verify/Default/Save/Exit); скриншот сверен. DCMTK-сеть (C-ECHO/Ping/Store)/KDICOMConf — device-заглушки. Портировано 8 диалогов + KDialog + KProgressBar. ✅ KLogin ПОРТИРОВАН (режим `login`): диалог входа, форма Acct(maxLen8)/Pswd(password,maxLen16) + Login/Logout(скрыт), SetKStyle(W460), без close-кнопки; скриншот сверен. Проверка пароля/KAccount — device. Портировано 9 диалогов + KDialog + KProgressBar. ✅ KDeviceInfo ПОРТИРОВАН (режим `deviceinfo`): экран публичного знака — центрированная QR-картинка (publicSignEN.jpg прошивки) 300×300 + подпись + Exit 120×120; SetKStyle(W460), титул TR_Svce; скриншот сверен (реальный ассет). KQRCode (реф. QFrame-наследник) → заглушка QLabel-картинка. Портировано 10 диалогов + KDialog + KProgressBar. ✅ KGeneralSetDlg ПОРТИРОВАН (режим `generalset`): большой диалог настроек, 4 секции (больница: имя/лого/кнопки; пациент: 9 чекбоксов полей; учётки: радио логина/логаута + спинбокс таймаута 30мин; кнопки Default/Save/Exit); SetKStyle(W460), титул TR_Gnrl4; скриншот сверен. LoadSystemConf/SaveAccountSet/лого-с-USB — device. ✅ KControlInfo ПОРТИРОВАН (режим `controlinfo`): диалог машинного контроля/лицензии, 2 группы (Processor/Endoscope) с полями SN/дата/остаток/поддержка + View/Activate + Exit; SetKStyle(W460), титул TR_TTInformation; скриншот сверен. KControlProc/DES/EEPROM — device. ✅ KAuthMachineDlg ПОРТИРОВАН (режим `authmachine`): диалог авторизации машины, ТОЛЬКО отображение (поля ввода кода НЕТ) — группа TR_TMAuthorization с 4 строками SN/CN/EDate/Rmnng (значения-заглушки) + 2 кнопки-действия ETMachine/SSettings2 (minW 420) + Exit (fixed 160); SetKStyle(W460); скриншот сверен. ClickAuthMachine (DES/KControlProc)/SetServerInfo/заполнение из KSystemSet — device. ✅ KSystemSetDlg ПОРТИРОВАН (режим `systemset`): большой диалог системных настроек 566×1080, НЕ отдельные диалоги — всё секциями одного QGridLayout: Видео/UI (5 комбо: разрешение/форма угла/срез/объектив/сегментация) + Язык/Дата (язык + формат даты + QDateEdit + QTimeEdit) + Сеть (IP/маска/шлюз через QLineEdit+inputMask, MAC) + label_message(TR_TITEARestart) + панель кнопок (Общие/Параметры/Устройство/Default/Save/Exit); SetKStyle(W460), титул TR_SSettings; скриншот сверен. Кастом-виджеты заменены: KLineH→QFrame(HLine), KIpLineEdit→QLineEdit+маска, KParamSetBtn→панель. KSystemSet(ini)/KNetWorkSet(NIC)/SetSystemTime(RTC)/тик часов/роль-гейтинг MAC — device. ⚠️ Реверс сиблингов KSystemSetDlg (footer KParamSetBtn → SwitchDialog): TR_Gnrl4→KGeneralSetDlg (уже есть), TR_DSetting→KSysDicom (УЖЕ ПОРТИРОВАН — коллизия поймана пречеком, повторно НЕ портировали), TR_PSET2→KSysPrinter (реально «печать», НЕ image-param); истинный image-param редактор = KCustomEdit (@0x633e20, СЛЕДУЮЩИЙ кандидат). ✅ KSysPrinter ПОРТИРОВАН (режим `sysprinter`): диалог настроек принтеров (реф. KSysPrinter, ctor @0x79c3d8), заголовок TR_SPrinter + QTableWidget 5 колонок (TR_PName/SType/CType2/Dvc/Dflt1, строки device) + кнопки Add/Del/Default/PrintSettings + Exit; SetKStyle(FULLSCREEN) под реф.-геометрию 1680×900; Del/Default/PrintSettings активны только при выделении строки (реф. OnTableWidgetSelectionChanged); скриншот сверен. KPrinterManager/CUPS/RefreshTable — device. ✅ KCustomEdit ПОРТИРОВАН (режим `customedit`): модальный редактор параметров изображения/видео (реф. KCustomEdit @0x633e20), 2 страницы (frame_page1 видна / frame_page2 скрыта, btn_nextpage «>» переключает — SwitchShowPage, чистый UI): page1 = усиление (тип + L1/L2/L3 ImgEnh/ColorEnh комбо) + баланс RGB (radio R/B/C + спины) + зум (L1 read-only 1.0, L2/L3 double 1.1–4.0) + кнопки эндоскопа (0/1/2/3 комбо) + педали (Lt/Rt); page2 = IRIS/denoise/brightEQ комбо + dehaze/HDR (KOptionListButton→QComboBox); SetKStyle(W460), титул TR_PSet; скриншот page1 сверен. Кастом заменены: KLineH→QFrame(HLine), KOptionListButton→QComboBox, KParamSetBtn→панель. 17 слотов Change* (видеопроцессор) + LoadVideoParam/входящие фиды — device. ✅ KDICOMServiceEditDlg ПОРТИРОВАН (режим `dicomsvcedit`): немодальная форма добавления/редактирования DICOM-сервиса (реф. @0x5b1a18), открывается из KSysDicom(Add/Edit); грид: тип (Storage/Commitment/Worklist/MPPS) + имя(maxLen16) + режим адреса cmb_server(IP/домен→QStackedWidget) + IP(маска)/Ping/результат + порт(validator 0..65535)/Echo(TR_Vrfctn)/результат + AE-title(maxLen16) + cmb_cmtType(device) + макс-результатов("99",maxLen3,0..999) + описание(TR_Inf/CField1/2) + подсказка TR_RField + Confirm/Cancel/Reset; SetKStyle(W700), титул TR_ADService(add)/TR_EDService(edit); скриншот сверен. Кастом KIpAddrEdit→QLineEdit+маска; cmb_server-переключение IP↔домен реализовано. Ping/EchoService/Save/persist/LoadData — device. ✅ KPatientListAddDlg ПОРТИРОВАН (режим `patientadd`, ПЕРВЫЙ клинический диалог): ввод нового пациента (реф. @0x7c2a60), немодальный, ФИКС 1024×712, АБСОЛЮТНАЯ геометрия (без layout'ов). Группа «инфо пациента» (сетка 3×3: ID/имя*/пол, ДР+календарь/возраст/тел, койка/2 польз-поля) + группа «инфо обследования» (пункт*/план-дата+календарь/направитель/дата-заявки) + подсказка TR_RField + Exam/OK/Cancel; SetKStyle(W1024), титул TR_APatient; красные «*» на обязательных (имя/пункт); возраст↔ДР пересчитываются взаимно (чистый UI, с блокировкой петли); скриншот сверен. Кастом KMemComboBox→editable QComboBox+maxLength, KPatientDateEdit→QDateEdit. OnBtnConfirm/OnBtnExam(persist БД+запуск), MRU/поиск в БД, cmb_examitem(GetEndoClassToQstring), польз-поля/видимость групп/формат дат — device. Соседи (не цель): KPatientManagmentUi (воркспейс), KPatientListViewUi/Search/OptUi (браузер), KPatientListEditDlg (edit-близнец), KPatientListSetupDlg (колонки). Портировано 18 диалогов + KDialog + KProgressBar. ✅ KPatientListSearch ПОРТИРОВАН (режим `patientsearch`): панель поиска по списку пациентов/worklist — ВНИМАНИЕ: это НЕ KDialog, а встраиваемый QWidget-строка (реф. @0x7d5cf0, base QWidget, 1591×170, абс. геометрия); порт как самостоятельный QWidget (базовый класс совпал), строка перестроена в сетку 4×2 «подпись/поле» + колонка Search(F11)/Reset(F12); 8 критериев: ID/имя/пол+возраст(диапазон)/пункт/направитель/статус/дата-от/дата-до(«--»); наружу сигналы QueryItems/SigToExit/SigToFocusOut; скриншот сверен. Кастом KPatientDateEdit→QDateEdit. Reset (SlotToResetSearchData) — чистый UI, реализован; Search собирает map и эмитит QueryItems (запрос в БД downstream = device). ПРИЁМ: не форсить KDialog, если реф.-база = QWidget — порт как QWidget, харнесс grab() снимает любой QWidget. Портировано 18 диалогов + 1 виджет-панель + KDialog + KProgressBar. ✅ KReportEditUi ПОРТИРОВАН (режим `reporteditui` — ВНИМАНИЕ: `reportedit` уже занят self-test'ом БД-отчётов, коллизия ключа поймана при рендере → переименовал): редактор отчёта обследования (реф. @0x4d4108) — ЦЕНТРАЛЬНЫЙ клинический диалог. База KFullScreenDialog(parent,2007) — НЕ KDialog и НЕ модаль (3-й арг = int-id окна, не bool!); портирован над KDialog(FULLSCREEN), 1920×1195. Слева колонка 14 кнопок (211px): SImage/AMark/WConclusion | UDICOM/PPreview/Print/шаблон/Glry/SAGlossary/Sttgs | Save/Exit. Справа: сетка базовой инфо (13 полей ENo/дата/эндоскоп/статус/имя/пол+возраст/PID/направитель/ДР/тел/койка/2польз) + инфо-строка изображений + 8 текст-полей со счётчиками (VOExam/EConclusion=3000, DName/OMode3/IFindings/Sgstn/2польз=800) + ряд врачей (биопсия/HP/ассистент/врач) + панель глоссария (runtime). Счётчики символов + обрезка по maxLen — чистый UI, реализованы. Скриншот сверен (8 счётчиков видны). Кастом заменены: KFullScreenDialog→KDialog, KGridWidget→QWidget, KQuickInputComboBox→editable QComboBox, KCounterTextEdit→QTextEdit+счётчик. ClickBtnSave(persist БД)/SaveAsWordBank/UploadDicom/print/синхро-вьюхи/источники данных — device. Соседи глоссария (не цель): KThsaurusManageMentUi, KThesaurusSaveUi. ПРИЁМ: ключ preview-режима грепать среди self-test имён ДО выбора. Портировано 19 диалогов + 1 виджет-панель + KDialog + KProgressBar. ✅ KThesaurusSaveUi ПОРТИРОВАН (режим `thesaurussave`): сохранение фразы в глоссарий (реф. @0x4e3e30), открывается из KReportEditUi(btn_save_as_word_bank); KDialog немодаль, фикс 1024×768, SetKStyle НЕТ, титул SetTitle(TR_AGlossary) (перекрывает setupUi-титул TR_Glry); абс. геометрия; блок группа/заголовок (cmb_group editable device + edit_title maxLen100 + Grp:/Ttle: с красными «*») + блок находки/заключение (2 QTextEdit префилл под VOExam:/EConclusion:) + подсказка TR_RField + Save/Cancel; все stock Qt; скриншот сверен. KThesaurusOpt (ReadFile/GetDiseagroupList→cmb_group, AddDiseaseContent на Save, валидация+warning) — device. ✅ KThsaurusManageMentUi ПОРТИРОВАН (режим `thesaurusmgr`): управление глоссарием (реф. @0x4e75e8, орфография «Thsaurus» как в бинарнике), KDialog немодаль, 1024×768, SetKStyle НЕТ, титул SetTitle(TR_Glry); layout-based; верх — панель редактирования (cmb_group editable + edit_title maxLen100 + Grp:/Ttle: с «*» + 2 QTextEdit находки/заключение), низ — панель обзора (cmb_device 5 статичных типов эндоскопа TR_Gspy/Cspy/Bspy/Nlspy/Cholngscpy + tree_model 1-колоночный QTreeWidget + edit_content disabled) + подсказка TR_RField + Add/Edit/Clone/Delete/OK/Cancel (по 133px); Edit/Clone/Delete активны только при выделении в дереве (чистый UI); скриншот сверен. Кастом KTextEdit→QTextEdit. KThesaurusOpt CRUD/ReadFile/UpdateTree/SwitchDeviceType — device. Глоссарий-пара (Save+Manager) закрыта. ✅ KReportPreviewDlg ПОРТИРОВАН (режим `reportpreview`): предпросмотр печати отчёта (реф. @0x500d08), открывается из KReportEditUi::ClickBtnPrintPreveiw. База KFullScreenDialog(parent,-1) (НЕ KDialog, 2-й арг = int-id -1, не bool) → портирован над KDialog(FULLSCREEN), 1920×1080, SetKStyle(FULLSCREEN), титул SetTitle(TR_Prvw2). Слева тулбар (stretch 235:1685, bg rgb29,31,35): по-ширине(F1)/по-странице(F2)/печать(F3) | HLine | zoom+(F4)/zoom−(F5) | comboBox_scale (editable, 29 % статичных + regex-валидатор [0-9]{0,3}%, idx0) | comboBox_template (device) | Exit. Справа widget_preview/gridLayout_preview — реф. встроенный KReportPreviewCenterDlg (рендер страницы) → плейсхолдер QLabel. Zoom+/− двигают comboBox_scale (чистый UI); скриншот сверен. OnBtnPrint(QPrinter)/comboBox_template(OnDeptChanged/RefreshComboBox)/рендер-пайплайн — device. ✅ KReportTempletEditDlg ПОРТИРОВАН (режим `templeteedit`): редактор шаблонов отчёта (реф. @0x51f300, орфография «Templet»), открывается из KReportEditUi::ClickBtnTemplateSettings. ПОДТВЕРЖДЕНО ≠ data-класс KReportTemplateManager (диалог его использует, но это разные классы). База KFullScreenDialog(parent,-1) → KDialog(FULLSCREEN) 1920×1080, титул SetTitle(TR_Tpte). Слева (stretch 0:1): label TR_Tpte: + comb_templet (device — библиотека/отделение) + tree_content (KTempletTreeWidget→QTreeWidget, 1 кол, header скрыт, чекбоксы секций). Справа right_widget(minW1200): txt_edit_templet (KNewTempletEditor→QTextEdit 820² read-only WYSIWYG). Снизу Exit/Save&Exit | EditInfo/Default; тексты кнопок переопределяются в InitWidget (Exit→TR_CExit, Default→TR_Dflt). Скриншот сверен. Кастом: KFullScreenDialog→KDialog, KTempletTreeWidget→QTreeWidget, KNewTempletEditor→QTextEdit. comb_templet(GetTempletsInfos)/tree(KReportTemplateDataNew)/preview(KDocumentGenerator)/OnSaveAndExit/OnResetDefault/OnDeptChanged/OnEditInfoClicked — device. ✅ KImageEditor ПОРТИРОВАН (режим `imageeditor`, ПЕРВЫЙ из workflow захвата/аннотации): аннотирование снятого изображения (реф. @0x4a1cc8). База KFullScreenDialog(parent,-1) → KDialog(FULLSCREEN) 1920×1080, титул SetTitle(TR_IProcessing). Слева graphicsView (KImageEditorGraphicsView→QGraphicsView, чёрный фон) + label_file_name (device). Справа: label TR_CType + 5 icon-only radio курсора-стрелки (Lock/RightDown/RightUp/LeftUp/LeftDown, авто-эксклюзив, дефолт Lock; иконки — ресурсы, у нас глиф-замена ↘↗↖↙) + спейсер + Prev(F4)/Next(F5)/Save(F1)/Delete(Del)/Exit(Esc) (хоткеи из ctor). Скриншот сверен. Кастом KImageEditorGraphicsView→QGraphicsView. SetArrow*/OnBtnSave/OnBtnDelete/SetChangedWithoutSave/label_file_name — device; Exit→close, Prev/Next — UI-нав. ✅ KReportEditAddMarkView ПОРТИРОВАН (режим `addmarkview`): панель добавления меток в отчёт — ВНИМАНИЕ: НЕ диалог, встраиваемый QWidget (реф. @0x4be7b8, base QWidget, 1614×760), KReportEditUi кладёт в grid(0,1); своих OK/Cancel НЕТ. Порт как самостоятельный QWidget. Три колонки: A (метка изображения: label TR_IProcessing + 5 arrow-radio lock/ld/lu/ru/rd эксклюзив + graphicsView_imgMark + label_file_name + Prev/Next/Clear), B (имя позиции: label TR_OName + label_selected + listWidget_organ editable device + Add/Del/Clear-disabled), C (боди-метка: label TR_Bdmk + 5 radio ld/lu/ru/rd/point эксклюзив + graphicsView_positionMark + Clear). Кастом KImageEditorGraphicsView→QGraphicsView (2 экз.). Add/Del позиции — операции со списком (UI, реализованы); скриншот сверен. SetImgMarkArrow*/SetPositionMark*/draw-event/img-нав/listWidget-персист(KReportEditDataSource) — device. Портировано 24 диалога + 2 виджет-панели + KDialog + KProgressBar. ⚠️ KVideoCal (диалог видео-калибровки) РЕВЕРСНУТ но НЕ портирован — коллизия с нашим data-классом video/KVideoCal (ROADMAP C6, пречек поймал). ✅ KPrintSettingsDlg ПОРТИРОВАН (режим `printsettings`): настройки печати (реф. @0x78ec10), KDialog немодаль, 900×900, SetKStyle НЕТ, титул SetTitle(TR_PESettings) + тёмный setStyleSheet. Превью снимка m_pImageLbl (device→плейсхолдер) + сетка принтера (имя-комбо device + размер бумаги A4/Carta статично + чекбокс TR_IOptimization) + сетка слайдеров (яркость[-100,100]/гамма[0,2000], KMySlider→QSlider, кнопки −/+ 25×25) + OK/Cancel/LoadDefault. Скриншот сверен. Кастом KMySlider→QSlider. OK/Cancel→close (чистый UI); слайдеры/±/LoadDefault/printer-change/превью (KPrinterManager/гамма-яркость печати) — device. Портировано 25 диалогов + 2 виджет-панели + KDialog + KProgressBar. ✅ KDicomQueueViewUi ПОРТИРОВАН (режим `dicomqueue`): вид очереди DICOM-заданий — ВНИМАНИЕ: НЕ диалог, встраиваемый QWidget+KObject-mixin (реф. @0x8102f8, base QWidget, 1630×1034). Порт как самостоятельный QWidget. grp_view → vbox → [widget_search (KDicomQueueSearch→плейсхолдер-фильтр), tableView (KTableView→QTableWidget, 11 колонок: чекбокс/Tpe2/Svce/PID/Nme/STime/Fname/Sze/Stts/Dtls1/id-скрыт), пейджер-бар абс.геометрия (label записей + |</</поле/номер/>/>| — KPagePushButton→QPushButton глиф-замена, KPageLineEdit→QLineEdit)]. Строки — device (БД). Скриншот сверен. OnGetDicomQueueDataFromDB/OnQuery/OnResendSelectedData(пере-отправка)/SubscribeMsg — device; пейджинг/выделение/сортировка — UI. Сосед (не цель): KDicomQueueOptUi (панель кнопок), KDicomQueueSearch (свободно). Портировано 25 диалогов + 3 виджет-вью + KDialog + KProgressBar. ✅ KPasswordEdit ПОРТИРОВАН (режим `passwordedit`): диалог смены пароля (реф. @0x456048), KDialog немодаль, 320×240, SetKStyle(W320), титул SetTitle(TR_PEdit). Сетка: новый пароль (KPasswordLineEdit→QLineEdit, echo Password, maxLen16, tooltip TR_IAMCharacters) + подтверждение (то же) + ряд OK(QToolButton, выключена пока оба поля не заполнены)/Cancel. OnCheckInput (вкл. OK) + IsPasswordConfirmSame (сверка при OK) — чистый UI, реализованы; скриншот сверен. Кастом KPasswordLineEdit→QLineEdit(Password). GetKAccount(GetPasswordRegExp/ValidateIfPWValid/ConvertPasswordToMD5)/KSystemSet::SaveAccount — device. Cancel→close. Портировано 26 диалогов + 3 виджет-вью + KDialog + KProgressBar. ✅ KExamListSetupDlg ПОРТИРОВАН (режим `examlistsetup`): настройка списка обследований + путь экспорта на USB (реф. @0x7f97e0), KDialog немодаль, ФИКС 1024×712, SetKStyle(W1024), титул SetTitle(TR_MRLSettings), абс. геометрия. grp_list: label TR_LDisplay + 12 чекбоксов видимости колонок (пол/возраст/дата/направитель/эндоскоп/SN/ДР/тел/койка/№рег/2 польз) + подсказка TR_VITMRLWChecked. grp_path: label TR_EPath + 2 радио пути экспорта USB (ExportUdiskPath1/2, эксклюзив QButtonGroup, дефолт path1). Снизу Save/Exit. Все stock Qt, кастомов нет. Скриншот сверен. SaveSetupData(KExamListConfigHandler::SetExportPath/SetIsShow*)/LoadConfigData — device. Exit→close. Прим.: выделенных USB/backup-диалогов в бинарнике нет (KExportRecord/KUsbDevice/KUdiskStorageDevice/KFileBackup — headless-движки без Ui). Портировано 27 диалогов + 3 виджет-вью + KDialog + KProgressBar. ✅ KServerInfoDlg ПОРТИРОВАН (режим `serverinfo`): настройки сервера — реально конфиг DNS/прокси (реф. @0x5efe60), открывается из KAuthMachineDlg::SetServerInfo (замыкает петлю с 13-м портом). KDialog немодаль, 562×587, SetKStyle(W460), титул SetTitle(TR_SSettings2). Группа TR_SSettings2: DNS1:/DNS2: поля + TR_PServer: подпись + прокси/базовый-URL поле (label_proxy) + Save/Exit. Всё stock Qt (QLineEdit без валидаторов), кастомов нет. Скриншот сверен. InitWidgets (чтение endoinfoserver.ini)/Save (запись ini dns1/dns2/proxy/loginurl/endoinfoposturl + перезапись /etc/resolv.conf) — device. Exit→close. Портировано 28 диалогов + 3 виджет-вью + KDialog + KProgressBar. ✅ KColdlightAdjust ПОРТИРОВАН (режим `coldlightadj` — ВНИМАНИЕ: `coldlight` уже занят self-test'ом, ключ переименован; 2-я коллизия preview-ключа после reportedit): сервис/тюнинг источника света (реф. @0x747f68), KDialog немодаль, 450×1161, SetKStyle(W460), титул SetTitle(TR_LSConfiguration). 4 группы: (1) TR_LLTest — время лампы (hour[0-50000]h/minute[0-59]min + Save + суммарное); (2) 自动调光参数 — PID авто-диммера: 9 строк Up/Down double-спинов (pAgc..dAec, dec3 step0.001 max10, ialc_down без max), agc max[-6..27]/min[-6..18]/threshold[0..255] int, delt_a[-5..0]/r/b/p + overTh[0..1]/pwmBase[0..1000], самотест (checkBox start/random, комбо AGC/AEC статично, flickTh[0..5000]/range/fixStep[0..1000]); (3) TR_AIris — 7 device-меток замеров; (4) Save/Exit. Все stock Qt, кастомов нет; китайские подписи через fromUtf8. Скриншот сверен. SaveAutomaticDimmerParam/SaveLightUseTime/RefreahData/DimmingTest*/EndoScope-Camera-SystemStatus/value-метки — device. Exit→close. Портировано 29 диалогов + 3 виджет-вью + KDialog + KProgressBar. ✅ KScopeInfoEdit ПОРТИРОВАН (режим `scopeinfo`): редактор информации об эндоскопе (реф. @0x644cf0), KDialog немодаль, 600×919, SetKStyle(W460), титул SetTitle(TR_EInfo). Секции: frame_2 сервис-логин (SN/CN device-метки + Login/Logout + endoSN + import auth), тип (cmb_type device), спека TR_ESpecification (model/type readonly-edit+device-комбо, каналы/длины: 3 Φ-mm QDoubleSpinBox dec1 max25.5 step0.1 «Φ ...mm» + рабочая длина QSpinBox mm max5000 step5), статус KScopeStaus→плейсхолдер + stackedWidget(камера/скоп), frame_manu произв.инфо (дата 1970-2050/ESN/частота readonly), frame_user (contrastno maxLen16/comment maxLen20), Save/Exit. Скриншот сверен. Кастом KLineH×3→QFrame(HLine), KScopeStaus→плейсхолдер-фрейм. GetEndoScope(ShowEndoInfoCID/EepromSaveRet/OnSave→EEPROM/CID)/ClickImportAuthBin/UserLogin-Logout/LoadScopeInfo(комбо) — device; ValueChanged(dirty)/Exit→close — UI. ✅ KCameraInfoEdit ПОРТИРОВАН (режим `camerainfo`, камерный близнец KScopeInfoEdit): редактор инфо камеры (реф. @0x5fdf58), KDialog немодаль, 600×843, SetKStyle(W460), титул SetTitle(TR_CHInfo). Проще скоп-версии: frame_2 сервис-логин (SN/CN device + Login/Logout + CHSN + import) + тип (cmb_type статично TR_SLens/TR_HLens) + сетка model (readonly-edit + device-комбо) + esn (maxLen10, валидатор [A-Za-z0-9]{0,10}) + frame_manu пустой плейсхолдер + Save/Exit. Все stock Qt, кастомов нет. ПРИМ: в реф. service-виджеты скрыты по умолчанию (роль-гейтинг KAccount::CurrentRole — device); у нас показаны. Скриншот сверен. GetCamera(ShowCameraInfoSaveRet/OnSave→EEPROM)/ClickImportAuthBin/UserLogin-Logout/SN-CN из KSystemSet/cmb_model — device; ValueChanged/Exit→close — UI. Пара scope+camera info-edit закрыта. ✅ KLogView ПОРТИРОВАН (режим `logview`): постраничный просмотрщик лог-файлов (реф. @0x712f40), KDialog немодаль, 800×600, SetKStyle(W460), титул SetTitle(TR_LView). textBrowser (QTextBrowser, lineWidth2) + ряд навигации «<<»/«>>» (страницы, подключены на скролл — чистый UI) + btn_fullscreen(TR_FScreen, роль==4 device) + «|<<»/«>>|» (смена лог-файла). Все stock Qt, кастомов и device-IPC НЕТ — бэкенд только локальный QFile. Скриншот сверен. UpdateFileNameList/OpenNewFile/ShowLogText/Pre-NextPageView/Pre-NextLogView (файловая логика)/роль-гейтинг — заглушки. Портировано 32 диалога + 3 виджет-вью + KDialog + KProgressBar. ✅ KErrorRate ПОРТИРОВАН (режим `errorrate`): QC-диалог теста частоты битовых ошибок /误码率测试 (реф. @0x5e1768), KDialog немодаль, 284×728, SetKStyle(W460), титул 误码率测试. ВНИМАНИЕ: подписи — инлайн-tr() с китайским/ASCII текстом (НЕ TR_-ключи!), у нас через QString::fromUtf8. Группа 结果(результат device) + чекбокс 出错后停止调试 + параметры (интервал[1-60]/время h[0-99]+m[0-59]/счётчики) + группа 阈值 (пороги risk/NG [1-99]) + группа 错误统计 (register/Lane0/Lane1/CRC/ECC/SOT — device) + кнопки 开始/停止/暂停/继续 (min120×30). Все stock Qt, кастомов нет. Стейт-машина enable кнопок — чистый UI, реализована. Скриншот сверен. SlotStart/Stop/Pause/Continue(BER-тест)/Slot_timeOut/CalResult(QTimer, счётчики Lane/CRC/ECC/SOT)/value-метки — device. ПРИЁМ: часть диалогов X2000 — без TR_-ключей, чисто китайские инлайн-tr() (fromUtf8). Портировано 33 диалога + 3 виджет-вью + KDialog + KProgressBar. ✅ KStatisticInfo ПОРТИРОВАН (режим `statinfo`): панель статистики устройства — ВНИМАНИЕ: НЕ диалог, встраиваемый QFrame (реф. @0x7437a0, base QFrame, 667×279, StyledPanel/Sunken). Порт как самостоятельный QFrame. Плоская read-only сетка меток из 3 секций (заголовок+KLineH-разделитель, caption/value 140px): (1) TR_IProcessor — SN/остаток(RSpace)/видео(ATFVideo)/снимки(ANOSPictures); (2) TR_LSource — SN/лампа тек.(AWTOCMLamp)/всего(AWTOAMLamp); (3) TR_EInfo — SN эндоскопа/частота(FOUse) + кнопка btn_EndoInfo(TR_DInformaion, эмитит сигнал OpenScopeInfo — реализован). Нет таблиц/комбо/дат. Кастом KLineH×3→QFrame(HLine). Скриншот сверен. InitWidget (KSystemSet::GetProcessorSN/KEndoScope::GetEndoInfo-GetEepromData/GetMainCtrlThread::GetLamp*UsedTime) — device; значения-плейсхолдеры. Портировано 33 диалога + 4 виджет-вью + KDialog + KProgressBar. ✅ KSystemTemperature ПОРТИРОВАН (режим `systemtemp`): монитор температуры /温度监测 (реф. @0x5f9658), KDialog немодаль, 400×336, SetKStyle(W460), титул 温度监测. Подписи — китайский инлайн-tr() (fromUtf8). frame → сетка 3 строк caption/value (4EV温度/白灯温度/UV灯温度, значения device, vSpacing12) + Exit (центр спейсерами). Все stock Qt, кастомов нет. Скриншот сверен. UpdateTemperature (QTimer 1с: KPlControl::ReadValueFromPL 0xffa50a14 линейная °C + ColdLight::QueryLightTemperature) — device. Exit→close (в реф. не подключён). Портировано 34 диалога + 4 виджет-вью + KDialog + KProgressBar. ✅ KRigidEndoBtnGuide ПОРТИРОВАН (режим `rigidguide`): оверлей-гайд кнопок жёсткого эндоскопа — ВНИМАНИЕ: НЕ диалог, встраиваемый QWidget-оверлей (реф. @0x69d768, base QWidget, контент 540×370, абсолютная геометрия, 9 QLabel). Порт как самостоятельный QWidget. Фон — реальное фото эндоскопа rigidendo_btn_guide.png (theme::asset, локальная сверка) + подписи функций A/B (лево/право, короткое TR_S/долгое TR_L) + M (центр) + легенда TR_SSFSPLSFLPress + подсказка TR_PABOTCTCTWindow. Кастомов нет (все QLabel). Скриншот сверен (реальный ассет + метки на местах). GetEndoBtnFuncText(KUserOsdSet — имена функций кнопок)/onSetEndoBtnFuncText/SetVisible(глоб.флаг) — device; имена = фолбэк TR_EEOMenu. Портировано 34 диалога + 5 виджет-вью + KDialog + KProgressBar. ✅ KRecordCase ПОРТИРОВАН (режим `recordcase`, 40-й UI-класс): диалог записи автотест-кейса (реф. @0x7340f8), KDialog немодаль, 320×240, SetKStyle(W460), титул tr("Record Case"). Тексты — англ. TR-ключи (не китайские). Абс.геометрия: форма Module Name/Case Name (QLineEdit maxLen63, grid в layoutWidget 30,50,271,61) + чекбокс Record TimeStamp (60,130, checked) + кнопка Start Record (106,180). Все stock Qt, кастомов нет. Скриншот сверен. RecordCaseName (запись кейса в /home/root/system/autotest/casefile/)/ctor Mkdir — device. Без OK/Cancel. Портировано 35 диалогов + 5 виджет-вью + KDialog + KProgressBar. ✅ KNetPrintList ПОРТИРОВАН (режим `netprint`): диалог поиска сетевых принтеров (реф. @0x78cc90), KDialog+KObject немодаль, 646×490, SetKStyle НЕТ, титул SetTitle(TR_Sch) + тёмный setStyleSheet. frame → gridLayout_2 (label_msg TR_Schng.«поиск…» + m_pfindIpList QListWidget device-скан) + ряд кнопок Search/OK/Cancel (все стартуют disabled, скриншот отражает нач. состояние до скана). Все stock Qt, кастомов нет. Скриншот сверен. QTimer/StartSearch/LoadPrinterList/RefreshList (KHalPrinterAPI-скан)/ClickBtnOK(commit)/SubscribeMsg(0x2b2b) — device. Cancel→close. Портировано 36 диалогов + 5 виджет-вью + KDialog + KProgressBar. ✅ KHospitalInfoEditDlg ПОРТИРОВАН (режим `hospitalinfo`): диалог редактирования инфо больницы/предприятия (реф. @0x559790), KDialog немодаль, 674×540, SetKStyle НЕТ, титул SetTitle(TR_EHInformation) (перекрывает setupUi TR_HIEdit). Форма-грид (правые подписи): логотип (label_logo_img белый фон, pixmap device) + caption1 TR_Cptn1 (имя больницы, config, bg rgb39,39,41) + caption2 TR_Cptn2 (KMemComboBox→editable QComboBox maxLen30, bg rgb62,63,68) + textEdit_statement TR_Dcltn (декларация, bg rgb39,39,41) + ряд Default/Save/Cancel (фикс 156×42). Все stock Qt кроме KMemComboBox→QComboBox. Скриншот сверен. Логотип(KSystemSet::GetHospitaLogo)/cap1/statement/combo-items из report-template config(LoadDataFromReportTemplateConfig/InitQuickInput)/OnBtnSaveClicked(persist) — device. Cancel→close. Портировано 37 диалогов + 5 виджет-вью + KDialog + KProgressBar. ✅ KUserSrvSet ПОРТИРОВАН (режим `usersrvset`): диалог сервисных настроек (реф. @0x70c0b8), KDialog немодаль, 460×768, SetKStyle(W460), титул SetTitle(TR_Svce). groupBoxServer(TR_Fnctn): секции Log(Backup/View), MMControl(Processor/Endo), Otr(Upgrade/Recovery + videoCal/lightConfig — реф. открывают под-диалоги, часть уже портирована: KVideoCal/KColdlightAdjust). groupBoxInfo(TR_EInformaion): 5 строк caption/value (IPSN/лампа тек.-всего/ESN/FOUse — значения device). frameButton: центрированный Exit(fixed120). Все stock Qt, кастомов нет. Скриншот сверен. Слоты кнопок(ClickProcessorCtl/EndoCtl/BackupLog/ViewLog(→KLogView)/Recovery/Upgrade/LightConfig/VideoCal)/proxy-подписки/TimerTaskRecovery/InitWidget(info+роль-гейт lightConfig role<3) — device. Exit→close. Портировано 38 диалогов + 5 виджет-вью + KDialog + KProgressBar. ✅ KExamDetailInfoUi ПОРТИРОВАН (режим `examdetail`): панель деталей обследования (реф. @0x495c98). База KFullScreenDialog(parent,2004) → KDialog(FULLSCREEN) 1280×960, титул SetTitle(TR_Dtls). Слева widget_left_opts(фикс260): 4 action-кнопки 212² (Report/Export/Upload/Delete) + label_disk_vol(250², device) + Exit. Справа: шапка пациента (имя/дата) + widget_tableView (таблица файлов device→QTableView) + play-bar пагинации (Total/hint + head/pre/edit_page[validator]/«/1»/next/tail). Кастом: KFullScreenDialog→KDialog, KImgPushButton→QPushButton-глифы, таблица→QTableView. ПРИМ: 212²-размеры сжимаются т.к. device-рескейлер ResizeExamDetail заглушён — структура сверена. Скриншот сверен. Слоты(OpenReportEdit/Export/Upload/OpenDeleteDlg)/модель таблицы/disk-vol/SubscribeMsg(0x2f07/0x2f0b)/пагинация — device. Exit→close. Портировано 39 диалогов + 5 виджет-вью + KDialog + KProgressBar. ✅ KAlgParamAjustDlg ПОРТИРОВАН (режим `algparam`): диалог настройки параметров алгоритма /算法参数调试 (реф. @0x5c9a80), KDialog немодаль, 330×900, SetKStyle(W460), титул 算法参数调试. Подписи китайский+ASCII инлайн-tr() (fromUtf8). QStackedWidget 2 стр: page1 = SCL(特殊光, 3×3 hex-спины)/master-switch(算法总开关, checked в ctor)/CCM(3×3)/AWB(白平衡, T/para/cfg + up/r/wbr/dw/b/wbb); page2 = Knee/Gamma/Denoise(图像降噪, combo 低/中/高)/BrightBalance(亮度均衡) + ряд чекбоксов фич (SensorLUT/色彩增强/图像增强/电子去烟/宽动态) + Exit/> + конвертер定点浮点转换. ~31 QSpinBox(hex)/5 QDoubleSpinBox/13 QCheckBox. Все stock Qt, кастомов нет. pb_nextPage-переключение + конвертер fixed↔float (арифметика) — чистый UI, реализованы; скриншот page1 сверен. Slot_checkBox_*/Slot_pb_*_save(push в FPGA)/InitStatus/QTimer-поллеры — device. Exit→close. Портировано 40 диалогов + 5 виджет-вью + KDialog + KProgressBar. ✅ KAddPrinterDlg ПОРТИРОВАН (режим `addprinter`): диалог добавления принтера (реф. @0x7972f0), KDialog немодаль, 828×381, SetKStyle НЕТ, титул SetTitle(TR_APrinter) + тёмный setStyleSheet. Грид: имя(validator [A-Z_a-z0-9-]{1,16}) + тип подключения(combo WPrinter/UPrinter/NPrinter статично) + адрес(QStackedWidget: page0 URL-combo device / page1 IP-edit) + Search(150×31) + драйвер(readonly,NoFocus,border) + AddDriver + ряд чекбоксов(default img/report, report checked) + OK/Cancel. Кастом KIpLineEdit→QLineEdit+маска. ConnectType переключает stacked-страницу (Network→IP, иначе URL) — чистый UI, реализовано; скриншот сверен. OnOkBtnClicked(SavePrinter/AddUsb/Net/WinPrinter)/OnSearchBtnClicked(скан)/AddUsbPrinterToComBox(CUPS)/OnAddDriverBtnClicked(под-диалог KAddPrinterDriverDlg свободно) — device. Cancel→close. Портировано 41 диалог + 5 виджет-вью + KDialog + KProgressBar. ✅ KAddPrinterDriverDlg ПОРТИРОВАН (режим `adddriver`): диалог выбора драйвера принтера (реф. @0x792618), открывается из KAddPrinterDlg(AddDriver). KDialog немодаль, 1044×636, SetKStyle НЕТ, титул SetTitle(TR_ADriver) + тёмный setStyleSheet. Строка поиска (TR_DSearch + ledit maxLen128, h40) + две колонки-списка (m_pMakerListWidget TR_Mnfctrr 400×400 device / m_pPrinterListWidget TR_PDriver 600×400 device) + OK(disabled до выбора)/Install(minW180)/Cancel. Все stock Qt, кастомов нет. Скриншот сверен. Initialize/OnMakerListWidgetCurrentRowChanged(KPrinterManager map производитель→PPD, CUPS)/OnInstallBtnClicked(PPD с диска)/фильтр — device. OK/Cancel→close. ПОТОК ПЕЧАТИ ЗАКРЫТ: KSysPrinter→KAddPrinterDlg→KAddPrinterDriverDlg. Портировано 42 диалога + 5 виджет-вью + KDialog + KProgressBar. ✅ KPUserLoginDlg ПОРТИРОВАН (режим `puserlogin`): вход привилегированного/сервисного пользователя (реф. @0x5de170) — ОТДЕЛЬНЫЙ от KLogin (обычный вход) и KPasswordEdit. KDialog немодаль, 600×400, SetKStyle(W460), титул TR_DLogin (реф. windowTitle «Dialog» мёртвый). groupBox(TR_DLogin) с центрированной формой: account(TR_Acnt:, maxLen512, фокус) + password(TR_Pswd:, KPasswordLineEdit→QLineEdit echo Password, minW180, maxLen512, blacklist-валидатор) + ряд Login(TR_Lgn)/Cancel(TR_Lgt, оба 160). Все stock Qt кроме KPasswordLineEdit. Скриншот сверен. Login()(проверка учётки по БД) — device; CancelLogin→close, eventFilter(Enter→Login) — UI. Портировано 43 диалога + 5 виджет-вью + KDialog + KProgressBar. ✅ KProcessorControl ПОРТИРОВАН (режим `procctl`): диалог управления процессором/машинного контроля (реф. @0x73b870), открывается из KUserSrvSet(Processor). KDialog немодаль, 640×480, SetKStyle(W460), титул TR_PControl. Две группы: groupBox(TR_TControl) срок/остаток (TR_EDate:/到期时间, TR_Rmnng:/剩余天数 — device) + 开启/解除管控/TR_IAuthorization; groupBox_matchEndo(TR_CEControl) число эндоскопов (TR_TNOEndoscope:/镜体数量 device) + TR_Vw + 开启/解除管控/TR_IAuthorization. Снизу Exit. Только метки+кнопки, stock Qt. Смешанные подписи TR+китайский (fromUtf8). Скриншот сверен. ChangeTimeCtlState/ImportDelayLicense/ChangeEndoMatchCtlState/ImportMatchEndoLicense/ViewEndoScopeList/KControlINI::ReadMcTime-ReadMcEndo — device. Exit→close. Сервис-подграф: KUserSrvSet→KProcessorControl (сиблинг KEndoScopeControl свободен). Портировано 44 диалога + 5 виджет-вью + KDialog + KProgressBar. ✅ KEndoScopeControl ПОРТИРОВАН (режим `endoctl`, 50-й UI-класс сессии): диалог управления эндоскопом/машинного контроля (реф. @0x740130), сиблинг KProcessorControl, открывается из KUserSrvSet(Endo). KDialog немодаль, 640×480, SetKStyle(W460), титул TR_EControl. groupBox(TR_UOEControl): срок(TR_EDate:/截止日期)/остаток(TR_Rmnng:/剩余天数)/использований(TR_NORUses:/次数, значения device) + 开启/解除管控/TR_IAuthorization. groupBox_2(TR_CPControl): число совместимых процессоров(TR_CPAmount:/Num device) + TR_Vw + 开启/解除管控/TR_IAuthorization. Снизу Exit. Только метки+кнопки stock Qt, TR+китайский (fromUtf8). Скриншот сверен. ChangeUseTimesCtrlState/ImportDelayLic/ChangeMatchProCtrlState/ViewMatchProList/ImportMatchProLic/KControlProc/RecMainCtrl — device. Exit→close. Пара контроля (Processor+Endo) закрыта. Портировано 45 диалогов + 5 виджет-вью + KDialog + KProgressBar. ✅ KEndoScopeSN ПОРТИРОВАН (режим `endosn`): диалог ввода серийника эндоскопа (реф. @0x742110), KDialog немодаль, 640×480, SetKStyle(W460), титул TR_ESN. Центрирующая сетка 5×3 со спейсерами: заголовок TR_PETESNNumber: + строка SN:(литерал)/lineEdit (validator [A-Za-z0-9]{0,12}) + ряд OK/Cancel (фикс 100). Все stock Qt, кастомов нет. Скриншот сверен. onOK (SN>8 симв. → commit в KEndoScope + KMessageBox::warning) — device (пустой/короткий-чек оставлен UI, запись опущена). Cancel→close. Портировано 46 диалогов + 5 виджет-вью + KDialog + KProgressBar. ✅ KProcessorSN ПОРТИРОВАН (режим `procsn`): диалог ввода серийника процессора (реф. @0x6ff748), сиблинг KEndoScopeSN. KDialog немодаль, 640×480, SetKStyle(W460), титул TR_PSN. Центрирующая сетка со спейсерами: заголовок TR_PITPSN: + строка SN:/lineEdit (validator [A-Za-z0-9]{0,12}) + OK/Cancel (фикс 100). Все stock Qt. Диалог ЦЕЛИКОМ pure-UI: onOK — валидация(>8)+сохранение в m_SN, getter GetSetSN(); device-запись SN в процессор — в вызывающем коде. Cancel→close. Скриншот сверен. Пара SN-ввода (Endo+Processor) закрыта. ✅ KExamListViewUi ПОРТИРОВАН (режим `examlist`): вид списка обследований — ВНИМАНИЕ: НЕ диалог, встраиваемый QWidget+KObject-mixin (реф. @0x801468, base QWidget, 1630×1034), структурный близнец KDicomQueueViewUi. Порт как самостоятельный QWidget. grp_view → vbox → [widget_search (KExamListSearch→плейсхолдер), tableView (KTableView→QTableWidget, 20 колонок: чекбокс/PID/имя/пол/возраст/дата/направитель/врач/эндоскоп/SN/статус/ENo/картинки/видео/ДР/тел/койка/№рег/2польз), пейджер-бар (абс.геометрия: label записей/hint + home/pre/поле/номер/next/tail)]. Строки — device (БД). Кастом KTableView→QTableWidget, KPagePushButton→QPushButton(глиф), KPageLineEdit→QLineEdit, KExamListSearch→плейсхолдер. Скриншот сверен. OnGetExamListDataFromDB/OnQuery(БД)/SubscribeMsg×8/disk-capacity/custom-field заголовки — device. Соседи: KExamListSearch, KExamListOptUi (панель кнопок). Портировано 47 диалогов + 6 виджет-вью + KDialog + KProgressBar. ✅ KQRCode ПОРТИРОВАН (режим `qrcode`): панель отображения QR-кода — ВНИМАНИЕ: НЕ диалог, встраиваемый QFrame (реф. @0x745df0, base QFrame, 307×300, minSize300×300, StyledPanel/Raised). Порт как самостоятельный QFrame. Чистая display-панель: центрированное QR-изображение (label_QRCodePic фикс 260×260, scaledContents) + центрированная подпись (label_QRCodeInfo font-size:18px). Ни одного connect, кастомов нет. Обе метки пусты (device: SetQRCodePic/SetQRCodeInfo). Скриншот сверен. Портировано 47 диалогов + 7 виджет-вью + KDialog + KProgressBar. ✅ ReportConfigDlg ПОРТИРОВАН (режим `reportconfig`, ПЕРВЫЙ non-K-класс!): диалог настройки полей отчёта (реф. @0x4f2da8, имя БЕЗ K-префикса). KDialog немодаль, 1085×757, SetKStyle НЕТ, титул SetTitle(TR_Set). Верх: тип (combo TR_Exm 1/2/OMode3) + метка TR_Used (cyan-стиль). Центр widget_center (тёмная скруглённая панель rgb21): 3 секции чекбокс+метка полей 4 колонки — TR_PInfo (14: ENo/EmDate/EInfo/Stts/Nme/Gdr/Age/PID/Aplct/DoB/Tel/BNo/2польз), TR_Dgnse (6: VOExam/EConclusion/DName/OMode3/IFindings/Sgstn + 2 польз-поля с lineEdit), прочее (BSite/HP/ADoctor/Dctr) + подсказка TR_VITREWChecked. Снизу Save/Cancel. 26 QCheckBox+метки/2 QLineEdit/combo, всё stock Qt, DEVICE-слотов НЕТ (конфиг KReportEditUIConfig — заглушка). Скриншот сверен. Save/Cancel→close. ПРИЁМ: не все реф.-классы с K-префиксом (ReportConfigDlg) — грепать точное имя. Портировано 48 диалогов + 7 виджет-вью + KDialog + KProgressBar. ✅ KProgressDlg ПОРТИРОВАН (режим `progressdlg`): диалог прогресса операций с данными (реф. @0x451870, реф.-база QDialog → drop-in над KDialog, 512×278). РАЗБЛОКИРУЕТ KDataOprEventDeal (был блокер, см. §10). Фрейм umessage_frame_back (StyledPanel/Raised) → vbox: label_Text(TR_Prpng, AlignLeft|Bottom) + [lblCurrentProgress(TR_CFProgress) + progCurrentFile + lblTotalProgress(TR_TProgress) + progTotal] + спейсер + btn_Cancel(TR_Ccl, minW150, центр). Прогресс-бары border-стиль, value0, font-size18. Все stock Qt, кастомов нет. Скриншот сверен. ToDlgMsgDispatcher-сигналы (SigUpdate*Progress/Label/Hide кросс-поточные) — device; публичный API Set*Progress/Set*Label/SetTitle/DoModal/IsCancel — портируемый. btn_Cancel→close. Портировано 49 диалогов + 7 виджет-вью + KDialog + KProgressBar. ✅ KVideoPlayerOSD ПОРТИРОВАН (режим `videoosd`): OSD-панель управления видеоплеером — реф. QDialog frameless always-on-top оверлей (реф. @0x3d47d8); порт как самостоятельный QWidget (панель). vbox: label_file_name(placeholder «202405210003.mp4») + [horizontalSlider (QSS: groove rgb1,188,196 / add-page rgb84,88,88 / handle-image slider_inspecto.png) + hbox[время cur/total («00:19:23»/«/02:01:56») + 8 transport-кнопок LastFrame/NextFrame/LastVideo/Play-Pause/NextVideo/Speed/Save/Exit]]. Тексты — англ. placeholder-литералы (НЕ TR; device перезаписывает имя/время). Кастом KImageButton×8→QPushButton. Скриншот сверен (слайдер-QSS отрендерен). KVideoPlayer(декод)/2 QTimer/GetUIResolution/transport-слоты — device. btn_exit→close. Портировано 49 диалогов + 8 виджет-вью + KDialog + KProgressBar. ✅ KUnusedImgPlayBar ПОРТИРОВАН (режим `imgplaybar`): пейджер-бар браузера изображений — НЕ диалог, встраиваемый QWidget (реф. @0x4ef598, base QWidget, 1614×52). Порт как самостоятельный QWidget. gridLayout 1 ряд 8 кол: label_total_num(TR_TotalNum) + btn_head/btn_pre(48×32) + edit_page(48×32, numeric-validator) + label_total_page(«/1») + btn_next/btn_tail + спейсер. Кастом KImgPushButton×4→QPushButton(глиф). Наружу сигнал PageChanged. Скриншот сверен. RefreshPageNums/CheckFirstOrLastPage(KUnusedImgModel числа/enable)/ClickBtn*/OneditingFinished (навигация) — device. Портировано 49 диалогов + 9 виджет-вью + KDialog + KProgressBar. Осталось: ~72 UI-класса. ✅ KViewHardEndo ПОРТИРОВАН (режим `viewhardendo`): OSD-оверлей живого видео жёсткого эндоскопа (реф. @0x464a70, base QFrame, 891×596), reference-Ui оверлей (ОТЛИЧАЕТСЯ от нашего упрощённого video/KViewSoftEndo — тот paintEvent-вьювер). Абс. геометрия оверлеев: label_video(чёрн.) + frame_time(rec «00:00:00»/иконка/системное время) + frame_connect(gridLayout_4: статус/USB «U1»/«U2» font14) + frame_lefttime(gridLayout_3: TR_RTime+остаток серый) + frame_osd + widget_endobtnguide (KRigidEndoBtnGuide — переиспользован, показывает реальный ассет). Только QFrame/QLabel/grid. Скриншот сверен (оверлей + реальный button-guide). Содержимое меток (rec/часы/USB/остаток/OSD) через KUiMsgProxy — device-плейсхолдеры. Портировано 49 диалогов + 10 виджет-вью + KDialog + KProgressBar. Осталось: ~71 UI-класс.
ВАЖНО: чистые диалоги ИСЧЕРПАНЫ (субагент подтвердил — все свободные Ui_*::setupUi(QDialog*) уже в covered). Остаток UI = встраиваемые вью/оверлеи (device-heavy) + КАСТОМ-ВИДЖЕТЫ, которые мы ПОДСТАВЛЯЛИ throughout (KParamSetBtn/KLineH/KOsdSpin/KOsdMenuCell/KMemComboBox/KImgPushButton/KScopeStaus/KCounterTextEdit/KIpLineEdit/KPasswordLineEdit/KQuickInputComboBox/KOptionListButton/KGridWidget/KTableView/KPagePushButton). СЛЕДУЮЩИЙ ФОКУС: портировать реальные кастом-виджеты (заменить подстановки, поднять точность существующих диалогов).
✅ KParamSetBtn ПОРТИРОВАН (режим `paramsetbtn`, 60-й UI-класс, ПЕРВЫЙ реальный кастом-виджет): переиспользуемая панель кнопок настроек (реф. @0x685030, base QFrame, 420×114) — ранее ПОДСТАВЛЯЛАСЬ обычной панелью в KSystemSetDlg/KCustomEdit. 3 ряда: frame_custom(btn_custom1/2/3, текст через SetBtnCustomFuncName с elide+tooltip) + frame_page(скрыт, «<»/«n/m»/«>», показ при SetTotalPageNum>1, авто-enable на границах) + frame_btn(Default/Save/Exit). Сигналы: ClickBtnCustom(int)/PageChanged(int)/ClickBtnDeafault()(sic)/ClickBtnSave()/ClickBtnExit(). API: SetTotalPageNum/SetBtnCustomFuncName/HideCustom3Btn/SetBtnCustomVisible — ВСЕ реализованы. Всё stock Qt, вложенных кастомов нет; скины — внешний QSS по objectName. Скриншот сверен (демо: 3 стр + custom-имена, elide виден). GetSystemStatus (гейтинг custom1) — device-заглушка. ПРИЁМ: реальные кастом-виджеты — полноценные компоненты с сигналами/API, портировать как reusable (потом можно заменить подстановки в диалогах).
✅ KLineH + KLineV ПОРТИРОВАНЫ (режим `lineh`, 61-й UI-класс, ПАРА в одном файле KLineH.{h,cpp}): гравированные разделители (реф. @0x68c8e8/@0x68c7d8, base QFrame) — ранее подставлялись QFrame(HLine/Sunken) в ~8 диалогах. Ctor = ровно 3 вызова: setStyleSheet(<линейный градиент>) + min/max по фикс-оси 2px. Литералы градиентов перенесены дословно (@0x88b4c0): KLineH верх #010101→низ #3B3B3B (жёсткий стык 0.48/0.52), KLineV слева #3B3B3B→справа #010101. МЕТОДОВ/СИГНАЛОВ НЕТ (только moc-квартет+ctor/dtor). 100% PORT. Скриншот сверен (горизонт. канавка Above/Below + вертик. Left|Right).
✅ KPagePushButton ПОРТИРОВАН (режим `pagepushbtn`, 62-й UI-класс): иконочная кнопка пейджер-бара (реф. ctor @0x7b72e8, base QPushButton) — ранее подставлялась QPushButton+глиф в барах KDicomQueueViewUi/KExamListViewUi. Собств. сигналов НЕТ (штатный clicked()). ctor: setMouseTracking(true) (WA_MouseTracking). InitButton(QMap) @0x7b73e0 — ключи normalIcon/hoverIcon/disableIcon → грузит 3 QPixmap + setFixedSize(normal.size()). Кастомный paintEvent @0x7b7068 рисует 1 из 3 пиксмапов по состоянию (disabled→disable, hover-флаг→hover, иначе normal; отдельного pressed-пиксмапа НЕТ). Флаги pressed(+0xa8)/hover(+0xa9): mousePress/mouseMove(ручной hit-test)/focusIn/focusOut + RefreshBtnOfLeave() @0x7b72b0 (сброс при уходе курсора с бара). Идентичность кнопки НЕ зашита — 3 пути картинок из call-site. Скриншот сверен с РЕАЛЬНЫМИ ассетами прошивки (page_{head,front,next,tail}_{normal,hover,disable}.png 48×32, из mainapp/application/qss/icon/pageturning): head=hover(cyan)/front,next=normal/tail=disable — ВСЕ 3 состояния видны. ГРАБЛИ: GetReadOnlyBaseDir() возвращает путь БЕЗ хвостового слэша → конкатенация «syspreset»+«mainapp» ломает путь → пиксмапы null; чинить через QDir::absoluteFilePath (не строковый +).
✅ KCounterTextEdit ПОРТИРОВАН (режим `countertextedit`, 63-й UI-класс): редактор с лимитом длины + счётчиком (реф. ctor @0x5a7d10, base QTextEdit — НЕ контейнер). Ранее подставлялся QTextEdit+QLabel. Счётчик НЕ встроен — эмитится сигналом ChangeCounterShowText(«cur/max»), метку рисует внешняя форма. default cap=800; InitWidget(n) @0x5a7d50 (=SetMaxLength) ставит cap + connect textChanged→OnTextChanged (n<0 → no-op). OnTextChanged @0x5a8048: length()>max → left(max)+курсор в End+эмит счётчика; иначе эмит length(). GetCounterShowText @0x5a7df8 = «n/max». Считает QChar (UTF-16), CJK-safe. Текст I/O — штатный QTextEdit. 100% PORT. Скриншот сверен (43 симв. → обрезано до 30, счётчик «30/30»).
✅ KImgPushButton ПОРТИРОВАН (режим `imgpushbtn`, 64-й UI-класс): 4-состояний картиночная кнопка (реф. ctor @0x5a8ca8, base QPushButton). Родня KPagePushButton, но ОТЛИЧИЯ: конфиг InitButtons(normal,hover,checked,disable,QSize,bool) @0x5a8b78 — явные пути (не QMap); 4 состояния (+checked, без pressed); состояние из ШТАТНЫХ флагов Qt (isEnabled/isChecked/underMouse), собств. булей/mouse-оверрайдов НЕТ; ctor без WA_MouseTracking; нет RefreshBtnOfLeave; текста нет; пиксмап МАСШТАБИРУЕТСЯ в QRectF(0,0,drawSize). paintEvent @0x5a8e00: !loaded→штатная отрисовка, затем disabled>checked>hover>normal. checkable ctor НЕ включает (вызывающий сам). 100% PORT. ВЕНДОР-БАГ: на успешном InitButtons m_drawSize остаётся (-1,-1) → вырожденная отрисовка; в порте ставим drawSize=size и на успехе (помечено). Скриншот сверен (normal |◀ / checked ▶ / disabled — 3 форсируемых состояния, реальные ассеты).
✅ KIpAddrEdit + KIpLineEdit1 ПОРТИРОВАНЫ (режим `ipaddredit`, 65-66-й UI-классы, ПАРА в KIpAddrEdit.{h,cpp}): композитный IP-редактор + октет-эдит. Ранее подставлялся QLineEdit+inputMask "000.000.000.000;_". KIpLineEdit1 (реф. @0x67ab90, base QLineEdit): один октет — maxLength=3, без рамки, центр, без контекст-меню, QIntValidator(0,255), next/prev-указатели на соседей; focusIn→selectAll; keyPress «.»→next / BS в пустом→prev; textEdited: 2 цифры со значением ≥26 → авто-вперёд, вне диапазона → KMessageBox::warning+клип к 255. KIpAddrEdit (реф. @0x67acd0, base QFrame): 4× KIpLineEdit1 + 3 метки-точки «.» (objectName IP_LABEL_DOT), resize(200,40), tab-order+next/prev проставлены, дети позиционируются ВРУЧНУЮ в resizeEvent (без layout — координаты в реф. не извлечены, равномерная раскладка). text() @0x67be88 — джойн «a.b.c.d» ("" если любой пуст); SetText() @0x67c9a0 — QRegExpValidator (((2[0-4]\d|25[0-5]|[01]?\d\d?)\.){3}…) + split по «.». Сигналы наружу textChanged(QString)/textEdited(QString) реэмитят склейку. 100% PORT (KMessageBox::warning — наш враппер). ТРЕТИЙ вариант KIpLineEdit (single-field hand-masked, реф. @0x6f9030) — ОТЛОЖЕН (устаревший дизайн с ручной маской+KMessageBox, портировать только если конкретный диалог его инстанцирует). Скриншот сверен (192.168.1.100 в 4 боксах + точки + эхо джойна). ГРАБЛИ: QFrame без layout не имеет sizeHint → в компоновщике схлопывается; в реф. диалоги ставят его по геометрии (в превью — setFixedSize).
✅ KOsdSpin ПОРТИРОВАН (режим `osdspin`, 67-й UI-класс): OSD-спинбокс (реф. иерархия QFrame→KFrame→KOsdSpinBase→KOsdSpin, ctor @0x484050). КОНТЕЙНЕР (не подкласс QSpinBox): внутренний QSpinBox (native ± скрыты) + свои «-»/«+» + заголовок-метка в QGridLayout. Промежуточные KFrame/KOsdSpinBase СПЛЮЩЕНЫ в QFrame (добавляли device-навигацию аппаратными клавишами — DEVICE-STUB). setupUi @0x484330: фрейм фикс-ширина 250, grid(9,0,9,0); label_title(0) / spacer(1, схлоп при пустом title) / btn_sub «-» 30×30 NoFocus(2) / spin_value min-w120 NoFocus AlignCenter NoButtons(3) / btn_add «+» 30×30 NoFocus(4). Конфиг-структура KOsdSpinConfig{title,min,max,step,def,msgId,ctxId} → InitWidget @0x483f98 (setMin/Max/SingleStep/Value). ClickAdd/SubBtnAct: cur±=step→RefreshSpinStatus @0x483ef0 (клип+enable/disable ±кнопок на границах). SetIntValue @0x484250 — тихий сеттер (disconnect/setValue/reconnect). DEVICE-STUB: ValueChangedAct слал KUiMsgProxy::SendToMainCtrl(msgId,ctxId,val) → заменён на Qt-сигнал valueChanged(int). Скриншот сверен (Gain=65 через SetIntValue + безымянный спин def=3, native-стрелки скрыты). ✅ KOsdDoubleSpin ПОРТИРОВАН (режим `osddoublespin`, 68-й UI-класс): СИБЛИНГ KOsdSpin (реф. @0x47ff98) — идентичный контейнер/сетка, но внутренний QDoubleSpinBox «doubleSpinBox» + setLayoutDirection(LeftToRight) + setDecimals(n). Конфиг KOsdDoubleSpinConfig — double min/max/step/def; decimals в реф.-структуре явно не выделен → принят =1 из device-кодирования value×10 (1 знак). SetDoubleValue — тихий сеттер. DEVICE-STUB: ValueChangedAct слал SendToMainCtrl (int)(value*10+offset) → заменён на Qt-сигнал valueChanged(double). Реализован ИЗ СПЕКИ реверса KOsdSpin (без отдельного субагента). Скриншот сверен (MI=1.2 через SetDoubleValue, 1 знак, native-стрелки скрыты).
✅ KMemComboBox + KQuickInputWidget ПОРТИРОВАНЫ (режим `memcombobox`, 69-70-й UI-классы): редактируемый комбо с «памятью» + его find-попап. Ранее подставлялся editable QComboBox. KMemComboBox (реф. @0x690a48, base QComboBox): setEditable(true), кастомный find-попап вместо нативного дропдауна, QRegExpValidator-фильтр имени (@0x883068 — АППРОКСИМАЦИЯ ASCII-набором, точный 0x9b-литерал с unicode-fullwidth не транскрибирован). editTextChanged→OnRadarChange @0x690fe0 (антирекурсия: disconnect/ShowFindWnd/reconnect). SetTableName/SetDisplayId/currentText(trimmed)/text/setText(без попапа)/GetName/GetId. Записи «name - id» (сепаратор « - » @0x88ba80). Сигналы наружу itemSelect(int) @0x82b668 / FocusOut() @0x82b698. SelectFindWndItem @0x691390: флаг +0x34==0→id в поле, иначе name. KQuickInputWidget (реф. безрамочное Popup): QListView+QStringListModel, SetItems/MoveCursor/keyPress-навигация, клик/Enter→itemSelect(int). DEVICE-STUB: персистентность — зашифрованный SQLite (KDbSqlite, ключ SONOSCOPE_X2000_KEY, таблицы tb_QuickInputApplicant/Patient/Doctor + tb_Report) через KQuickInput*DbTableHandler::GetMatchDate(prefix) → вынесено за инъектируемый провайдер SetMatchProvider(fn); в превью — in-memory. Скриншот сверен (комбо «Jo» + попап 3 совпадения по префиксу с форматом «name - id»). ✅ KQuickInputComboBox ПОРТИРОВАН (режим `quickinputcombo`, 71-й UI-класс): MRU-комбо быстрого ввода (реф. @0x5a91e0, base QComboBox). СИБЛИНГ KMemComboBox, но проще: нативный дропдаун поверх модели (не find-попап), используется в редактировании отчёта. Init @0x5a9278 (реф. new KQuickInputModel+LoadData+setModel) — в порте DEVICE-STUB: загрузка через инъектируемый SetLoadProvider(fn), заполнение нативной item-модели. Save() @0x5a9358 — MRU-коммит currentText: дедуп (findText≥0 → return -1, ничего не вставлять), иначе insertItem(0) в начало (таймстамп — device-часть, опущен). showPopup @0x5a9730: emit SigIsShowPopup(true) + ленивое подключение currentIndexChanged→OnCurrentIndexChanged. Реализован ИЗ СПЕКИ реверса KMemComboBox (без отдельного субагента). KQuickInputModel (DB-бэкенд) уплощён на нативную модель. Скриншот сверен (дубликат «No abnormality found»→-1, новый «Biopsy taken» вставлен в начало, count 3→4).
✅ KOptionListButton ПОРТИРОВАН (режим `optionlistbutton`, 72-й UI-класс): сегментный переключатель (реф. ctor @0x3d9380, base QWidget). ВАЖНО: вопреки имени/подстановке — это НЕ попап/QComboBox, а полностью кастом-painted горизонтальный сегмент-контрол (все опции встык равными ячейками, клик по ячейке = выбор; попапа/меню/стрелки/ассетов НЕТ). Ранее ошибочно подставлялся QComboBox — реальный виджет иной природы и выше по точности. SetOptions @0x3d92c8 (авто-выбор index 0), SetIndex @0x3d8950 (emit IndexChanged только при изменении), Option/Options, SetOptionData/GetOptionData/GetCurOptionData/GetTheFirstOptionIndexWithData (QVariant на опцию), сеттеры цветов/шрифта/radius/borderWidth. IndexAtPos @0x3d8a30 (cellW=width/n, clamp); mouseRelease→выбор; mouseMove→тултип если эллипсис. paintEvent @0x3d9a78: AA→DrawBackground(градиент, выбранная ячейка highlight-кисть)→DrawText(эллипс+центр, выбранная highlight-цвет)→DrawGrid(скруглённая рамка radius=9 + вертикальные разделители). ResizeGradient (2 вертик. QLinearGradient) + ResizeMask (скруглённый QРath→QRegion→setMask, radius-3) в show/resizeEvent. Цвета из бинарника: normal #666666→#444444, highlight #141519, border #313131, text #FFFFFF, шрифт Arial 16. Сигнал IndexChanged(int) @0x819ec0. 100% PORT. Скриншот сверен ([2D|Color|PW] Color-выбран + [Freeze|Live] Freeze-выбран, рамка/разделители/градиенты/highlight).
✅ KOsdMenuCell ПОРТИРОВАН (режим `osdmenucell`, 73-й UI-класс): ячейка OSD-меню (реф. @0x4812a8, base QFrame — НЕ KOsdSpinBase). Отличия от KOsdSpin: derives напрямую от QFrame, БЕЗ SendToMainCtrl/сигналов (чисто визуальная), навигация — во владеющем KOsdMenu. Круглая иконка-кнопка + подпись + флаг «выбрано». setupUi @0x481838: grid 3×2 — frame_flag(0,1,rowspan3)→label_flag(10×10 flag_select.png), frame_icon(1,0)→hLayout+спейсеры+label_icon(min52/max76 scaledContents center), label_tag(2,0 center). Состояние — stylesheet-свап + fixed-width + свап пиксмапа (без paintEvent): UnSelect @0x481680 (button_normal.png, radius19, w110, иконка52), Select @0x4813f0 (button_select.png, radius26, w160, иконка76, флаг виден если showFlag&&!greyed), Focus @0x481128 (подпись cyan rgb(0,205,209) если не greyed). API: SetTitle/SetIcons/Select/UnSelect/Focus/UpdateUI/IsSelected/IsGreyed/SetGreyed/UpdateGreyedFlag/CheckGreyedCondition(virtual база false)/SetSubWindowPosition/SetLocatedMenu. clicked() добавлен (Qt-идиоматика; реф. шёл через KOsdMenu-навигацию). DEVICE-STUB: KDisplayOption::GetOsdIconPixmap → theme::asset("black/osd/…"). Скриншот сверен с РЕАЛЬНЫМИ ассетами (Equip normal grey / Preset selected cyan+флаг / Measure focused cyan-подпись).
✅ KScopeStaus + KScopeValue ПОРТИРОВАНЫ (режим `scopestaus`, 74-75-й UI-классы, ПАРА в KScopeStaus.{h,cpp}): кликабельная панель статуса эндоскопа + дочерний виджет спеков. Ранее подставлялся заглушкой-QFrame. Имя реально с опечаткой «Staus». KScopeStaus (реф. @0x68cf70, base QFrame): 3 стек-ребёнка — KScopeValue(105×120) + label иконки биопсийного канала(105×88) + teal-метка модели(105×40 @ (0,215), «font-size:17px;color: rgb(0,153,153);», скрыта до подключения). SetScopeModel @0x68cc00: путь = KEncStyle::getBiopsyImg(model) (РЕАЛЬНЫЙ чистый метод — сборка пути, ini не читает), если KEndoScope::IsEndoHasChannel(model) (static) → очистить иконку, иначе setPixmap; модель → KEncStyle::GetEndoDisplayModel (РЕАЛЬНЫЙ, маппинг без ini). SetScopeSN/SetScopeConnect/SetShowSpareFlag/SetPicCheck (репозиция детей). Сигналы наружу clicked() @0x82a920 + SetInstrumentChannel/DistalEnd/BendingSection(double) — пробрасываются в одноимённые слоты KScopeValue. KScopeValue (реф. @0x68d578, base QWidget): 5 меток (model/sn/channel/distal/bending), AlignCenter, «font-size: 12px;»; caption-префиксы спеков — ИНФЕРЕНС (точные литералы не извлекались). DEVICE-STUB: живое чтение сцена (GetEndoModel в ctor, GetEndoScope()->GetEndoInfo()) — не тянется, данные подаёт вызывающий. Скриншот сверен (EC-X20L + SN12345678 в KScopeValue 12px + teal-модель 17px внизу). ГРАБЛИ: та же C7-родня — QFrame с ручной геометрией без layout схлопывается в компоновщике (в превью setFixedSize).
✅ KPasswordLineEdit ПОРТИРОВАН (режим `passwordlineedit`, 76-й UI-класс): поле с regexp-фильтром (реф. @0x690200, base QLineEdit). Ранее подставлялся QLineEdit(Password). ВОПРЕКИ имени: НЕ контейнер с «глазом» и НЕ ставит echoMode(Password) — просто QLineEdit с живым regexp-фильтром; echoMode — ответственность вызывающего. Единственный член QRegExp m_regexp (+0x30). setValidator @0x690318 ПЕРЕОпределён: из QRegExpValidator берёт QRegExp (сам QValidator отбрасывается), null→no-op, странный guard проверяет валидность ТЕКУЩЕГО m_regexp (воспроизведён точно). OnTextChanged @0x690368: обрезать хвост (chop) пока не exactMatch(regexp), переписать при изменении. 100% PORT. Скриншот сверен («12a34» + \d* фильтр → «12», echo=Password показывает 2 точки).
✅ KTableView + KTableModel + KLineDelegate ПОРТИРОВАНЫ (режим `tableview`, 77-79-й UI-классы, троица в KTableView.{h,cpp} + структура KHeaderProperty): кастомная таблица. Ранее подставлялась QTableWidget. KTableView (реф. ctor @0x7bc278, base QTableView): ставит модель+делегат в InitTableView @0x7bc990 (НЕ в ctor); SelectRows/SingleSelection/NoEditTriggers, скрытый vertical header, sort-индикатор. InitTableView: headers→KTableModel, defaultSelCol≥0→KLineDelegate, SetTableColumnWidth (per visible), setSortIndicator(defaultSelCol,Descending), resizeColumnsToContents+клип к width. keyPressEvent @0x7bcb58: Up/Down→selectRow±, Tab→SigToFocusOutCurrentView, F2/Enter-сигналы. KTableModel (QAbstractTableModel, пейджинг): rowCount=rowsPerPage, columnCount=видимые; data DisplayRole→row[key], TextAlignment→center(0x84), CheckState col0; страница не в кэше→emit SigGetDataFromDB(page); flags col0 UserCheckable(0x31) остальные 0x21 (НЕ editable); setData CheckState тоггл mainKey в чек-сет+SigHeaderCheckBoxStateChange; SetModelData(page,rows). KLineDelegate (QStyledItemDelegate): col0 фон #1a1a1a + 1 из 4 PNG (checked/unchecked×hover, реальные ассеты patient/checkbox/ из ProjectPresetPath); editorEvent col0-клик→тоггл, dblclick→SigToDoubleClick, Space→SigToSpaceKeyPress. KHeaderProperty{title,key,width,visible}. Сигналы KTableView: SigGetDataFromDB/SigHeaderCheckBoxStateChange/SigToDoubleClick/SigToFocusOutCurrentView/SigToKeyPressF2Event/SigToKeyPressEnterEvent. DEVICE-STUB: пейджинг-данные из caller/DB через SigGetDataFromDB→SetModelData (в превью — фикстуры); чекбокс-ассеты — реальные. Скриншот сверен (чекбокс col0 checked row0 + selectRow blue row1 + sort ▼ ID + скрытая колонка исключена + 6 строк на страницу).
ИТОГ: 79 UI-классов за сессию (49 диалогов + 10 вью + 17 кастом-виджет-юнитов: …/KPasswordLineEdit/KTableView+KTableModel+KLineDelegate), все скриншот-сверены, регрессия 106/106 всю сессию. ВЕХА: 50 UI-классов за сессию, все скриншот-сверены, регрессия 106/106 всю сессию. ИТОГ UI-фазы: 40 UI-классов за сессию (35 диалогов над KDialog разных стилей W320/W460/W700/W1024/FULLSCREEN + 5 встраиваемых QWidget/QFrame-вью), все скриншот-сверены, регрессия 106/106 всю сессию; пойманы коллизии имён (KVideoCal C6, KFactoryOptions C5) и preview-ключей (reportedit→reporteditui, coldlight→coldlightadj). УРОК: имя из бинарника + грепать до порта. Иконка btn_close — реф. close.png (device asset), у нас текст ✕. Существующие
UI-классы строят layout ВРУЧНУЮ (без Ui_-файлов), стили — через `theme::` (qss прошивки).

**(историческое) СЛЕДУЮЩИЙ ЗАХОД ТРЕБОВАЛ РЕШЕНИЯ ПОЛЬЗОВАТЕЛЯ О НАПРАВЛЕНИИ** (device-bound):
UI-порт (131 Qt-Widgets-класс, тестируется скриншотами, не self-test), OSD-меню панели 8″
(нужно решение по железу), DCMTK-сеть, GStreamer live-video, HAL/HW. Все — не чисто-off-device;
либо нужен реверс IDatabase + разрешение коллизий для реф.-DB-слоя (парал. существующему).

**ПОСЛЕДНЕЕ (итерация 4): под-элементная CRUD-модель `KDocumentGenerator`**
(self-test `docgen` расширен, класс **12/33**). Два Qt-free метода-ядра (четыре UI-обёртки
Add/Delete/Update/ClickSubItem — Qt через InitDocument/ChangeItemSelected, ждут документной
итерации):
- **`AddSubItemData`** @0x540bf0 — вставка копии item в список под parentId по позиции pos
  (0→в начало, ≥размера→в конец; реф. 3 ветки = один insert в min(pos,size)). parentId==""→
  корень; промах родителя → лог+выход. Дедуп: если id уже есть → RemoveSubItem(parentId,
  item.m_strName) перед вставкой (КВИРК: RemoveSubItem без root-фолбэка → в корне дедуп
  no-op). Слияние cfgMap: upsert трёх полей. Хвост (сверено СЫРЫМ asm): оба find по
  **item.m_strID** — `item.m_strID.find(STR_REF_IMAGE_TEXT_MAP)` ‖ `STR_REF_IMAGE_TEXT_MAP_EXT.
  find(item.m_strID)` → SyncRefresnImageItemData. Возврат ulong (норма -1) реф. игнорируется→void.
- **`DeleteSubItemData`** @0x53fb10 — id==""→из корня убрать ВСЕ узлы с m_strID==item.m_strID
  (цикл не прерывается); иначе RemoveSubItem(id, item.m_strName). Каскад конфигов (сверено
  СЫРЫМ asm — needle **item.m_strID**, НЕ parentId): стереть все ключи m_mapItemConfigs,
  содержащие item.m_strID как ПОДСТРОКУ (сносит сам элемент + детей по префиксу; квирк — "/CC"
  тоже попадает под подстроку "/C"; сиблинги с иным префиксом целы). Хвост по item.m_strID.
**ИСПРАВЛЕНО:** в итерации 2 `imgSyncOk` печатался, но НЕ входил в `ok` docgen (image-sync
тесты не гейтили PASS — латентный пробел, ровно про который §4). Добавлено `&& imgSyncOk
&& crudOk`.

**ПРЕДЫДУЩЕЕ (итерация 3b): ЗАКРЫТ оркестратор `KDocumentGenerator::SyncRefresnImageItemData`**
@0x53efa0 (опечатка Refresn; self-test `docsync` на РЕАЛЬНЫХ синглтонах прошивки).
Весь кластер синхронизации колонок image-text-map теперь закрыт (класс **10/33**).
Поток: templName (выбранный шаблон) → libName=GetTempletLibName → libData=GetTemplateLib(libName)
→ refKey=`/RT_IMAGE_TEXT_MAP` (при templName=="NP-4x1" → EXT-префикс) → top=FindRefItem;
proto=копия первой колонки ДО очистки; top.children очищаются; для каждого ребёнка libTop —
resolve, SyncImageItemContent(proto,copy), push; затем SyncImageItemParam(libData).
**КЛЮЧЕВОЕ (сверено СЫРЫМ asm — декомпилятор путал стек-слоты sp+0x108 templName /
sp+0x128 libName):** с "NP-4x1" сравнивается **templName** (сырое имя), а в GetTemplateLib
идёт **libName** (префикс "ReportTemplate…", совпадает с ключами-группами). Тест: группа
ReportTemplateNP-1x4 включает Image1x4.xml → поддерево из 4 колонок MAP0..MAP3; богус-колонка
после пересборки исчезает. Все три Qt-free метода-помощника (GetAllItemIDs/SyncImageItemContent/
SyncImageItemParam) + GetTempletLibName из предыдущих итераций работают в связке.
**RENDER-КАРКАС ЗАКРЫТ (итерация 5): минимальный InitDocument для TEXT_BLOCK работает
end-to-end** (self-test `initdoc`, класс **16/33 = 48%**). `GetTextDocument`/`InitDocument`/
`PutFooterOnBottom`/`InsertBlockLineAfterItem` + `KRTCreatorContext::GetFontSize` (обе
перегрузки). ctor KDocumentGenerator ТЕПЕРЬ создаёт m_pContext (реф.), добавлен dtor +
запрет копирования. `initdoc` строит реальный QTextDocument из 2 текстовых блоков и сверяет:
оба блока отрисованы, формат rootFrame (bg white/margin 20/width 100%), стиль-различие
(ThirdTitle Bold vs ReportText), пересборка чистит документ (текст не двоится). GetFontSize
сверен дизасмом: база QFont("Source Han Sans CN",16,Normal), DPI-скейл setPointSize(qRound(
pt×physicalDotsPerInch/150)), item-ветки FontType/стиль-по-типу (RT_TITLE_TABLE_BLOCK→
FourthTitle, RT_TEXT_BLOCK→ReportText). ОТСТУПЛЕНИЕ (помечено): InsertBlockLineAfterItem —
документированный no-op (выравнивание футера по низу; тянет FindFrameOrCell + метрики
QTextDocumentLayout), на контент не влияет. Change*Selected/Move*/UpdateBlock (Qt-редактор) — следующие UI-заходы.

**✅ ТВОРЕЦ ИЗОБРАЖЕНИЯ (итерация 6): `KRTImageItemCreator`** (self-test `rtimage`,
`app/src/report/KRTCreatorContext.cpp`). Модель `KImageBlock` уже была. Обёртка
CreateBlock(item*,frame*) @0x536240 — как у текста (вложенный фрейм + ElementId), рисующая
renderImage @0x535c40: `KImageBlock::Url(valid)` — если файл не загрузился (valid=false) →
НИЧЕГО не вставляется (пустой блок прячет HideInvalidBlock, off-device без картинок не падает);
иначе QTextImageFormat (setName=url [ImageName 0x5000], setWidth/Height если >0 [0x5010/0x5011])
+ QTextBlockFormat (Alignment: Left→VCenter|Left 0x81, Center→HCenter 0x04, Right→VCenter|Right
0x82 — LEFT/RIGHT добавляют VCenter, CENTER нет; keep-маркер UserProperty+2), insertBlock+
insertImage. GetRatioTo1K НЕ применяется (размеры в device-px). Тест: RegisterPicPath(source→
PNG) → изображение вставлено (name/160/120); несуществующий файл → гейт (не вставлено).
⚠ В прошивке KRTImageItemCreator наследует KRTTableItemCreator (child-frame таблицы); у нас
иерархия уплощена — фрейм-формат как у текста + ElementId (на визуал min не влияет).
**✅ ТВОРЕЦ ТАБЛИЦЫ (итерация 7): `KRTTableItemCreator`** (self-test `rttable`) —
обслуживает RT_TABLE_BLOCK + RT_TITLE_TABLE_BLOCK (самые частые: 93+89 в поставке).
CreateBlock(item*,frame*) @0x53a400 → createTable @0x53a6f0 (QTextTableFormat: border=
BorderWidth [0x4000], borderBrush=BorderColor [0x4009], colWidthConstraints [0x4101],
ElementId [0x100001]; insertTable(rows,cols); ShowTitle→rows+1 + mergeCells(0,0,1,cols) +
insertTitle в строку 0) → createChild @0x539d08 (проход по m_lstSubItems, позиция idx/cols ×
idx%cols, +1 строка при hasTitle, cellAt → m_context.CreateBlock(type,item,cell) РЕКУРСИЯ в
творцов). Добавлена перегрузка `KRTTextItemCreator::CreateBlock(item*, QTextTableCell&)`
@0x53b798 (текст прямо в ячейку). Тест: 2×2 grid с Alpha/Beta/Gamma/Delta в порядке +
формат (border/colw) + title-вариант (merged-строка заголовка сверху, данные ниже).
ОПУЩЕНО для min (помечено): single-column CreateFrame-ветка (всегда таблица), cellSpacing/
margins, cellat-переопределение, split-line, модель в UserProperty, removeRows-обрезка,
полное форматирование заголовка (insertTitle=insertText(FullText)).
**ОСТАЛОСЬ по творцам:** SubData (KRTSubDataItemCreator, вложенный фрейм-таблица), ImageGroup
(KRTImageGroupCreator, сетка снимков), image/table-в-ячейку перегрузки, KTableBlock/KImageBlock
метатипы. ТРИ из 5 типов блоков рендерятся (text/image/table = 481 из ~500 вхождений поставки).

**✅ РЕДАКТОРНАЯ ПОЛОВИНА, примитивы поиска (итерация 8): `FindFrameOrCell` +
`GetSelectFrame`/`GetSelectCell`** (self-test `findcell`, класс **19/33 = 58%**).
`FindFrameOrCell` @0x53ca28 — рекурсивный обход дерева фреймов QTextDocument, ищет элемент с
ElementId==id (property UserProperty+1 на frameFormat фрейма ИЛИ format ячейки). Таблицы
(qobject_cast<QTextTable*>) — по ячейкам + рекурсия во вложенные фреймы ячеек; обычные фреймы —
по childFrames (QTextFrame::iterator). Совпал фрейм → *outFrame; ячейка → outCell (взаимоискл.).
`GetSelectFrame`/`GetSelectCell` @0x53d1b0/@0x53d160 — поиск по m_strCurItemId от rootFrame.
Заодно доставлена штамповка ElementId на ЯЧЕЙКИ в createChild (реф. GetCellWithID @0x539c20,
ранее опущено) → ячейки теперь находимы. Тест: находит текст-фрейм/таблицу/ячейку, промах,
сентинел. Всё Qt-чистое.
**✅ ВЫДЕЛЕНИЕ (итерация 8b): `ChangeFrameSelected`/`ChangeCellSelected`/`ChangeSingleItemSelected`/
`ChangeItemSelected`** (self-test `selectitem`). Выделение = серый фон #a0a0a0 (BackgroundBrush
0x820), рамка не трогается; sel→setProperty, !sel→clearProperty. ChangeSingleItemSelected пишет
m_strCurItemId только при успехе. ChangeItemSelected расширяет id до набора (SynColumnID →
GetAllItemIDs).
**✅ UI-ОБЁРТКИ (итерация 9): `AddSubItem`/`DeleteSubItem`/`UpdateSubItem`/`ClickSubItem`/
`ChangeLayout`** (self-test `docedit`) — мутация данных + InitDocument + выделение. БЕЗ новой
декомпиляции (зависимости готовы).
**✅ ПЕРЕМЕЩЕНИЕ (итерация 10): `MoveFront`/`MoveBack`** (self-test `movefb`) — std::swap
СОДЕРЖИМОГО соседних узлов данных + InitDocument (фреймы напрямую не двигаются). Гейт по
сентинелу m_strCurItemId (НЕ по m_bCanMove-флагам — те лишь для UI); границу (первый/последний)
проверяют сами; после свопа выделяется переехавший id. Класс ~26/33.

**OFF-DEVICE-ЯДРО KDocumentGenerator ПО СУТИ ЗАКРЫТО.** Работает сквозной цикл: данные
(CRUD/image-sync/оркестратор) → рендер (text/image/table + InitDocument, валидировано на
реальном шаблоне `initdocreal`) → поиск (FindFrameOrCell) → выделение → UI-обёртки →
перемещение. **✅ ПРАВКА ВНЕШНЕГО ВИДА (итерация 11): `ChangeTxtColor`/`ChangeBgColor`/`ChangeFontSet`**
(self-test `changecolor`). ВАЖНО (реф.): цвета красят ВЕСЬ документ (не выделение!) —
ChangeTxtColor: QTextCursor(Document)+mergeCharFormat(ForegroundBrush) + дубль строкой
m_mapConfigs["FontColor"]=color.name(); ChangeBgColor: rootFrame BackgroundBrush +
m_mapConfigs["BgColor"]=name (тот же ключ, что читает InitDocument). Оба БЕЗ InitDocument.
ChangeFontSet: upsert набора в m_mapItemConfigs (перезапись полей ПОЛНОСТЬЮ, не merge), тоже
без хвоста. Класс ~29/33.
**ОСТАТОК (мелочи/редкое, off-device-ядро закрыто):** `UpdateBlock` (перерисовка одного фрейма
через KRTCreatorContext::UpdateBlock, сейчас диспетчер-заглушка), `InsertBlockLineAfterItem`
(футер-падинг — нужны метрики QTextDocumentLayout), редкие творцы ImageGroup/SubData (в поставке
НЕ встречаются), image/table-в-ячейку перегрузки, Q_DECLARE_METATYPE для round-trip редактора.

**ОСТАВШАЯСЯ разведка (для истории; каркас уже закрыт выше):**

**ПЛАН RENDER-ИТЕРАЦИИ (минимум для TEXT_BLOCK, всё Qt Gui/Widgets — тестируемо в ui_preview):**
Раскладка KDocumentGenerator подтверждена: +0x00 QTextDocument* m_pDoc, +0x08
KRTCreatorContext* m_pContext, +0x10 KReportTemplateDataNew* m_pData, +0x18 std::string.
- `GetTextDocument` @0x53eac0 — ТРИВИАЛЬНЫЙ: `m_pDoc = new QTextDocument(parent); InitDocument();
  return m_pDoc;`. ✅ чистый Qt.
- `InitDocument` @0x53e108 — ОПОРНЫЙ. Работает по СУЩЕСТВУЮЩЕМУ m_pDoc (не создаёт новый):
  assert(m_pDoc/m_pContext); `m_pDoc->clear()`; фон корневого фрейма из attr "BgColor"
  (QColor::setNamedColor) + rootFrame format setMargin(20.0) + QBrush + QTextLength(100.0);
  дефолтный шрифт из GetFontSize; PageCount=m_pData[0x40], если >0 — QTextCursor(rootFrame),
  beginEditBlock, ГЛАВНЫЙ ЦИКЛ по m_pData->m_lstItems: строит type-строку, подстановка
  номеров страниц (ветки по report_preview::NP_1x4), `m_pContext->CreateBlock(type, item,
  rootFrame)`, между страницами InsertSplitLine; endEditBlock; `PutFooterOnBottom()`. ⚠️ Qt +
  GetFontSize (экран).
- `KRTCreatorContext::CreateBlock(type,item,QTextFrame*)` @0x5466e8 — ДИСПЕТЧЕР: m_creators.find(type),
  фолбэк на TEXT_BLOCK, `creator->CreateBlock(item, frame)` (виртуальный). ✅ (реестр/скелет
  KRTCreatorContext УЖЕ ЕСТЬ — коммит 730823e; нужен только render-путь).
- `KRTTextItemCreator` — ✅ СДЕЛАН (self-test `rttext`, в `app/src/report/KRTCreatorContext.cpp`).
  Обёртка `CreateBlock(item*, QTextFrame*)` @0x53b8b0: KTextBlock + вложенный QTextFrame с
  ElementId в property UserProperty+1 → рисующая `renderBlock(KTextBlock const&, QTextCursor&)`
  @0x53b3b0: QTextCharFormat (Bold→setFontWeight(75), Italic, FontPointSize=Size×KScreenMng::
  GetRatioTo1K, ForegroundBrush из FontColor), QTextBlockFormat (Alignment), insertBlock+
  insertText(FullText); затем HideInvalidBlock(inner)+(frame). property-id (FontWeight 0x2003,
  Italic 0x2004, PointSize 0x2001, Foreground 0x821, BlockAlign 0x1010, ElementId 0x100001)
  сверены сырым asm — ставятся высокоуровневыми Qt-сеттерами. ✅ добавлены `KTextBlock::FontColor`
  (@0x555030: styleAttr "FontColor"→setNamedColor, всегда true) и `KRTCreatorContext::
  HideInvalidBlock` (@0x546168: setVisible(false) для пустых/невалидных блоков без keep-флага
  UserProperty+2). ОТСТУПЛЕНИЕ (помечено): KTextBlock-как-QVariant-метатип в property
  UserProperty(0x100000) опущен — round-trip-машинерия редактора, на визуал не влияет,
  требует Q_DECLARE_METATYPE (добавить при Change*Selected). Тест rttext рисует блок в
  QTextDocument и сверяет weight/italic/pointSize(18×1.333=24)/color/align/ElementId.
- `KRTCreatorContext::GetFontSize` @0x547690/@0x547400 — QFont("Source Han Sans CN",16,0x32);
  без спец-конфига QGuiApplication::primaryScreen()->physicalDotsPerInch()+setPointSize; иначе
  attr FONTTYPE. ❌ экран/DPI — для min можно упростить до фикс-шрифта (пометить отступление).
- **`KScreenMng`** — ✅ СДЕЛАН (self-test `screenmng`, `app/src/ui/KScreenMng.*`). ОКАЗАЛСЯ
  НЕ железом, а ЧИСТЫМ Qt (QGuiApplication::primaryScreen) → реимплементирован faithfully,
  не заглушкой. sizeof 0x20: +0x00 QSize m_mainResolution (дефолт 1920×1080 → перезапись
  реальным экраном), +0x08 widthRatio=W/1920, +0x10 heightRatio=H/1080 (кламп к 1.0 только
  при ≤0), +0x18 m_ratio=widthRatio (его отдаёт GetRatioTo1K). Синглтон call_once. Под
  offscreen читает offscreen-экран (в тесте 2560×1440 → ratio 1.333). Формула сверена дизасмом
  ctor (@0x4b9d10: fdiv/fcsel hi).
- Заглушки/вспом.: `PutFooterOnBottom` (Qt via InsertBlockLineAfterItem), `GetSplitLineInfo`
  (есть? проверить), `InsertSplitLine` (только при PageCount>1 → для min no-op).
Творцы Image/ImageGroup/Table/SubData (0x536240/0x535a08/0x53a400/0x537438) — ВНЕ min-итерации
(тянут QTextTable/QTextTableCell/QImage, но тоже чистый Qt) — добавлять после каркаса.
ИТОГО min InitDocument для TEXT_BLOCK: ~7 своих методов. ✅ KScreenMng + KRTTextItemCreator
СДЕЛАНЫ. ОСТАЛОСЬ: `GetTextDocument` (тривиальная фабрика), `InitDocument` (опорный),
`GetFontSize` (упростить до фикс-шрифта, пометить), `PutFooterOnBottom` (Qt via
InsertBlockLineAfterItem).
✅ **РАСХОЖДЕНИЕ РАСКЛАДКИ РАЗРЕШЕНО (дизасм InitDocument):** наш `KReportTemplateDataNew`
ВЕРЕН, менять НЕ надо. Разведка спутала внутренние члены STL с полями: «PageCount @+0x40» —
это `std::list::_M_size` (C++11 list = 0x18 байт: +0x30 next, +0x38 prev, +0x40 size), т.е.
**число элементов m_lstItems**, НЕ страниц; «SplitLineInfo @+0x48» — наш `m_mapItemConfigs`
(GetSplitLineInfo("/", m_mapItemConfigs, out) достаёт KSplitLineInfo по ключу "/").
Подтверждена и раскладка KDocumentGenerator (+0x18 — доп. член-строка).

**СПЕКА InitDocument @0x53e108 (готова к реализации, min TEXT_BLOCK):**
1. assert(m_pDoc/m_pContext); `m_pDoc->clear()`.
2. Формат rootFrame: `bg = m_mapConfigs["BgColor"]` → QColor::setNamedColor; `fmt=root->
   frameFormat(); fmt.setMargin(20.0); fmt.setProperty(BackgroundBrush 0x820, QBrush(c,Solid));
   fmt.setProperty(FrameWidth 0x4003, QTextLength(PercentageLength=2, 100.0)); root->setFormat(fmt)`.
3. `m_pDoc->setDefaultFont(m_pContext->GetFontSize(nullptr))`.
4. `if (m_lstItems.size()>0)`: цикл по m_lstItems → `m_pContext->CreateBlock(item.m_strType,
   &item, root)` (ОБЯЗАТЕЛЬНО); между элементами InsertSplitLine при split.width>0 [ОМИТ min].
5. `PutFooterOnBottom()`.
ОСТАЮТСЯ ДЛЯ InitDocument (следующий заход): `GetTextDocument` (тривиально), `GetFontSize(item*)`
(для null-item: QFont("Source Han Sans CN",16,50)+DPI setPointSize; item-config-ветка FONTTYPE
— упростить/дизасм), `PutFooterOnBottom` (Qt via `InsertBlockLineAfterItem` @0x53dc90 — тоже
создать; no-op когда футера нет). МОЖНО ОМИТ для min: сброс члена-строки @+0x18, подстановка
номеров страниц (__to_xstring "%lu", NP_1x4/GetParam), GetSplitLineInfo/InsertSplitLine,
begin/endEditBlock. Диспетчер CreateBlock УЖЕ вызывает творца (проверено `rttext`).

**ПОСЛЕДНЕЕ (эта сессия): KDocumentGenerator итерация 2 — слой синхронизации колонок
image-text-map** (self-test `docgen` расширен, `app/src/report/KDocumentGenerator.*`).
Взяты ТРИ Qt-free метода, восстановленных ДЕКОМПИЛЯТОРОМ Ghidra на hermes (класс теперь
**9/33**):
- **`GetAllItemIDs`** @0x53fdb0 — все id колонок, зеркалящих данную (+ сама). Needle-глобал
  реф. это **`"/RT_IMAGE_TEXT_MAP"` СО СЛЭШЕМ** (сверено дизасмом статических инициализаторов,
  .bss a97938); голое `"RT_IMAGE_TEXT_MAP"` — ДРУГОЙ глобал `STR_TOP_IMAGE_TEXT_NAME`.
  НОРМАЛИЗАЦИЯ: id контейнера MAP0 → cur=его родитель, с сохранением формата
  (relative→relative, MAIN_CONTENT-prefixed→prefixed; 4 глобала — MAP/MAP0/MAP_EXT/MAP0_EXT).
  Не-image id → результат ровно `[cur]`. Иначе: матчи по `attrs["SynColumnID"]==cur` (в
  порядке ключа map), затем ОДИН push самого cur ПОСЛЕДНИМ.
- **`SyncImageItemContent`** @0x53c490 — из target выкидываются суб-элементы, чьё
  **`m_strName`** (НЕ id!) нет среди суб-элементов proto. `this` в теле не используется.
- **`SyncImageItemParam`** @0x53eb20 — `GetSubItemsParam(libData,"/RT_IMAGE_TEXT_MAP",tmp)`;
  для каждого конфига с атрибутом `SynColumnID`: значение SynColumnID есть ключом у нас →
  копия конфига под своим itemId; нет → запись по itemId стирается (устаревшая колонка);
  без SynColumnID → пропуск.
- Атрибут вендора **`SynColumnID`** (орфография SYN, НЕ SYNC; 92 вхождения в поставке).
**ЗАКРЫТО (итерация 3a): `KReportTemplateManager::GetTempletLibName`** @0x5999d0
(self-test `reporttmplmgr` расширен). По имени шаблона → имя содержащей библиотеки:
обход `m_vecTempletLibInfos`, для каждой либы — её депты (реф. GetAllDeptDefault →
map<string,bool>, у нас `depts`, ОТСОРТИРОВАН по имени = тот же порядок); ключ депта
БЕЗ первых 3 символов (dept-префикс "KW_") сравнивается с templName; ПЕРВОЕ совпадение →
`out = TempletName()` либы, немедленный возврат. ПРОМАХ: реф. голым return НЕ трогает out
(caller предынициализирует — воспроизведено, out не очищаем). Квирк: реф. `substr(3)` без
guard роняет на ключе <3 символов — у нас пропуск (депты поставки всегда "KW_*").
Проверено: "NP-1x4"→"ReportTemplateNP-1x4", промах сохраняет сентинел.

**ЗАКРЫТО (итерация 3b): `SyncRefresnImageItemData`** — см. блок «ПОСЛЕДНЕЕ» выше
(нестыковка "NP-4x1" разрешена сырым asm: сравнивается templName, не libName).
refKey по умолчанию `"/RT_IMAGE_TEXT_MAP"`, при `libName=="NP-4x1"` →
`"/RT_MAIN_CONTENT/RT_IMAGE_TEXT_MAP"` (STR_REF_IMAGE_TEXT_MAP_EXT). `ChangeLayout`
@0x5405e0 и `PutFooterOnBottom` @0x53e078 — оркестраторы, тянут QTextDocument-слой
(InitDocument/InsertBlockLineAfterItem) → ждут документной половины.

**ПРЕДЫДУЩЕЕ (89-режимная позиция ниже сохранена для истории):** ЧЕСТНАЯ МЕТРИКА ПОКРЫТИЯ — `docs/COVERAGE.md` (генерится
`python3 tools/coverage.py > docs/COVERAGE.md`): **485 классов / 6431 метод в референсе,
затронуто 86 классов / 910 методов (14.2%)** (report_template 28/36 = 78% — свободные функции
namespace по сути закрыты, остаток «36» — Qt-виджет KLineEdit). Это нижняя оценка (считает совпадение имён;
~9 наших классов имеют свой API и показывают 0% при рабочем коде). По доменам:
CORE 33.3%, DB 29.8%, MISC 23.9%, REPORT 15.6%, DICOM 12.5%, UPDATE 10.2%, UI 2.0%, HW 0.7%.
Off-device-ядро Фаз A/B/C/D закрыто в основном. KVideoProxy 57/116.
**СНЯТЫ С DB-РОАДМАПА (реверс показал ложный домен — эвристика имён в COVERAGE.md врёт,
это НЕ DB-классы):** `KUpdateAction` — на самом деле **KDialog** (диалог прошивки:
UI-оркестрация поверх движка KUpdateConf; решение «что обновлять» уже у нас в KUpdateManifest,
а реальная запись раздела — KUpdateConf::ExecUpdCmd → KHalClass::execUpdateAction = DEVICE/HAL).
`KImportRules` — на самом деле **KDialog** (UI-диалог импорта правил
logcheck: USB `<usb>/rules/` → редактирование в textEdit → копия в `system/autotest/logcheck/
rulesfile`; тянет KDialog+KUsbDevice). `KRecordItem` — **OSD-ячейка меню** (KOsdMenuCell-
GreyedWhenCameraDisconnected: кнопка старт/стоп видеозаписи, TR_Rcd/TR_RStop, серая при
отключённой камере/USB; тянет KCamera/KUsbDevice/KUiMsgProxy). Оба — в UI/OSD-фазу, не сейчас.
ОБЩИЙ УРОК (уже отмечался на KFactoryOptions/KDataOprEventDeal): перед взятием кандидата из
COVERAGE.md ОБЯЗАТЕЛЬНО проверять дизасмом typeinfo/базы — «домен» там угадан по имени и врёт.

**ПОСЛЕДНЕЕ (эта сессия, 12-я итерация): KLcdProxy — семантический слой панели 8"**
(self-test `lcdproxy`; `app/src/ui/KLcdProxy.*`, `KUiMsgProxy.*`, генератор
`tools/gen_lcdproxy.py`). Третий и последний крупный класс, снятый аудитом с пометки
«device». Обычный QObject, sizeof 24; **сигналов и слотов НЕ ОБЪЯВЛЯЕТ ВООБЩЕ**
(methodCount == 0, Q_OBJECT только ради qt_metacast). Синглтон отдаёт свободная
функция **`Get_KLcdProxy()` — с ПОДЧЁРКИВАНИЕМ**, в отличие от GetEndoScope()/GetCamera().
Работы с железом нет: единственные PLT-вызовы — QObject/QString/tr/new/delete.
**ТАБЛИЦА ДИСПЕТЧЕРИЗАЦИИ `m_lcdActTab`** извлечена `tools/gen_lcdproxy.py`:
1968 байт = **82 записи по 24 байта** (24, а не 16, потому что указатель на метод
в Itanium ABI занимает 16 байт). Все 82 указателя разрезолвлены в имена, коллизий
пар (key, act) нет. Генератор проверяет инварианты (pad == 0, adj == 0, невиртуальность).
**СТРУКТУРА `_KeyVlaue`** (ОПЕЧАТКА ВЕНДОРА сохранена) — 4 байта: ключ + значение,
причём значение это результат GetKeyStatus, **УСЕЧЁННЫЙ до 16 бит**.
**КВИРКИ ОРИГИНАЛА, ВОСПРОИЗВЕДЁННЫЕ 1:1 (покрыты тестом):**
- Диспетчер ищет по **ПАРЕ (key, act)**, причём `act` сравнивается как ОДИН БАЙТ,
  хотя хранится как quint16; граница цикла **ЗАШИТА как 0x52**, а не sizeof/24.
  Промах → **2** (без лога и без обработчика по умолчанию), попадание → 1.
- **Расширение знака параметра проверяет ТОЛЬКО БИТ 7** и игнорирует биты 8..31:
  `0x180` превращается в **128**, а не в -128. Латентный баг вендора.
- Ключи 525..536 несут **ДВА назначения**: act 2/3/4 — вызвать кнопку, act 7 —
  ПЕРЕНАЗНАЧИТЬ её функцию. Поэтому поиск обязан идти по паре.
- У ключа 3: **LONG = сброс параметров, SHORT = ОТМЕНА сброса** (и запись LONG стоит
  в таблице ПЕРЕД SHORT). Ключ 13 имеет ТОЛЬКО длинное нажатие; ключ 24 использует
  act **7**, а не 3, в отличие от всех прочих панельных клавиш.
- Один обработчик на разных ключах: SaveImage на (11,SHORT) и (35,SHORT),
  StartOrStopVideoRecord на (11,LONG) и (36,SHORT), OpenOrCloseVersionDialog на
  (2,LONG) и (33,SHORT).
- `GetHardEndoSystemSet1Params` — ЧИСТЫЙ tail-call алиас Soft-варианта (отдельная
  точка входа, сохранена).
- `GetHardEndMainViewParams` добавляет ключ **0x0F НАПРЯМУЮ, в обход GetKeyStatus**
  (в switch'е его нет вовсе).
- Ключ **0x217 НАМЕРЕННО пропущен** и в GetKeyStatus, и в наборе HardEndoUserSet2.
- Ножные переключатели — ПОЛНЫЙ no-op при ViewType == 1.
- Группа ключей 0x04-0x08/0x17 отдаёт уровень **+1**: панель ждёт 1-based, конфиг 0-based.
- Ключ 0x104 отдаёт **сентинел 0xFFFE** (в панель уходит именно 0xFFFE, не -2).
**ТРИ ИМЕНИ-ОБМАНКИ (продолжение серии):** `CheckIsLightAdjustEnable` — **вообще не
предикат**: тело это «если GetEndoType() == 13, послать сигнал», возвращаемое значение
мусорное и ВСЕ четыре вызывающих его игнорируют (реализовано как void).
`SetTimeFormat` **ставит формат ДАТЫ** и никогда не трогает 12/24-часовой режим.
`GetDateFormateIndex` — «Formate» опечатка; промах по списку молча даёт 0.
**ИСПРАВЛЕНО В СУЩЕСТВУЮЩЕМ КОДЕ:** порядок значений enum'ов `KUserOsdSet::Button`
и `Press` приведён к референсу — **A=0, B=1, M=2** и **Long=0, Short=1** (было
{M,A,B} и {Short,Long}). Доказательство — смещения в реф. ReadVideoParamConfig
@0x4048b8. **На чтение/запись это НЕ влияло** (значения использовались только как
метка switch при сборке имени ключа ini, self-test `osdset` как проходил, так и
проходит), но сломало бы упакованный индекс `button*2 + press`, который нужен
KLcdProxy. Добавлена перегрузка `GetButtonFunctionId(int)`.
Также в KSystemStatus добавлены сигналы `UserSetChange`/`SystemSetChange` и обёртки
`ChangeUserSet`/`ChangeSystemSet` (в реф. сигналов ТРИ, у нас был один).
**ОТСТУПЛЕНИЕ ОТ ОРИГИНАЛА (помечено в коде):** реф. `SetTimeFormat(int)` индексирует
QList **БЕЗ ПРОВЕРКИ ГРАНИЦ** (`d->begin + arg`) ⇒ любое значение вне 0..3 читает
указатель за границей и портит кучу. Поставлена проверка 0..3.
**✅ ДОЗАКРЫТО (та же сессия, дизасм-проход по обработчикам):** коды сообщений ВСЕХ
~63 обработчиков ВЫВЕРЕНЫ дизасмом и проставлены. Ключевые: freeze=9, chb=10,
imgenh=(12,param), colorenh=(11,param), zoom=(8,param), iris=(28,param),
tone=(20,**255**) — arg2 ЛИТЕРАЛ, не param; save=4, record=2, usb=39, wb=13,
power=0, connect=1, moire=30, window=(45,param), airpump=(48,param),
reset=(37,1)/cancel=(37,0), colorR/B/C=(26, idx0/1/2, param),
imgEnhL*=(24, idx, param), colorEnhL*=(25, idx, param), zoomL*=(27, idx, param),
Remote*/Foot*=(43, код 0x20D..0x218, param). GainSwitch: обе шлют **7**, гейт по
PanelType (short при 1, long при 0). **БЕЗ dispatch (только запись состояния/лог):**
OpenLamp/CloseLamp (SetLampStatus), Add/SubLampLevel (АБСОЛЮТНЫЙ уровень: гейт SS
LowLight → SetImageBrightness иначе SetLightLevel; Add и Sub БАЙТ-ИДЕНТИЧНЫ),
SwitchAuto/ManuLightAdjust (SetDimmingType), SwitchVlsMode (SetVlsMode),
StartTrans/SwitchAirPumpLevel (только лог), SetLanguage/SetVLSGroup/SetConnerShape/
SetResolution/SetVideoSplit (конфиг + ChangeSystemSet 0x101/0x103/0x104/0x105/0x106),
OpenOrCloseVersionDialog/EndoInfoDialog (PanelKeyVersion/EndoInfo), FileView.
Выборка кодов покрыта self-test `lcdproxy`. ⚠️ БЫЛА ОШИБКА В МОЕЙ ПЕРВОЙ ГЕНЕРАЦИИ:
Remote-кнопки сгенерировал как 2-арг, а они 3-арг (43, код, param) — как ножные;
исправлено, поймано тестом.
**✅ ДОЗАКРЫТО (13-я итерация):**
- **ИСПРАВЛЕНА МОЯ ОШИБКА:** ключи 0x213-0x218 возвращали СЫРОЙ ID функции, а реф.
  оборачивает результат в **`FunctionIdToIndex`** — панель ждёт ПОЗИЦИЮ в списке
  функций, а не идентификатор. Добавлен `KUserOsdSet::FunctionIdToIndex`
  (реф. @0x403e60: funcId > 11 -> 0, промах по списку -> 0; отрицательный в реф.
  читает за границей — у нас отсекается). Покрыто тестом.
- **Ключ 0x101 заработал:** добавлены `KSystemSet::GetSystemLanguage/SetSystemLanguage`.
  Это ЦЕЛОЕ-энум под тем же ключом `Common/Language`, а не строка:
  **0=Chinese 1=English 2=Spanish 3=Italian 4=French 5=Russian 6=German 7=Polish**;
  геттер КЛАМПИТ в 1 (English) при значении < 0 или >= KProjectSet::LanguageMode().
- **Режим `lcdproxy` перенесён в TMP_MODES** в `tools/selftest.sh`: он теперь ПИШЕТ
  конфиг (SetSystemLanguage), а значит обязан работать во временном ENDO_ROOT.
  Поймано регрессией (89/89 → 88/89 и обратно) — ровно тот случай, о котором
  предупреждает §4: пишущие режимы нельзя пускать по прошивке.
**ОСТАЛОСЬ (честный остаток):**
- **GetKeyStatus покрывает не все ключи**: 0x00/0x01/0x07/0x08/0x19/0x103/
  0x105/0x106/0x201-0x20C/0x20D-0x212 падают в дефолт 0, потому что у нас ещё нет
  источников. Точные реф.-выражения для КАЖДОГО выверены дизасмом и записаны в
  коде GetKeyStatus — осталось добавить аксессоры.
  ⚠️ **СТРУКТУРНОЕ РАСХОЖДЕНИЕ, ТРЕБУЕТ ОТДЕЛЬНОГО ЗАХОДА:** наш `_KUserConf`
  (`sys/KUserSet.h`) хранит btnA/B/M Long/Short + footSwitch1/2, а реф. `KUserSet`
  оперирует ПЛОСКИМ массивом `RemoteSwitch/Switch1..4` (+0x00..+0x0c) и
  `FootSwitch/Switch1..2` (+0x10/+0x14) — это, похоже, РАЗНЫЕ наборы (кнопки ручки
  живут в KUserOsdSet). Пока НЕ трогаю: нужен точечный реверс, чтобы не сломать
  рабочий `userset`. Нужно добавить: `KVideoSet::{GetImgEnhValue,GetColorEnhValue,GetZoomValue}`,
  `KSystemSet::{GetSystemLanguage,GetCornerShape,GetResolutionType,GetSaveVideoSplit}`,
  `KColdLightConfig::GetUserVLSConfig`, `KUserSet::{GetButtonFunctionId,GetPedalFunctionId,
  FunctionIdToIndex}`, `KUserOsdSet::FunctionIdToIndex`. Список в коде.
- **НАЙДЕНО В ЗАВИСИМОСТЯХ (чинить при реализации):** `KUserSet::GetButtonFunctionId`
  (граница 5, база +0x00) и `GetPedalFunctionId` (граница 3, база +0x10) проверяют
  границу ЗНАКОВО ⇒ отрицательный индекс проходит и читает ЗА границей массива.

**ПРЕДЫДУЩЕЕ (11-я итерация): KCamera** (self-test `camera`,
`app/src/endo/KCamera.*`, генератор `tools/gen_camera.py`). Второй класс из тех, что
аудит 10-й итерации снял с пометки «device». Работы с железом НЕТ ВООБЩЕ — весь внешний
вызов это LogPrintf(Ex)/KAccount::CurrentRole/KEncStyle::IsCameraValid/usleep.
Обычный QObject, sizeof 0x78, синглтон через СВОБОДНУЮ функцию `GetCamera()`, 7 сигналов.
Карта серий (**ровно 7 записей, ОДНА карта**) извлечена `tools/gen_camera.py`
(переиспользует extract_slots из gen_endoscope.py и сверяет число пар с числом вызовов
fromAscii_helper).
**ЧЕМ ОТЛИЧАЕТСЯ ОТ KEndoScope (важно — не копировать вслепую):**
- карта серий ОДНА (у эндоскопа две: основная + «ухань»);
- **Get{Camera,Sensor,Firmware}Type возвращают ЖЁСТКИЕ КОНСТАНТЫ 8 / 2 / 2**, а НЕ
  делегируют в KEncStyle, как четвёрка Get*Type у KEndoScope. Это главное расхождение;
- `ExtractFixDataPage` БЕЗ параметра длины;
- структура инфо — 5 обычных QString, без числовых полей и без QStringList;
- контрольной суммы в классе нет вообще (её считает вызывающий HmiMcu тем же
  makeCRC4Endo, который, напомню, на деле XOR, а не CRC);
- **ПОРЧИ ПАМЯТИ НЕТ** — оба пути разбора ограничены по месту вызова, так что,
  в отличие от KEndoScope, никаких отступлений от оригинала не потребовалось.
**КВИРКИ ОРИГИНАЛА, ВОСПРОИЗВЕДЁННЫЕ 1:1 (покрыты тестом):**
- **`IsNeedRollover()` — ИМЯ ВВОДИТ В ЗАБЛУЖДЕНИЕ** (та же ловушка, что и
  `IsEndoModelHaveSuffix`): это не счётчик и не «переворот», а признак «модель РОВНО
  `10-100-201`» (серия 4I6), выставляемый точным регистрозависимым сравнением.
- `m_nCameraInfoSaveRet` инициализируется **1**, и `SetCameraStatus(0)` тоже ставит **1**.
- **`ExtractFixDataPage` ПРОПУСКАЕТ байты 0x0b/0x0c**: синий гейн читается с 0x0d.
  Похоже на сдвиг на два байта, но так в бинарнике; `SaveFixParam` зеркалит это,
  оставляя 0x0b/0x0c нулями. Тест ловит ловушку (0xEEEE в пропущенных байтах не
  должен попасть в bGain).
- Годность страницы: флаги НЕ 0x00 и НЕ 0xFF (вхождение в {0,0xFF} ⇒ FALSE).
- `GetCentorPointStart` считает по firmwareType, но тот жёстко 2 ⇒ **всегда (1,7)**;
  оба диапазона всегда 0.
- `ExtractCameraInfo`: строки NUL-ТЕРМИНИРОВАННЫЕ (strlen), а не фиксированной ширины;
  `.trimmed()` только у модели; байты **0x1c..0x21 ПРОПУСКАЮТСЯ**.
- `IsCameraMatch`: роль > 2 — обход ДО любой валидации; серийник проверяется ТОЛЬКО
  на непустоту, его значение не сравнивается никогда; белый список — точное
  регистрозависимое равенство (НЕ подстрока и НЕ поиск по карте серий).
- `ResetCameraEepromData` даёт НЕнулевые дефолты: центр (16,16), rGain 18000, bGain 11800.
- `ClearCameraInfo` пишет литерал `"Cleared by R&D!"`.
- Лог-опечатка сохранена: `"Camera: Read centor point: (%d,%d).\n"` (centor, не center).
**НЕ РЕАЛИЗОВАНО (осознанно):** `ClearCameraEepromData` — в реф. 512 эмиссий сигнала
с `usleep(30000)` каждая, ≈15 с блокировки (та же картина, что у KEndoScope).
**НЕ УСТАНОВЛЕНО:** назначение трёх строк _CameraInfoStruct +0x10/+0x18/+0x20
(смещения и длины известны точно); тип возврата GetCentorPointStart (QPoint или
std::pair) — у нас out-параметры.
**ПОЛЕЗНО:** `GetCamera()` имеет **86 мест вызова в 62 функциях** (KCameraInfoEdit,
KVideoProxy, AlgParaManager, KVideoCal, KExamBussinessHandler, KFactoryOptions и др.) —
класс разблокирует широкий срез. Заодно подтвердилось допущение в комментариях
KExamBussinessHandler: GetCameraInfo() отдаёт модель на +0x00 и серийник на +0x08.

**ПРЕДЫДУЩЕЕ (10-я итерация): АУДИТ ПОМЕТОК «device» + KEndoScope**
(self-test `endoscope`, `app/src/endo/KEndoScope.*`, генератор `tools/gen_endoscope.py`).
**СНАЧАЛА — СИСТЕМНЫЙ АУДИТ.** После того как пометка «device» оказалась неверной
у K3ADimming, проверены дизасмом ВСЕ классы, помеченные в ROADMAP Фазы E как
«нужен прибор». Результат: **пометки неверны у большинства**.
| Класс | Реальная поверхность железа | Вердикт |
|---|---|---|
| KEndoScope (77) | **НЕТ ВООБЩЕ** (ни open/ioctl/mmap/read/write, ни I2C) | off-device |
| KCamera (38) | **НЕТ**; весь внешний вызов — KEncStyle/KAccount/LogPrintf/usleep | off-device |
| KLcdProxy (103) | **НЕТ**; это СЕМАНТИЧЕСКИЙ слой над транспортом, не транспорт | off-device |
| KEndoScopeControl, KProcessorControl, KSysPrinter | KMessageBox/QTableWidget | это **UI**, Фаза F |
| KHalPrinterAPI (12) | 12 сквозных Hal_*@plt | **ДЕЙСТВИТЕЛЬНО device** |
| KComDataReceiveThread (5) | read@plt — вот откуда байты MCU | **ДЕЙСТВИТЕЛЬНО device** |
⇒ Фаза E верна примерно для **20 методов, а не для ~600**. Строка «прибор: LCD/сенсор/
камера/USB/принтер/MCU» в COVERAGE.md — угадана по именам и врёт для всех трёх HW-строк.
**РЕАЛИЗОВАН KEndoScope.** Обычный QObject (vtable 14 слотов), sizeof 0x80; синглтон
отдаёт СВОБОДНАЯ функция `GetEndoScope()`, а не GetInstance. Байты EEPROM/CID приходят
ПАРАМЕТРОМ, наружу идут 12 Qt-СИГНАЛОВ — потому и проверяем off-device.
**ТАБЛИЦЫ ИЗВЛЕЧЕНЫ ГЕНЕРАТОРОМ** `tools/gen_endoscope.py` (эмулятор AArch64 в стиле
gen_keysym): карта моделей→серий **40 записей** + отдельная «ухань»-карта **РОВНО
ИЗ ОДНОЙ** записи (ED-5GT→NJE). Промах по карте → ПУСТАЯ строка, сентинела нет.
**ТРИ ПОЛЯРНОСТИ ВЫВЕРЕНЫ ДИЗАСМОМ (не угаданы):** у IsSuperfineEndo (6 записей) и
IsEndoHasChannel (4 записи) результат `QStringList::contains` возвращается НАПРЯМУЮ ⇒
вхождение означает TRUE; сравнение ТОЧНОЕ и РЕГИСТРОЗАВИСИМОЕ. IsVideoCalReveral —
подстроки "430"/"500" ИЛИ ПОЛНОЕ равенство "ED-5GT".
**КВИРКИ ОРИГИНАЛА, ВОСПРОИЗВЕДЁННЫЕ 1:1 (покрыты тестом):**
- **`ExtractEndoinfoFromdata` разбирает модель в ЛОКАЛЬНУЮ переменную и теряет её**:
  поле sModel очищается и НЕ заполняется. Ставит модель только CheckEndoInfo.
- **`IsEndoModelHaveSuffix` — ИМЯ ЛЖЁТ**: это `model.indexOf("430") != -1`,
  регистрозависимый поиск ПОДСТРОКИ, никаких суффиксов.
- **`ISEndoScopeMatch` НЕ содержит логики "*N" вообще** (она в KEncStyle::IsScopeValid
  и CheckEndoInfo). Роль > 2 — привилегированный обход всех проверок.
- **`makeCRC4Endo` — ВОПРЕКИ ИМЕНИ НЕ CRC**: 8-битный бегущий XOR, seed 0, без полинома,
  по байтам 0x00..0x7e, сверяется с 0x7f. На страницах 508/511 контрольной суммы НЕТ ВООБЩЕ.
- **Второй параметр ВСЕХ четырёх Extract*-методов МЁРТВ** (регистр сразу переиспользуется).
- **`ExactMatchProcessorEepromData` чистит белый список ДО валидации** — мусорная
  страница стирает хороший список без отката.
- **`SaveMatchProcessor2Eeprom` пишет в байт длины ПОЛНЫЙ размер списка**, а записей
  сохраняет не более 5 ⇒ страница может заявлять 7 при пяти данных.
- **`AddEndoUsedCount`: декремент остатка выполняется ДО раннего выхода** — при
  usedCount == 0xFFFF он молча теряется; а 0xFFFE инкрементится ровно в сентинел 0xFFFF.
- `GetEndoGlassType` дефолтит **3** (а не 0); `GetRotateType` — 0. Четыре Get*Type
  делегируют в KEncStyle и НЕ проверяют указатель, а эти два — кэш-геттеры и проверяют.
- `ExtractExtraFixParam` шлёт сигнал для roundCut и videoCap, но для deadline — НЕТ.
- `ResetEepromData` НЕ трогает черновую страницу; wbRed/wbBlue дефолтят 18000/11800.
**ДВА ОСОЗНАННЫХ ОТСТУПЛЕНИЯ ОТ ОРИГИНАЛА (НЕ квирки, помечены в коде):**
1. В реф. `ExactMatchProcessorEepromData` НЕ ограничивает pData[1] (uchar) и игнорирует
   переданную длину ⇒ читает до ~3 КБ ЗА границей 64-байтовой страницы, и любой EEPROM
   с байтом 0 == 0xAA впрыскивает мусор кучи в белый список доступа. Воспроизводить
   порчу памяти нельзя — у нас длина передаётся явно и OOB не допускается.
2. `GetZoomRatio` в реф. даёт 1.36f при superfine **И** `KSystemSet::GetCornerCutSize
   (model) == 1`. Второй половины условия у нас нет (метод не реализован) ⇒ при
   superfine-модели с иным размером обрезки мы вернём 1.36f, а реф. — fZoomRatio.
   Дореализовать вместе с KSystemSet.
**НЕ РЕАЛИЗОВАНО (осознанно):** CheckEndoInfo/CheckEndoControl (нужен KControlProc::
GetDeadline и роли), ClearEepromData (в реф. 512 страниц с usleep(30000) ≈ 15.4 с
блокировки), OpenEndoControl/ReleaseEndoControl.
**НЕ УСТАНОВЛЕНА** семантика полей _EndoInfoStruct +0x10..+0x13 и +0x30 (смещения и
ширины известны точно, назначение — нет).

**ПРЕДЫДУЩЕЕ (9-я итерация): K3ADimming — авто-диммирование 3A**
(self-test `dimming`, `app/src/ctrl/K3ADimming.*`). 43 метода; **40 из 43 оказались
off-device**, вопреки пометке «device» в ROADMAP Фазы E — пометка была НЕВЕРНА.
Синглтон без vtable/typeinfo, sizeof 0x288 (648).
**ПОЧЕМУ ЭТО ЧЕСТНО ПРОВЕРЯЕМО:** яркость в классе **НЕ ВЫЧИСЛЯЕТСЯ** — ни гистограммы,
ни зонного взвешивания, ни таблиц весов. Статистика приходит УЖЕ ГОТОВОЙ (2 uint32 +
2 double) в `AUTO_DIMMING_PARA` от KVideoProxy::ReadIrisAndRatio через KSystemStatus.
Значит синтетические входы self-test'а — РОВНО ТО ЖЕ, что видит боевой код; подделывать
нечего. Единственная работа с железом во всём классе — ОДНА запись
`WriteValueToPL(0xA0048010, 0x00350303)` в SetOV6946DimmingParam. Восемь обращений
к GetSystemStatus читают только ФЛАГИ И ИНДЕКСЫ (вынесены в шов).
**АЛГОРИТМ:** ошибка `e = 20*log10(target/measured)`; мёртвая зона 15; далее ветвление
по d8: `d8 <= 0.25` -> ПИД, иначе при `e < 0` -> CalDelt (подавление пересветов), иначе e.
Затем `m_lgtTotal = speed*step + Conver3ADimmingParaToLgt()`, кламп UpdataLgtData и
распределение бюджета ConverLgtTo3ADimmingPara по трём полосам: всё в экспозицию ->
остаток лампе -> остаток усилению. Делитель дБ<->ток — **17903.32**.
**ПИД — В ПРИРАЩЕНИЯХ (velocity form)**, БЕЗ ограничения интеграла (windup) и БЕЗ
клампа выхода; гейн-планирование по 6 наборам {Kp,Ki,Kd} в зависимости от полосы
m_lgtTotal и знака ошибки.
**КВИРКИ ОРИГИНАЛА, ВОСПРОИЗВЕДЁННЫЕ 1:1 (покрыты тестом):**
- **История ПИД (eLast/ePre) — ФАЙЛОВЫЕ СТАТИКИ типа float, а НЕ поля объекта**:
  переживают пересоздание синглтона и общие для всех. Хранение во float при расчёте
  в double воспроизведено намеренно.
- **`Calculate3ADimmingPara` ПИШЕТ ОБРАТНО в структуру вызывающего**: при measured == 0
  ставит 1 прямо в переданную ссылку.
- **`SetAlcMax(double)` НИЧЕГО НЕ СОХРАНЯЕТ** — всё тело метода это лог. Мёртвый сеттер.
- **Мёртвая ветка в `UpdataPid`**: случай `L > p3` выбирает ТОТ ЖЕ набор, что и `L <= p2`,
  поэтому сравнение с p3 лишь вырезает среднюю полосу.
- **`SetOtherSensorDimmingParam` — байт-клон SetCameraAutoDimmingParam, но m_expMax
  НЕ выставляет.**
- **Ассиметрия скорости:** exposureRatioCal при x >= 0 возвращает как есть, а при x < 0
  умножает на `-0.8r^2 + 1.8r + 0.5`; при r == 0 это 0.5 => **затемнение идёт ВДВОЕ
  медленнее осветления**.
- Строки 1 и 2 таблицы m_manuAlcTable ИДЕНТИЧНЫ; `GetManuDimmingALC` индексирует
  плоско и БЕЗ проверки границ обоих индексов.
- `lp2LdB` не проверяет домен: при lp <= B получится log10(<=0) -> -inf/NaN прямо
  в m_lgtTotal. `m_unused40 = 0.1` пишется ctor и НЕ ЧИТАЕТСЯ НИГДЕ.
- `SetDimmingPidParam` присваивает поля 0x54/0x58 ПЕРЕКРЁСТНО — та же перестановка,
  которая уже была отмечена со стороны ini в `_KAutomaticDimmerParam`. Согласуется.
**КОЛЛИЗИЯ (разрешена):** `Float2FixedPointNumber` уже есть в `video/KVideoProxy`
и покрыт self-test `plreg`. В бинарнике эта функция продублирована **ЧЕТЫРЕЖДЫ**
(K3ADimming/KVideoProxy/AlgParaManager/KAlgParamAjustDlg) — у каждого класса своя копия,
поэтому и у нас локальная копия в K3ADimming: это ВЕРНО оригиналу и не тянет
QObject-зависимость ради чистой функции.
**ТОЧНОСТЬ:** спека субагента давала CalDelt = -1.9396597658938428 (посчитано в double),
но коэффициенты в реф. — **float**, и бинарник даёт **-1.9396596920301528**
(расхождение 7.4e-8). В тесте закреплено float-точное значение — оно и есть поведение
оригинала. Хороший бесплатный оракул: `CalPow(24.3496) == 16.49985014952608` совпадает
бит-в-бит с литералом m_expMax в .rodata.
**НЕ УСТАНОВЛЕНО:** имена `_SensorType` 0/1/2 (3 = OV6946 и 4 = OAH0428 закреплены
SetSensorLgtParam); порядок значений `_VLS_MODE`. Цепочка запуска
`KMainCtrlThread::Process3ADimming` НЕ тестируется намеренно — она честно
KVideoProxy/KSystemStatus-зависима и принадлежит другому классу; тест остановлен
на границе `Calculate3ADimmingPara(para)`.

**ПРЕДЫДУЩЕЕ (8-я итерация): KKey2Name — таблица имён клавиш**
(`app/src/sys/KKey2Name.*`, генерируемый `KKeyNameTables.h`, `tools/gen_keysym.py`;
проверки добавлены в self-test `smalllang`). Закрывает «остаток» 7-й итерации.
**ИСХОДНАЯ ГИПОТЕЗА БЫЛА НЕВЕРНА.** Предполагалось, что имена лежат в массивах
`g_strKeysym_S40` (233) / `g_strKeysym_S50` (357) в .bss. На деле:
**`GetNameOfKey` ЭТИ МАССИВЫ НЕ ЧИТАЕТ ВООБЩЕ — они мёртвые данные.** Проверено
двумя независимыми способами: слоты GOT (0xa43c90/0xa44f78), через которые они
адресуются, не читаются нигде вне статического инициализатора, и ни один adrp+add
во всём .text не попадает в их диапазон. Это устаревшие копии того же списка имён
(S50 короче на 24 записи, вставленные в S40 с индекса 292).
**КАК УСТРОЕН НА САМОМ ДЕЛЕ:** `KKey2Name::GetNameOfKey(int)` @0x776808 занимает
**0x862c байт** и КАЖДЫЙ ВЫЗОВ заново собирает НА СТЕКЕ (кадр **24608 байт**) массив
из **614** записей `{int eKey; std::string name;}` (шаг 40), линейно ищет по коду,
возвращает копию имени и в конце разрушает все 614 строк. То есть один вызов —
614 конструирований и разрушений std::string. Ни индексной арифметики, ни проверки
границ, ни ветки «S40 или S50» нет. Промах → **"SONO_Unknown"** (.rodata @0x89e5d0,
ветка 0x77d0d4 — ПРОВЕРЕНО ДИЗАСМОМ ЛИЧНО: субагент в своём же генераторе написал
"None", это было исправлено).
**СТРУКТУРА ТАБЛИЦЫ (614 записей):** индексы 0–232 — `PADK_*` (SDL-подобные), коды
0x000–0x142, **РАЗРЕЖЕНЫ** (индекс != код); индексы 233–613 — `SONO_*`, коды
0x3e9–0x565, СПЛОШНЫЕ (для этой таблицы index = code - 0x3e9). Один дублирующийся
код 0x0 (`PADK_UNKNOWN`, `PADK_FIRST`) — линейный поиск отдаёт ПЕРВОЕ совпадение.
`GetNameOfQtScancode` @0x77ee38 — 54 записи в .bss @0x14e6ba0, тот же layout,
промах → **пустая строка** (иное поведение, чем у GetNameOfKey).
**ИНСТРУМЕНТ:** `tools/gen_keysym.py` — самостоятельный извлекатель: сам разбирает
секции/символы/релокации ELF и прогоняет мини-эмулятор AArch64 (adrp/add/sub/mov*/
movk/ldr-через-GOT/str/bl) по телам обеих функций, склеивая код и литерал по правилу
«строка на 8 байт выше своего кода». Вывод воспроизводим байт-в-байт (1311 строк):
`python3 tools/gen_keysym.py > app/src/sys/KKeyNameTables.h`.
**ПЕРЕКРЁСТНАЯ ПРОВЕРКА ДВУХ ИТЕРАЦИЙ (главная ценность):** коды модификаторов,
отреверсенные НЕЗАВИСИМО в 7-й итерации из `g_astModBitConf_S50`, дают имена
0x449→SONO_CapsLock, 0x456→SONO_Shift_Left, 0x462→SONO_Shift_Right,
0x463→SONO_Control_Left, 0x464→SONO_Function, 0x466→SONO_Alt, 0x468→SONO_AltGr —
ровно то, чем мы их считали. Обе итерации подтверждают друг друга.
**ИНТЕГРАЦИЯ:** заглушка `GetNameOfKey` в KSmallLangTranslate.cpp (возвращала
std::to_string) заменена настоящей; убраны устаревшие комментарии про S40/S50
в KSmallLangTypes.h и KLoadUnicodeText.h. E_SONO_KEY оставлен int: в PADK-части
коды разрежены, enum на 614 значений пользы не даёт.

**ПРЕДЫДУЩЕЕ (7-я итерация): KSmallLangTranslate — раскладки «малых языков»**
(self-test `smalllang`; `app/src/sys/KSmallLangTranslate.*`, `KSmallLangTypes.h`,
`KSmallLangTables.h` — ГЕНЕРИРУЕТСЯ, `tools/gen_kbdlayout.py`).
22 метода; Qt НЕ используется вообще, железа нет ⇒ класс проверяем off-device целиком
(единственная зацепка — KKeyKits::IsCapsLockOn, вынесена в подменяемый шов).
Мейерсов синглтон с ОДНИМ полем, sizeof == 8, ни vtable, ни typeinfo.
Исходник реф. — `platform/language/SmallLanguage/KSmallLangTranslate.cpp`.
Сосед `KSmallLangInputMethod` — Qt-обёртка, СОЗНАТЕЛЬНО не трогаем.
**ТАБЛИЦЫ ЛЕЖАТ В КОДЕ ОРИГИНАЛА, А НЕ В КОНФИГАХ ПРОШИВКИ** — впервые в проекте.
Поэтому их пришлось встроить в исходник, но НЕ перепечаткой: добавлен
**`tools/gen_kbdlayout.py`**, который читает ELF по виртуальным адресам (разбор таблицы
секций напрямую; .bss отдаётся нулями) и печатает готовый заголовок:
`python3 tools/gen_kbdlayout.py > app/src/sys/KSmallLangTables.h` (341 строка,
воспроизводится байт-в-байт). Извлечено: g_astModBitConf_S50, 8 массивов
g_aiModCmb2OutKeyArrIdxFor* (по 16 int), g_astLatinDetail_CmbChar (11x40),
5 таблиц g_ast<Язык>KbdDetail (по 52x40).
**СЕМАНТИКА МОДИФИКАТОРОВ (из g_astModBitConf_S50):** биты nModBitState —
0=Shift_L 1=Shift_R 2=CapsLock 3=AltGr 4=Fn 5=Ctrl 6=Alt; 4-битный индекс уровня —
бит0=Shift(любой) бит1=CapsLock бит2=AltGr бит3=Fn, а **Ctrl и Alt в индекс НЕ входят**
(nOutIdxBit == -1). Конвейер: сырой keysym → диспетчер → modIdx →
piModCmb2OutKeyArrIdx[modIdx] → уровень 0..6 → aeOutKey[уровень].
**КВИРКИ ОРИГИНАЛА, ВОСПРОИЗВЕДЁННЫЕ 1:1 (все покрыты тестом):**
- **`GetBit` возвращает СЫРУЮ МАСКУ (0/1/2/4/…/64), а не 0/1**, и это значение уходит
  наружу через KbdLayout_GetModifierStatus / GetAltgrModifierStatus /
  GetRussianAltgrLockStatus. Проверено: бит 6 даёт 64.
- **`KbdLayoutInit` возвращает true ДАЖЕ ДЛЯ НЕИЗВЕСТНОГО ЯЗЫКА** (после лога),
  оставляя прежнюю раскладку — возможно, nullptr.
- **`ProcLockedModifier` — ПЕРЕКЛЮЧАТЕЛЬ, а не установка**, и при отпускании
  (bPressed == 0) сразу возвращает 0, неотличимо от успеха: отпускание игнорируется.
- **ЛЮБОЕ событие AltGr (0x468), нажатие ИЛИ ОТПУСКАНИЕ, сбрасывает защёлку мёртвой
  клавиши** — молча отменяет начатую композицию.
- Сбой `SyncCapslockStatus` логируется и ИГНОРИРУЕТСЯ — диспетчеризация продолжается.
- **`KbdLayout_ProcRawKeysym` возвращает 0 (а не код ошибки), когда ProcOneRawKeysym
  провалился** — ошибка проглатывается после лога "Translate is Error, i_ret[%d]".
- Клавиша-модификатор отдаётся как ОДНО пригодное событие (возврат 1), а не поглощается.
- **Три МЁРТВЫХ первых параметра** (`KbdLayout_ProcRawKeysym`, `ProcOneRawKeysym`,
  `KbdLayout_GetModifierStatus`) — сразу затираются m_pKbdLayout; сохранены в сигнатурах.
  Плюс третий параметр `ProcMultiLevelKey` не читается вовсе.
- Ветки `lvl < 0` и `lvl > 6` в ProcMultiLevelKey НЕДОСТИЖИМЫ с поставляемыми таблицами
  (там только 0..3 и -1) — мёртвые сравнения сохранены.
- Цикл подстановки в `ProcCombineKey` БЕЗ раннего выхода — все 5 записей всегда
  сравниваются (с текущими данными безвредно, форма сохранена).
- Лог `"convert %s to %s"` срабатывает на КАЖДОЕ успешное преобразование и строит две
  std::string — аллокация на горячем пути.
- В .rodata @0x89bd60 висит НЕИСПОЛЬЗУЕМЫЙ литерал `"StepKey"` — след переименования.
**ГЛАВНАЯ НАХОДКА ПО ДАННЫМ:** `g_astFrenchDetail_CmbChar` и `g_astPolishDetail_CmbChar`
лежат в **.bss и ПОЛНОСТЬЮ НУЛЕВЫЕ** — статического инициализатора нет, во время работы
их никто не заполняет. Значит французские/польские мёртвые клавиши защёлкивают префикс,
но совпадения не находят НИКОГДА и всегда уходят в ветку переигрывания трёх событий.
Воспроизведено как честно пустые таблицы (покрыто тестом).
**НЕ УСТАНОВЛЕНО:** символические ИМЕНА клавиш. В бинарнике их нет — выдаёт только
`KKey2Name::GetNameOfKey`, читающий массивы `g_strKeysym_S40`/`g_strKeysym_S50` в .bss
(233 / 357 std::string, заполняются _GLOBAL__sub_I). Их расшифровка дала бы
авторитетную таблицу «код ↔ имя» — ЕСТЕСТВЕННАЯ СЛЕДУЮЩАЯ ЗАДАЧА. Пока E_SONO_KEY —
просто int, а GetNameOfKey заглушён кодом клавиши.

**ПРЕДЫДУЩЕЕ (6-я итерация): KVideoPlayerMng — список воспроизведения**
(self-test `videoplayer`, `app/src/video/KVideoPlayerMng.*`). 22 метода, 20 из них
off-device целиком; синглтон-ОБЫЧНЫЙ объект (ни vtable, ни QObject), sizeof 0x40.
**ГЛАВНЫЙ ИНВАРИАНТ, определяющий ВСЮ семантику класса:** AutoSetVideoFiles сортирует
каталог как `QDir::Name | QDir::Reversed` — **ПО УБЫВАНИЮ имени**. Имена записей
начинаются с метки времени ⇒ **индекс 0 — САМАЯ НОВАЯ запись**. Поэтому «Next» для
частей одной записи означает УМЕНЬШЕНИЕ индекса, а FindNextEnable сканирует ВПЕРЁД
(к более старым записям). Это не ошибка реф., а осознанная схема.
`KVideoListItem` — ровно 5 std::string (sizeof 0xa0): путь, группа (baseName ДО
ПОСЛЕДНЕГО '_'), индекс части (после него), baseName, suffix в нижнем регистре.
Расширения: набор {mp4,mkv,flv,avi} + фильтр имён {*.mp4,*.mkv,*.flv,*.avi}.
**КВИРКИ ОРИГИНАЛА, ВОСПРОИЗВЕДЁННЫЕ 1:1 (все покрыты тестом):**
- `ParseSplitVideo` декодирует путь как **ASCII/Latin-1** (fromAscii_helper), тогда как
  ВСЕ соседние методы используют fromUtf8. Несогласованность сохранена.
- `ParseSplitVideo` пишет out-параметры ДО проверки половин на пустоту ⇒ при возврате
  -1 «пустая половина» они УЖЕ ЗАТЁРТЫ. (А при -1 «нет '_'» и при -2 — не тронуты.)
- `AutoSetVideoFiles` **ОТБРАСЫВАЕТ** код возврата ParseSplitVideo ⇒ файлы без '_'
  попадают в список с ПУСТЫМИ group/index.
- В фильтр entryList входит **QDir::Dirs** (0x610b) — каталог с именем вида `*.mp4`
  проходит фильтр и отсеивается только проверкой isFile() (проверено тестом-ловушкой).
- `FindLastVideo` берёт **ПОСЛЕДНЕЕ** совпадение пути (свой скан без выхода из цикла),
  а `FindNextVideo` — **ПЕРВОЕ** (через GetVideoListItemIndexByPath). При дублирующихся
  путях направления разрешаются ПО-РАЗНОМУ.
- `FindNextSplitVideo` **не проверяет группу вообще** — слепо возвращает соседа idx-1
  (может отдать часть ЧУЖОЙ записи).
- `GetNext/GetPreVideoFileFullPath` НЕ прерывают скан на совпадении, выходящем за
  границу ⇒ последний (соотв. нулевой) элемент никогда не даёт результата.
- `IsValidVideoFile` логирует номера строк 49 / **62** / **60** — не по порядку.
- `CheckIsFirstOrLast` вопреки имени отдаёт «есть предыдущая / есть следующая» ЗАПИСЬ
  (отрицание «первая»/«последняя»), а список РОВНО ИЗ ОДНОГО элемента даёт false/false.
- `FindNextEnable` материализует полный временный vector копий элементов группы только
  чтобы взять последний (чистые накладные расходы; результат идентичен доскану).
**НЕ УСТАНОВЛЕНО:** поведение CheckIsFirstOrLast, когда путь в списке не найден
(похоже, оба out-параметра остаются нетронутыми).
**DEVICE-ЧАСТЬ (застублена):** `PlayVideo` — диалог KVideoPlayerOSD и
`system("modetest -D fd4a0000.zynqmp-display -w 36:alpha:255")`; ctor реф. также зовёт
свободную `InitGst()` (pthread_once + gst_init) — off-device пропущено.

**ТАКЖЕ В ЭТОЙ ИТЕРАЦИИ: разрешено расхождение по нумерации файлов** (см. §7) —
наш более ранний реверс `KSaveFile::FormatFlowNumber` был НЕВЕРЕН. Схема ОДНА:
FormatFlowNumber @0x6a89b8 — чистый stringstream width(3)/fill('0') БЕЗ потолка и БЕЗ
'^' (в теле нет ни 0x3e7, ни единой ссылки на .rodata); маркер ставит ВЫЗЫВАЮЩИЙ
GetFileFlowNumber @0x6a99d8 (.rodata "999^" @0x8695f8) ⇒ `"999^001"`, а
CheckIsFileNumberUseUp @0x6a92c8 сверяется с "999^999". Совпадает с
KExamDataFileNameGenerator::GetFileSerialNum. Исправлено, self-test `savefile` расширен.

**ПРЕДЫДУЩЕЕ (5-я итерация): KReportEditDataSource ЗАКРЫТ + ИСПРАВЛЕНЫ
ОШИБКИ 4-й итерации** (self-test `reportedit` расширен; `report/KReportItem.h`).
**⚠️ ДВЕ ОШИБКИ ПРЕДЫДУЩЕЙ ИТЕРАЦИИ, НАЙДЕННЫЕ ВТОРЫМ ЗАХОДОМ РЕВЕРСА:**
1. **KReportEntity был НЕВЕРЕН.** Колонок **16, а не 15**: отсутствовала **`Diagnose`**
   (0x60). Её ключ в ConvertToMap — НЕ литерал в .rodata, а 8-байтовый непосредственный
   операнд `"Diagnose"`, поэтому греп по строкам его не находил. Кроме того **HPType
   стоит на 0x160** (между Suggest и AssistDoctor), а не последним полем. sizeof 0x1e8.
   ВЫВОД НА БУДУЩЕЕ: короткие (<= 8 символов) имена ключей могут быть immediate-операндами
   и НЕ ВИДНЫ грепом строк — сверять состав полей по ConvertToMap, а не по .rodata.
   Также добавлен квирк: **ConvertToMap ПРОПУСКАЕТ поля == "INVALID_STRING" и HPType == -1.**
2. **InsertReportItem: было записано «UpdateReportEntity не вызывается никогда» — НЕВЕРНО.**
   Проба `GetEntity` действительно ВЕТВИТСЯ: найдено → **UpdateReportEntity**,
   не найдено → AddNewReportEntity (0x4ca878 `cbz w0` → Update). Повторное сохранение
   отчёта ОБНОВЛЯЕТ строку, а не плодит новые. Покрыто тестом (число строк не растёт).
**ДОБАВЛЕНО: `report_edit::KReportItem`** — агрегат sizeof **0xf0 (240)**, 31 поле,
ни vtable, ни typeinfo; ctor inline (символа нет), эмитируется только дтор
(weak ⇒ определён в заголовке), разрушающий 23 QString + 1 QStringList.
Имена полей ВЫВЕДЕНЫ из колонок-источников (ConvertToMap/NVP-тегов у типа нет).
⚠️ ЛОЖНЫЙ ДРУГ: `report/KReportTemplate.h::ReportItem` — СОВСЕМ ДРУГОЕ (узел дерева
шаблона). Объединять нельзя.
**ЗАКРЫТЫ GetReportItem + InsertReportItem** (оба ВСЕГДА возвращают 0):
- `GetReportItem`: сбой ЛЮБОЙ из двух выборок НЕ прерывает работу и НЕ логируется —
  просто пропускается блок присваиваний; хвост `TriofReference = tr("TR_TRIOFReference")`
  выполняется **БЕЗУСЛОВНО** (он же посадочная площадка обеих неудач).
  **Префикс пути USB подставляется РОВНО В ОДНО поле — RecordPath**, причём склейка идёт
  на std::string **ДО** LoadDbString. `ExamId` берётся ИЗ АРГУМЕНТА, а не из БД.
  **КВИРК: PatientSex присваивается только если LoadDbInt дал 0 И значение <= 3**
  (иначе поле остаётся нетронутым); PatientAge — безусловно.
- `InsertReportItem`: `ReportDate = QDate::currentDate().toString("yyyy-MM-dd")`;
  **формат "%d" используется для PatientSex, а НЕ для HPType** (тот копируется как int);
  **если ReportStatus == "Eg" или "--" → ставится ОДИН ПРОБЕЛ " "**; далее
  UpdateExamEntity → UpdatePatientInfoFromReport → `KObject().PublishMsg(12006, -1, -1, "")`
  из ВРЕМЕННОГО стекового KObject. Сбой GetExamEntity пропускает весь этот хвост, но код
  всё равно 0. НЕ УСТАНОВЛЕНО ТОЧНО: источник DrExamName (переиспользование регистра x27).
**KImgTableItem** — существует, это **QObject (не виджет)**, 44 символа, почти все —
тривиальные аксессоры; единственный трогающий пиксели метод `LoadImgToQImage` (0x94 байта,
похоже на обычный QImage::load). Значит **GenerateExamImgsForPdf достижим и без OpenCV**
в части KImgTableItem — но сам он использует cv::imread/resize/imwrite, так что остаётся
единственным незакрытым методом класса.

**ПРЕДЫДУЩЕЕ (4-я итерация): KReportEditDataSource + слой таблицы Report**
(self-test `reportedit`, `app/src/report/KReportEditDataSource.*`, `KReportEntity.*`,
`KScopeClass.h`). Класс СТАТЕЛЕСС — ctor голый `ret`, ни vtable, ни typeinfo, ни полей,
ни синглтона; ВСЕ 17 методов СТАТИЧЕСКИЕ. Исходник реф. —
`dialog/patient/reportedit/KReportEditDataSource.cpp` (несмотря на «dialog» в пути —
это НЕ виджет, typeinfo отсутствует вовсе).
**СДЕЛАНО:** ChangeFileListToString/ChangeStringToFileList, GetSysOrganNameList,
Get/SetUserOrganNameList, GetRegionImgPath, GetCursorImgPath, GetReportTypeName,
GetSysDeviceType, LoadDbString, LoadDbInt, DeleteReportByExamId, QueryReportTable,
GetOneRecordFromReportTB + KReportEntity(15 колонок) и KReportDBTableHandler(9 методов).
**КВИРКИ ОРИГИНАЛА, ВОСПРОИЗВЕДЁННЫЕ 1:1 (покрыты тестом):**
- Формат списка файлов: пустой список → `""`; иначе `"$" + ("#"+элемент)*`.
  Разбор требует ОБА условия: `left(1)=="$"` И `length()>2`; затем
  `mid(2,length()).split("#", KeepEmptyParts)` ⇒ ведущей пустой части НЕ бывает,
  а хвостовой `#` даёт пустой элемент. **ЭКРАНИРОВАНИЯ НЕТ** — `#` в имени ломает обмен.
- `GetSysOrganNameList`: сравнения только на 1/2/3 ⇒ и 0, и **14 (дуоденоскоп)** дают
  ГАСТРО-список. Размеры: гастро 9, колон 7, бронх 18, носоглотка 25.
- `GetUserOrganNameList`: при отсутствии/нечитаемости файла ИЛИ пустом списке
  возвращает системный. **Слияния, сортировки, дедупликации НЕТ** — списки альтернативны.
  `SetUserOrganNameList` — void, ПОЛНАЯ перезапись без чтения-слияния.
- `GetRegionImgPath` для неизвестного класса → **пусто**, а `GetCursorImgPath` →
  **ГОЛЫЙ базовый каталог** (относительная часть пуста). Поведение РАЗНОЕ.
- `LoadDbInt`: out-параметр при ошибке **НЕ ТРОГАЕТСЯ**; коды -2/-3/-1.
- `GetSysDeviceType` — `mov w0,#0; ret`, вызовов в бинарнике НЕТ (мёртвый символ).
- `KReportDBTableHandler::DeleteEntites` — НЕ РЕАЛИЗОВАН, сразу `ERR_NOT_SUPPORT = -8193`.
- `GetOneRecordFromReportTB` возвращает **1** на успех (не 0) и фильтрует снимки из
  `ExamImg` по фактическому наличию файла.
**⚠️ КОЛЛИЗИИ, НАЙДЕННЫЕ РЕВЕРСОМ — НЕ РАЗРЕШЕНЫ НАМЕРЕННО (нужна отдельная сверка):**
1. **`report/KEntityReport.h` vs реф. KReportDBTableHandler.** Наш объявляет себя
   «реф. KEntityReport/KReportDBTableHandler, tb_Report», но у него ДРУГАЯ таблица
   (`tb_Report` против **`Report`**, БЕЗ префикса), ДРУГОЙ ключ (`accessionNumber`
   против **`ExamId`**) и ДРУГИЕ колонки (есть templateName/examView, которых в реф.
   нет; нет ExamFindings/CustomField1/2/AssistDoctor/ExamImg/Reserved1/2/HPType).
   Совпадают по имени лишь GetAllRecordMainKey/GetQueryRecordNum — и у них РАЗНЫЕ
   сигнатуры. Настоящий слой заведён рядом (`report/KReportEntity.*`), KEntityReport
   НЕ ТРОНУТ, чтобы не сломать self-test `reportdb`. **Дублирование временное.**
2. **`report/KReportDataSource.h`** в комментарии ссылается на «реф.
   KReportEditDataSource», но это СОВСЕМ ДРУГОЙ класс (мешок QHash ключ-значение).
   Общих методов НОЛЬ. Комментарий вводит в заблуждение.
3. **enum-коллизия:** наш `KThesaurusOpt::ScopeClass` имеет ДРУГОЙ порядок
   (Duodeno=2), а реф. `KScopeClass::E_CLASS` — **Duodeno=14**. Смешивать нельзя:
   молча перепутаются Bronchoscope и Duodenoscope. Заведён отдельный `KScopeClass.h`.
**РАСХОЖДЕНИЕ В САМОМ РЕФЕРЕНСЕ:** Get/SetUserOrganNameList пропускают значения 0..14
(`cmp w0,#0xe; b.ls`), но индексируют 6-элементный вектор имён устройств как enum<<5 ⇒
при E_DUODENOSCOPE(14) читают ЗА ГРАНИЦАМИ. У нас индекс приведён к 0..5 (14→4).
**ОТЛОЖЕНО (нужны отсутствующие типы):** GetReportItem/InsertReportItem (нужны
`report_edit::KReportItem`, `KObject::PublishMsg`), GenerateExamImgsForPdf (нужны
`KImgTableItem` + OpenCV: imread/resize/альфа-композит/imwrite в `<thumbDir>/report/img/`).
**ФОРМАТ ФАЙЛА СПИСКА ОРГАНОВ:** `<usrDir>/patient/posname/<DEV_*>.xml`,
boost::serialization XML, NVP-тег `obj_organ_name_list`. Имя поля-вектора ВНУТРИ
KOrganNameList из бинарника НЕ УСТАНОВЛЕНО — читаем терпимо (любые `<item>`), пишем
своей разметкой. Кандидат на уточнение.
**НОВЫЙ ИНСТРУМЕНТ:** `tools/revstr.py <mangled>` — строковые литералы, на которые
ссылается функция (adrp+add), с разбором таблицы секций ELF напрямую.

**ПРЕДЫДУЩЕЕ (3-я итерация): KImageProcess — обработка изображений**
(self-test `imgproc`, `app/src/alg/KImageProcess.*`). 14 методов, off-device ЦЕЛИКОМ
(чистая математика + QImage/QPainter, ни одного системного вызова).
**КЛЮЧЕВОЕ: класс — ПУСТОЙ POD без vtable и полей, ctor — голый `ret`, ВСЕ методы
СТАТИЧЕСКИЕ** (x0 — первый реальный параметр, `this` не передаётся).
**МАТЕМАТИКА (коэффициенты СЧИТАНЫ ИЗ fmov-иммедиатов, не выведены из учебника):**
- RGB2Ybcbr / Ybcbr2RGB — **РОВНО Rec.709/BT.709, прямая И обратная согласованы**
  (не привычная мешанина с 601). Порядок каналов **RGB** (индекс 0 = R), шаг источника
  3 байта, приёмника 12. **Смещения +128 на Cb/Cr НЕТ, ограничения диапазона НЕТ.**
  bytesPerLine НЕ учитывается — буферы обязаны быть плотными.
  В Ybcbr2RGB умножения на 0.0f **реально выполняются** (movi+fmadd) — сохранены:
  они превращают NaN/Inf в хроме в NaN, а не выбрасываются оптимизатором.
- EnhanceBrightnessAndContrast — кусочная кривая ТОЛЬКО по Y (Cb/Cr не трогаются):
  линейная ветвь `A + t*Y` при `Y <= thr`, иначе квадратичная `a*Y^2 + b*Y + c`.
  **КВИРК: «пи» в реф. — ЛИТЕРАЛЬНАЯ 3.1416, а не M_PI** (@0x891528).
  **Линейная ветвь считается во float, квадратичная — В DOUBLE**, причём `Y*b` сначала
  во float и лишь потом расширяется — существенно для побитового совпадения.
  Кривая C1-непрерывна в точке порога и проходит через (255,255). Клампа НЕТ.
- EnhanceSaturability — насыщенность в стиле HSL, float RGB → uchar RGB.
  Ветки по знаку nSat нет (работает и на обесцвечивание). Кламп:
  `v = x + 0.5f; u = (uint)v` (усечение, отрицательные → 0); `v <= 255 ? u&0xFF : 255`.
  При nSat == 0 и S >= 1 **насыщенность ПОДЖИМАЕТСЯ к 1** (alpha = 1/S), а не остаётся.
- OptimizeReportImage — единственный потребитель четвёрки выше. Производственный тюнинг
  реф.: **яркость 40, контраст 20, порог 50, насыщенность 20**. bits() трактуется как
  32-битный ARGB (4 Б/пиксель) БЕЗ convertToFormat и БЕЗ bytesPerLine; альфа не трогается.
  **КВИРК: результат save() ОТБРАСЫВАЕТСЯ ⇒ возвращает 1 даже при неудаче записи.**
  Коды: 5 — ошибка, 1 — «успех».
**ГЕОМЕТРИЯ:**
- `_KGroupImgSize` — 40 байт, 10 x int32 (размер доказан обнулением 0x28).
  `_KPoint` — два int (размер сверх этого НЕ УСТАНОВЛЕН).
- `_KGroupType`: 0 — B слева/A справа, 1 — зеркально (**flag читается ТОЛЬКО здесь**:
  при flag == 1 высота НЕ удваивается и B не центрируется), **2 и 3 — холст 0x0,
  ничего не рисуется (фактически не реализованы)**, >= 4 — наложение B на A в _KPoint.
  Высота = 2*b.h, ширина миниатюры = a.w*H/a.h, центрирование H/4 (asr #2). Зазоров нет.
  **КВИРК: при негодном A (w<=0||h<=0) не пишется НИЧЕГО, даже imgB_*.**
  Порядок рисования в типах 0/1: СНАЧАЛА B, потом миниатюра A.
- `_LoadImgType` → целевой размер: **0 = 150x30, 1 = 850x120, >=2 = 160x120**.
  ResizeCopyImage вписывает **вручную во float32** (не средствами Qt) и масштабирует
  **Qt::FastTransformation**, тогда как CreateThumbnail — **SmoothTransformation**.
  Коды: 5 пустой путь, 1 src==dst либо сохранено, 0 не загрузилось, 2 не сохранилось.
- ScaledWithAspectRatio — `fcmpe`+`b.le` ⇒ **ничья И NaN уходят в ветку «по ширине»**.
**ИКОНКИ ВИДЕО:** MergeVideoSmallImage — `center.png` по центру → `<dir>/<base>_s.jpg`;
MergeVideoBigImage — `play.png` в (100, h-(ih+100)) → `<dir>/res/<base>_v.jpg`,
**ширина НЕ проверяется** (в отличие от малой). Ни одна из них НИЧЕГО не масштабирует.
Иконка проверяется на существование ПЕРВОЙ. Литералы: `icon/video/`, `center.png`,
`play.png`, `/res/`, `_s.jpg`, `_v.jpg`, `.jpg`, `/`.
**ДОБАВЛЕНО:** `KDisplayOption::GetThemeQssPath(QString)` — СТАТИЧЕСКИЙ, файловый путь
в текущей теме (у нас одна, «black»).
**НЕ УСТАНОВЛЕНО:** соответствие смещений +0x30/+0x34 в структуре реф.
GetSoftEndoViewConf полям (у нас метод с тем же именем возвращает ПУТЬ к ini — иная
сигнатура), поэтому FormatScreenShotImg берёт точку из GetDesktopViewConf. Также стоит
решить, кто владеет генерацией миниатюр видео: KImageProcess::MergeVideo* или уже
существующий KSaveVideoFile::CreateVideoSmallImage/SaveVideoThumbnail.

**ПРЕДЫДУЩЕЕ (2-я итерация): KExportRecord — экспорт осмотра на USB**
(self-test `exportrec`, `app/src/db/KExportRecord.*`). 19 методов, off-device целиком
(одна проверка USB подменяется через KUsbDevice::SetUsbPath).
**КВИРКИ ОРИГИНАЛА, ВОСПРОИЗВЕДЁННЫЕ 1:1 (все покрыты тестом):**
- **`makeExamIDPath(sId)` подставляет аргумент ДВАЖДЫ** ⇒ итог `<usb>/Export/<S>/<S>/`
  (makeNameDirPath уже дал `Export/<S>/`, после чего приписывается ещё `<S>/`).
  Похоже на copy-paste-дефект вендора — в этом же классе живёт опечатка **setExprotStatus**.
- **`needCopy` сравнивает имя с "PicInfo.ini", но РЕЗУЛЬТАТ ОТБРАСЫВАЕТ** (нет removeAt) ⇒
  фактически «нужны все обычные файлы». Мёртвая ветка сохранена.
- **`m_examIdsNum` после ctor НЕ ПИШЕТСЯ НИГДЕ** ⇒ `GetExamIdsNum()` всегда 0.
- Класс **никогда не выставляет «успешный» статус** — только 2 (сбой копирования) и
  4 (нет места); вызывающий трактует 0 как «ошибок не было».
- `IsSpaceEnough`: `(need + 1024) < free`, беззнаковое СТРОГОЕ сравнение, запас ровно
  **1024 байта**; в сообщении об ошибке `need` печатается **БЕЗ** запаса. Апостроф в
  литерале — U+2019. Свободное место меряется на **КОРНЕ USB**, не на `Export/`.
- `ExportFiles`: **3-й и 4-й параметры не читаются никогда**; `PicInfo.ini` при успехе
  **не увеличивает счётчик и не вызывает sync()**; ошибка логируется тегом **[I]**, а не [E];
  синтетический код **-7** = USB выдернули ⇒ немедленный выход (прочие ошибки — sync и дальше).
- `cleanDir` — НЕ рекурсивно, entryInfoList с фильтрами по умолчанию (т.е. "."/".." тоже
  перебираются, их удаление просто не удаётся), один sync() ПОСЛЕ цикла.
- `delExistFile` — сравнение ТОЛЬКО по имени, регистрозависимо, без break во внутреннем цикле.
**СОПУТСТВУЮЩЕЕ:** `KFileBackup::copyFile(QString,QString,bool overwrite) -> int`
(реф. сигнатура, 1 — успех, -1..-5 ошибки; распределение кодов по причинам — допущение),
`KSystem::ExportPath()` = `GetUsbPath() + "Export/"` (при пустом USB — пусто),
`QStringLogPrintf/Ex` в KSystemLog.
**НЕ НАЙДЕНО:** кто пишет `PicInfo.ini` — литерал встречается в X2000 ровно в двух местах,
оба внутри KExportRecord, и ни одно его не создаёт. Писатель вне этого бинарника.

**ПРЕДЫДУЩЕЕ (1-я итерация): KExamBussinessHandler — жизненный цикл осмотра**
(self-test `exambiz`, `app/src/db/KExamBussinessHandler.*`). 21 метод, ~20 КБ кода реф.;
непеременный синглтон sizeof==0x720, НЕ QObject/QDialog (сигналов нет вообще) — это
недостающий клей поверх нашего DB-слоя KEntity*.
**НОВОЕ В ЭТОЙ ИТЕРАЦИИ:**
• **KExamEntry (35 полей)** и **MainUiPatientInfo (10)** — `db/KExamEntry.h`, имена и порядок
  1:1 из литералов колонок `ConvertToMap` бинарника (смещения в комментариях).
• **KExamListDBTableHandler** — Add/Get/UpdateExamEntity (в реф. СТАТИЧЕСКИЕ, без `this`;
  0 — успех). DDL = НАДМНОЖЕСТВО колонок; если tb_ExamList уже создана «тонким»
  KEntityExam (10 колонок) — добирает недостающие через ALTER TABLE ADD COLUMN.
  SQLite регистронезависим к именам ⇒ PatientId(наш) == PatientID(реф.) — одна колонка.
• **KPatientEntry дополнен 2 полями реф., которых у нас не было: `PlanDate`, `ExamType`**
  (+ DDL/CRUD). Найдено по FinishSaveDataAction (`if (ExamType != -1)`).
**КВИРКИ ОРИГИНАЛА, ВОСПРОИЗВЕДЁННЫЕ 1:1 (покрыты тестом):**
• **GetGender — БАГ ПОЛЯРНОСТИ.** Скомпилировано как `if (sex.compare("1")) return tr("TR_M");`
  (пропущено `== 0`) ⇒ ветка берётся при НЕравенстве: "1"→**TR_F**, "0"/""/прочее→**TR_M**,
  а **TR_Nknwn недостижим**. Баг сохранён намеренно (решение: верность оригиналу).
• `ClearData()` ставит `PatientSex = "2"` (а не пусто), а строки KExamEntry/KPatientEntry —
  в сентинел **"INVALID_STRING"**, все int в -1.
• `IsCurrentExamIdExaming` — ЧИСТОЕ сравнение строк, m_eState не читает (вопреки имени);
  `IsExamEnd` для ЧУЖОГО ExamId всегда возвращает true.
• `GetSaveDataPath` при отсутствии USB-префикса возвращает **ПОЛНЫЙ** путь — неотличимо
  от успеха; `UpdateExamDataPathName` при провале rename НЕ меняет состояние вообще.
• `UpdatePatientInfoFromReport` берёт ключ БД из КЭША (`MainUiPatientInfo.ExamId`), а не из
  аргумента; `UpdatePatientInfoFrmoDB` (опечатка реф.) НЕ берёт мьютекс — в отличие от соседей.
• `FinishSaveDataAction` возвращает 0 даже при провале AddExamEntity и логирует его тегом
  **[I]**, а не [E]; мьютекс защищает ТОЛЬКО блок MainUiPatientInfo.
• `SaveMainUiPatientInfo`: DrReportName тянется за DrExamName, ПОКА они равны (как только
  пользователь переопределил врача отчёта — больше не трогается).
• `EndoTypeChangeAction` — пустая заглушка `return 0`.
**СПУТНИКИ, ТОЖЕ ОТРЕВЕРСЕННЫЕ В ЭТОЙ ИТЕРАЦИИ (2-й заход субагентом):**
- **KTimeInfo** (sizeof 0xE0, без vtable) — это **СНИМОК, а не «живые часы»**: `Init()` зовётся
  ТОЛЬКО из ctor, публичного Update/Refresh НЕТ ⇒ на каждую отметку оригинал создаёт НОВЫЙ
  объект (все геттеры — методы экземпляра, НЕ static!). 7 полей std::string:
  Year "%04d" / Month "%02d" (tm_mon+1) / Day / Hour / Min / Sec / **Msec "%03d"** (геттера
  для Msec в реф. НЕТ). Init: chrono::system_clock -> ns, secs=ns/1e9, **localtime** (НЕ
  localtime_r, Qt не используется вообще), msec=ns/1e6-secs*1000, snprintf в буфер 16 байт.
  Геттеры: GetCurrentTimeYYYY / YYYYMM(sep) / YYYYMMDD(sep) / HHMMSS(sep) /
  **YYYYMMDD_HHMMSS(dateSep,timeSep)** — где `_` между датой и временем **ЗАШИТ**.
  `sep` передаётся **по значению** (std::string). `GetMs()` — СВОБОДНАЯ функция, не метод.
- **KExamDataFileNameGenerator** (sizeof 0x20) — единственное поле экземпляра m_strExamId,
  счётчик `static std::atomic<int> m_atomFileNum` в .bss (ОБЩИЙ на класс). Экземпляр — за
  **свободной функцией `Generator()`** (реф. `_Z9Generatorv`), а НЕ GetInstance().
  `ResetData()` = store(0) + m_strExamId.clear() (зовётся из ctor и EndoPowerOffAction).
  `GetFileSerialNum()`: n=++счётчик; n<=999 -> "%03d"; **n>999 -> r=n%999; if(!r) r=999;
  snprintf("%d^%03d", 999, r) -> "999^001"** — первый %d это **ЛИТЕРАЛЬНАЯ 999**, не частное.
  `IsMaxNumFiles()`: >**1997** (0x7cd = 2*999-1) -> true. `GenerateFileName(a,b,ext)`:
  m_strExamId=a (побочный эффект) и `a + b + "_" + serial + "." + ext`.
  ВНИМАНИЕ: **РАСХОЖДЕНИЕ С НАШИМ KSaveFile::FormatFlowNumber** (даёт "999^" без 3-значного
  хвоста) — вынесено в отдельную задачу на сверку дизасмом (возможно, это ДВЕ разные схемы).
- **KPatientMngExamStatus** (`: KObject`, sizeof 0x38) — **НИКАКОГО ini/файла состояния НЕТ**,
  «состояние» = сама строка в tb_ExamList; **обратного чтения (LoadExamState/GetExamState)
  в бинарнике не существует вообще**. ctor: SubscribeMsg(12026/12013/12010) +
  ReUpdateLatestExamIdInfoToDb. `SaveExamState(info)`: лог "power off SaveExamState",
  берёт ExamId у KExamBussinessHandler, если непуст -> SaveState(id) + SaveOverExamPatientInfo
  (полей info НЕ читает). `SaveState`: GetExamEntity -> каталог `<usbPath>+RecordPath` ->
  пересчёт `*.jpg *.png *.bmp` и `*.avi *.mp4 *.mkv *.flv` -> RecordImgNum/RecordVideoNum,
  **ReportStatus "Eg" -> "--"**, UpdateExamEntity + PostMsgToUI(0, **12040**, 0,0,{}).
  РЕАЛИЗОВАНО, кроме SaveOverExamPatientInfo (таблицы быстрого ввода — у нас другой API).
- **Логирование** (реф. KLogPrint.cpp — СВОБОДНЫЕ функции, класса-логгера нет):
  `LogPrintf(tag,fmt,...)`, `LogPrintfEx(bool bEnable,tag,fmt,...)`, `LogPrintfx(tag,file,line,func,fmt,...)`.
  **Уровень НЕ вычисляется — он часть строки тега** ("[APP][I]: "/[W]/[E]/[D]); единственный
  рантайм-уровень `g_euLogPriority` смотрят ТОЛЬКО ...Ex: печатать iff `g_euLogPriority!=0 || bEnable`.
  GetCurTime: `"[%02d-%02d %02d:%02d:%02d:%03d]"` — **БЕЗ ГОДА**; строка = timeStr+tag+текст;
  перевод строки добавляется ТОЛЬКО если fmt им не оканчивается. Файл
  `<Root>/data/app/logfile/APPlog%04d-%02d.txt` — ротация **помесячная и только по имени**
  (нет лимита размера, нет retention, нет нумерованных ротаций); fopen("a+")/fclose НА КАЖДЫЙ
  вызов, без мьютекса; при неудаче открытия — молчаливый no-op (в stdout/stderr не пишет).
**ОСТАЛИСЬ ПОДПОРКАМИ:** `hal/KUsbDevice` (GetUsbPath), `kernel/KThreadPoolMsg::PostMsgToUI`
(журнал; коды UI **12041** — осмотр создан, **12044** — каталог переименован, **12040** —
статус осмотра изменён).
KDicomInterface дополнен реф.-именами ActivateSeries/EndSeries/RebindWorklist/DicomStore(examId)
— сеть DEVICE, off-device ведётся журнал вызовов (self-test проверяет последовательность).
device-геттеры модели/серийника (KEndoScope::GetEndoInfo / KCamera::GetCameraInfo) заменены
стендами SetEndoInfo/SetCameraInfo; выбор источника — `KSystemStatus::ViewType() != 0`.

**ПРЕДЫДУЩЕЕ: парсер скриптов автотеста (self-test `autotest`,
`app/src/autotest/KAutoTestScript.*`)** — кандидат B3.
**КЛЮЧЕВАЯ НАХОДКА: парсера `[Confiure]` в X2000 НЕТ ВООБЩЕ** (литерала "Confiure" в бинарнике
нет). Разбор живёт в ОТДЕЛЬНОМ процессе **`update/root/X2000Simulator`** (aarch64 PIE, НЕ
стрипнут, богат символами); в X2000 — только ПРИЁМНАЯ половина KAutoTestThread (получает готовые
коды по IPC и инжектит в uinput). Поэтому реф.-имена здесь — СВОБОДНЫЕ C-ФУНКЦИИ симулятора,
а не методы класса: INIFileCaseExec/AnalyseLineCase/ResetKeyValue/GetCaseValue/OneKeyExec/
SendOneKey + структуры KEY_CONFIG/INI_CONFIG + таблица stKeyValueStr @0x20318.
**НОВЫЙ ИСТОЧНИК ДЛЯ РЕВЕРСА: X2000Simulator — не стрипнутый бинарник, которым мы раньше не
пользовались.** Стоит просмотреть его и на другие подсистемы.
**РАЗБОР — РУЧНОЙ C, НЕ KConfig и НЕ QSettings:** fgets(512) → strncasecmp по префиксу →
strstr("=") → atoi. Следствия (1:1): комментарии '#'/';' НЕ поддерживаются, кавычек нет, табы
НЕ срезаются (только ведущие пробелы), неизвестные строки молча игнорируются.
**МОДЕЛЬ:** [Confiure] (опечатка реф.) + env(значение ОТБРАСЫВАЕТСЯ)/num/time; шаги — [KeyN]
с полями code/event/ctrl/alt/shift/value/sleep/cnt/cmd. **КВИРКИ (сохранены, покрыты тестом):**
• `num` ЧИТАЕТСЯ, НО НИГДЕ НЕ ИСПОЛЬЗУЕТСЯ — итерацию НЕ ограничивает; порядок = ПОРЯДОК В ФАЙЛЕ
  (не Key0..Key<num-1>), НОМЕР в "[KeyN]" вообще не парсится — совпадает любой префикс "[Key";
• заголовок сбрасывает (flush) ПРЕДЫДУЩИЙ шаг ⇒ **N заголовков дают N-1 шагов**, и ПОСЛЕДНИЙ шаг
  теряется без завершающего `[KeyEnd]` — именно поэтому скрипты прошивки им и заканчиваются
  (реальный keyboard.ini: num=165, 166 заголовков → 165 шагов; инвариант проверен тестом);
• дефолты ResetKeyValue: code=-1 (нераспознан ⇒ OneKeyExec=false), cnt=1, остальное 0;
• `code=Sleep` спит по **value**, а НЕ по sleep; `cnt` повторяет ТОЛЬКО Script/Cmd, не обычные шаги;
• `code=Script` → под-скрипт по ФИКСИРОВАННОМУ префиксу AppPath()+"autotest/" + cmd (НЕ
  относительно родителя), **без ограничения глубины и без защиты от циклов** — самоссылка
  рекурсирует до исчерпания стека (сохранено 1:1, циклы в скриптах недопустимы);
• GetCaseValue при отсутствии '=' в реф. возвращает NULL, и вызывающий падает на strlen(NULL) —
  ЭТО ДЕФЕКТ, а не поведение: у нас found=false (падение не воспроизводим).
**ТАБЛИЦА КОДОВ `stKeyValueStr` (146 записей) ПЕРЕНЕСЕНА ЦЕЛИКОМ** — это ground truth клавиш:
клавиатурная часть = значения Qt::Key (в приёмнике маскируются &0x0FFFFFFF), панельная = малые
enum-идентификаторы (GAIN 0x00 … VIEW 0x25, SWITCH_WINDOW 0x100…, COLOR_R 0x201…, BUTTONM_S 0x218),
псевдо-опкоды Sleep/Cmd/Script = 0xffff0002/3/4. Опечатки прошивки сохранены: **FOOTSWICH_LEFT/
RIGHT** (не SWITCH) и ДУБЛЬ имени COLOR_ENH_L2 на кодах 0x208 и 0x209.
РЕАЛИЗОВАНО OFF-DEVICE: разбор + модель + таблица + рекурсия Script (INIFileCaseExec у нас
СОБИРАЕТ шаги в вектор — исполнение device). НЕ реализовано (device): SendOneKey (IPC KMsgBuf),
Simulate_key/mouse (/dev/uinput), KAutoTestThread::run/KeyboardSimulation/PanelKeySimulation
(NB: PanelKeySimulation 2-й аргумент — УПАКОВАННЫЙ (event<<16)|value; KeyboardSimulation —
(Qt::Key & 0x0FFFFFFF) | (модификаторы<<4)). Также off-device, но не взято: AutotestLogCheck
(QProcess→logcheck.py), GetLogPath ("APPlog%04d-%02d.txt"), GetSnapScreenPngFile (пути
"%s/%s_%d/%d.jpeg"), KFactoryOptions::GetTestConfPath ("autotest/"+aging[-scope]+-1/-2+
-softendo/-hardendo). Реф.-квирк ResetFileExecCount: сохраняет НЕИНИЦИАЛИЗИРОВАННЫЙ регистр
в счётчики (+0x14/+0x18) — похоже на баг прошивки, не воспроизводим.
NB: `tools/coverage.py` считает только K*-классы из **X2000**, а эти функции живут в
X2000Simulator → в метрику покрытия НЕ попали (как ранее yxyDES2). Метрика — нижняя оценка.

РАНЕЕ (эта сессия): `KThirdPartyLegalNotices::ReadLegalNoticeText`** (self-test
`legalnotice`) — кандидат B6, декодирован ВРУЧНУЮ (метод маленький, субагент не нужен).
В реф. класс — Qt-диалог (KDialog+moc: InitWidget/ShowInfo/OnExit, виджет
"textEdit_legalNotice", свободная функция OpenThirdPartyLegalNotices), но прикладное ядро
вынесено в non-UI класс — тот же приём, что с KVideoCal; UI — Фаза F.
`ReadLegalNoticeText(QString &out, const QString &fileName)` → **void**: путь =
SystemPath() + **"presetdata/thirdpartylegalnotice/"** (литерал @0x884740, длина 33) +
fileName; QFile::open(ReadOnly); неудача → лог-предупреждение и ВЫХОД, **out НЕ ТРОГАЕТСЯ**;
иначе QTextStream::readAll(). Текст лога воспроизведён ДОСЛОВНО с опечаткой реф.:
`"LegalNotice.open(QFile::ReadOnly) fialed."` (@0x884768). Реальный файл прошивки —
thirdPartyLegalNotices.txt (355 строк, 45100 символов).

РАНЕЕ (эта сессия): `KSysPrintData` (15 методов) — персист конфигурации принтеров
(self-test `printdata`, `app/src/sys/`)** — кандидат B2 из разведки. **ВЕСЬ КЛАСС OFF-DEVICE**
(доказано: в диапазоне реф. 0x786388–0x788560 НЕТ ни одного вызова CUPS/HAL — железо живёт в
KCupsPrinter/KWindowsPrinter/KHalPrinterAPI, соседях по KPrinterManager). НЕ синглтон: владелец —
KPrinterManager (поле +0xC8), часть его методов — тонкие форвардеры (IsPrinterExist/
ChangeDefaultPrinterStatus/FindDriverPath — буквальные tail-jump).
**ВАЖНАЯ НАХОДКА (опровергла гипотезу): блок `<Printer><Item name="DefaultImagePrinter">…` в
KSysPrintService.xml — МЁРТВЫЕ ЛЕГАСИ-ДАННЫЕ.** Литералов DefaultImagePrinter/DefaultPDFPrinter/
DefaultVideoPrinter в 13-МБ образе НЕТ ВООБЩЕ (сырой байт-поиск по всем секциям), ни один путь
кода их не читает; `<Version>` тоже не читается. Живая схема — ТОЛЬКО
`<Root><PrinterList><item …11 атрибутов…/></PrinterList></Root>`; элемент всюду `item` строчными.
**KPrintServiceInfo** (sizeof 0x60): name@0x00, type@0x20, connect_type@0x24, device_or_ip@0x28,
default_printer@0x48, image_printer@0x49, pdf_printer@0x4A, затем ВСТРОЕННЫЙ **KPrintSettings**
(sizeof 0x10) @0x4C: paper_size/gamma/brightness/optimization — реф. копирует его одной парой
ldp/stp. Каждое поле = один XML-атрибут (имена сверены с append_attribute).
**Enum'ы:** E_PRINTER_CONNECT_TYPE 0=Windows/SMB, 1=USB, 2=Network (ДОКАЗАНО по
KAddPrinterDlg::AddWin/AddUsb/AddNetPrinter); E_PRINTER_SERVICE_TYPE — ДОКАЗАНО, что 1→слот
картинок, 2→слот отчётов, 0 пропускается (имена констант — предположение, на семантику не влияет).
**КВИРКИ (сохранены, проверены тестом):** `QueryAllPrinters` — вопреки имени ЗАГРУЗЧИК из XML,
а не опрос CUPS; `AddPrinter` — дедупа НЕТ, добавляет даже при совпадении @name (гейт на
вызывающем); `DelPrinter` — save БЕЗУСЛОВЕН и новый дефолт взамен удалённого НЕ назначается;
`UpdatePrintSettings` — save даже без совпадений; `SetDefaultPrinterInXml` — при НЕУДАЧНОЙ
загрузке всё равно продолжает и сохраняет (может затереть битый файл); `CancleDefaultStatusByType`
(опечатка реф.) — снимает дефолт только у ПЕРВОГО совпавшего; `FindDriverPath` возвращает путь
ПО ЗНАЧЕНИЮ (sret), аргумент НЕ мутирует, файл не читает — только карта; `SaveUrlDriverInfo` —
обновление НА МЕСТЕ, дубли не создаются. `RefreshCurrentPrinterNameInCache` — обход С КОНЦА,
дефолт-элементы в приоритете.
**ПУТИ:** реф. хранит АБСОЛЮТНЫЕ литералы в полях +0x40/+0x60 (`/home/root/system/printer/…`);
у нас — через KSystem::SystemPath() (тот же путь на устройстве, но работает под ENDO_ROOT).
**ИЗВЕСТНОЕ РАСХОЖДЕНИЕ:** реф. на pugixml (сохраняет порядок атрибутов), мы на QDomDocument —
порядок атрибутов в записанном XML ОТЛИЧАЕТСЯ. Для XML это семантически незначимо, round-trip
проверен тестом; при побайтовой сверке с прошивкой это учитывать.

РАНЕЕ (эта сессия): `KColdLightConfig` — параметры авто-диммирования
`_KAutomaticDimmerParam` (self-test `coldlight` расширен + новый TMP-режим `dimmer`)** —
закрыт кандидат из разведки конфигов. Файл `coldlight/coldlightCamera2aPara.ini` (26 float на
`<модель камеры>\<режим>`) НИКЕМ у нас не читался; реф.-читатели —
`GetCameraManuDimmerParam`/`SetCameraManuDimmerParam` (+ пара `Get/SetAutomaticDimmerParam`
для `coldlightCommPara.ini`). **В бинарнике обе пары — ПОБАЙТОВЫЕ КЛОНЫ**, отличающиеся ровно
одной инструкцией (длина литерала имени файла) → у нас общее тело, параметризованное файлом.
Добавлен `KSystem::ColdlightConfigPath()` (= SystemPath()+"coldlight/", реф. @0x673e80).
**СТРУКТУРА `_KAutomaticDimmerParam` (sizeof 100/0x64, доказана 6×stp xzr при зануления):**
18 float @0x00..0x44 (токены 0..17); **qint8** @0x48 (т.18, ЗНАКОВЫЙ — в ini бывает -3),
**qint8** @0x49 (т.19), **quint8** @0x4a (т.20, реф. toUInt), padding @0x4b; float @0x4c (т.21),
@0x50 (т.22); **ПЕРЕСТАВЛЕННЫЙ ХВОСТ (КВИРК, обязателен): т.23→0x58, т.24→0x5c, т.25→0x54**;
@0x60 — 4 байта, из ini не читаются и не пишутся. **ИМЕНА ПОЛЕЙ НЕВОССТАНОВИМЫ** (нет строк
.rodata/ассертов/форматов) → поля названы ПО СМЕЩЕНИЮ, семантику не выдумывали. (Косвенно:
18 float ложатся 6 тройками (a,b,0) с всегда нулевым 3-м — похоже на 6×PID{Kp,Ki,Kd}, но НЕ
доказано; потребитель — K3ADimming::SetDimmingPidParam, чистое копирование полей.)
**СЕМАНТИКА (все 4 метода void):** ключ = `"V01/<endoModel>/<lightMode>"` (QSettings пишет как
`[V01]` + `<model>\<mode>`); в endoModel '/' → `"__"`; подстановки `DefaultParam`/`default`
происходят **ТОЛЬКО при ПУСТОМ аргументе** — фолбэка «ключ не найден» НЕТ; значение читается как
QStringList, и при **count < 26** — лог ошибки и ВЫХОД, выходной параметр НЕ ТРОГАЕТСЯ.
Запись: float → `QString("%1").arg(double,0,'g',-1,' ')`, 3 целых → `.arg(qlonglong,0,10,' ')`;
разделитель `", "` даёт сам QSettings. Set-методы берут структуру **ПО ЗНАЧЕНИЮ** (как в реф.).
Тест: чтение сверено с РЕАЛЬНЫМ `10-100-201\EWL` (22,-3,10,-0.15,2 + перестановка 1.5/0.25/4),
DefaultParam-подстановка, гейт count<26; запись — round-trip + сырой файл (перестановка на диске,
санитайзер `10__100__201\WL`, раздельные файлы). Прошивка НЕ модифицируется (режим `dimmer`
работает во временном ENDO_ROOT).
DEVICE-остаток: `KMainCtrlThread::InitCamera2ADimmingParam` (берёт KEncStyle::GetCameraModel() +
GetAutoDimmingLightMode() и отдаёт результат в K3ADimming) — нужен живой скоп.

РАНЕЕ (эта сессия): `KEncStyle` — video.ini-геттеры per-скоп (расширен self-test
`encstyle`)** — закрыт кандидат A из очереди. Все 13 методов реф. off-device ПРИ ЯВНОМ аргументе
(в реф. 4 из них — getScopeRotateType/GetEndoShapeType/getScopeDefault{Round,Octangle}Cut — при
ПУСТОМ аргументе подставляют GetEndoModel() → device; у нас аргумент обязателен).
**ConvertSrc2Enc** (@0x667870) реализован ТОЧНО: посимвольно `sprintf("%02x", unicode()<0x100 ?
unicode() : 0)` — символ ≥ U+0100 даёт "00" (НЕ '?', как дал бы toLatin1()); "EB-X20"→"45422d583230".
**getScopeInfoPath — КАТАЛОГ** scope/ (не файл; вызыватели сами дописывают "video.ini").
**ДВУХУРОВНЕВЫЙ фолбэк** (сверено): сперва `Default/<key>` читается с НЕвалидным QVariant, результат
идёт дефолтом для `<hex>/<key>` → отсутствие [Default] схлопывается в 0 / 0.0f / null QRect / "".
Реализованы: getScopeSize(QRect), GetEndoSensorType/GetFirmwareType/GetEndoType/GetEndoShapeType
(**возвращают int-enum через таблицы config2*Map, НЕ QString** — важная правка гипотезы),
getScopeRotateType, getScopeDefault{Round,Octangle}Cut, GetScopeZoomRatio, getScopeParaDefault,
getBiopsyImg, getScopeType. **КВИРКИ (сохранены):** `config2firmwareTypeMap` — значение 6
ОТСУТСТВУЕТ, а 8/7 «перевёрнуты» (OV2740_1024X1024=**8**, OCHFA_OAH0428_720X720=**7**);
`GetScopeZoomRatio` и `GetEndoShapeType` — БЕЗ [Default], зашитые дефолты 1.0 и
"OCTANGLE_AND_ROUND"(=0); `getScopeParaDefault` — ОДИН проход, 4 ключа, POD, workLength через
toUInt() в **16-битное** поле (>65535 обрезается); `getBiopsyImg` — СБОРКА ПУТИ (ini не читает):
'/' из модели удаляется → `<scope-dir>/<hex>.png`; `getScopeType` — video.ini НЕ читает, сканирует
8 enc-файлов через KEncSettings::getStringList() по СЫРОЙ модели в порядке cenc=1, genc=0, benc=2,
nlc=3, denc=4, vetc=5, cysc=6, choc=7, не найдено → 0 (**0 неоднозначен**: гастроскоп и «не найдено»).
Тест на РЕАЛЬНОЙ секции [45422d583230] X-2600/SonoScape: QRect(0,0,864,1056), sensor OH01A=1,
fw OH01A_768X928=3, endo OH01A_EB_768X928=10, shape=0, roundCut=458, octCut=3932220, zoom≈1.14,
para(2.0/4.9/4.9/600), biopsyImg → существующий 45422d583230.png, scopeType=2 (benc→бронхоскоп).
НЕ РЕАЛИЗОВАНО (device): GetEndoModel (KEndoScope::GetEndoInfo + фолбэк на хвост
getSupportedScopeList при роли>2), no-arg twins (getScopeSize()/getCurScopeType()), getIsDefaultMatch.
ДОБАВЛЕН и **GetEndoDisplayModel** (off-device): модель КАК ЕСТЬ, подмена только при
KAccount::CurrentRole()<=1 && бренд=="PyCkeun" по таблице из 5 (EG-X20→G, EC-X20→C,
EC-X20L→C, EB-X20→B, EB-X20T→B); не в таблице → как есть.

РАНЕЕ (эта сессия): `KSysReportTempletModel` (21/21) + `KSysReportTempletControl` (27/27)
— ЗАКРЫТ §10-блокер отчётной ветки (self-test `templetmodel`, `app/src/report/`).** Оба класса
off-device целиком (чистая логика + QDate); декод — 3 субагента, все методы сверены дизасмом.
**MODEL** (sizeof 0xd8): поля m_vecTempletInfos@0x00 / m_mapCfgCache@0x18 (map имя→cfg) /
m_delList@0x48 / m_tmpData@0x60 (буфер либ-элементов). Init=ClearCfgCache+cfg.GetTempletInfos
(добавлен реф.-аксессор `KSysReportTempletCfg::GetTempletInfos(vec&)`); LoadDefault/DiscardChange=
cfg.LoadDefault/LoadUserXML+Init; Reload — append-only сверка (новые имена дописываются, in-memory
правки живут); Save = SaveTempletInfos + SaveTemplateCfg(кэш,delList) + при непустом m_tmpData
UpdateTemplateLib+сброс. **КВИРКИ (НЕ чинить, проверены тестом):** `GetDefalutTempletNameByDept`
ВОЗВРАЩАЕТ АРГУМЕНТ dept (sret=копия dept, вычисленное имя выбрасывается), плюс побочный эффект —
при отсутствии дефолта назначает `report_preview::NP_nx3`="NP-nx3" дефолтом департамента;
`SetDefault(templetName, dept, bool)` — ИМЕННО такой порядок, и clear-цикл снимает default у ВСЕХ
шаблонов департамента даже при bool==false; `DeleteTemplate` удаляет ТОЛЬКО если имя есть И в
infos, И в m_mapCfgCache (иначе лог «%s is in info lib but not in cfg lib» и ПРОДОЛЖАЕТ обход);
`RenameTemplate` при отсутствии в кэше НЕ переименовывает вовсе (даже SetTempletName), а при
наличии — миграция ключа + **старое имя уходит в m_delList**. `SaveSingleTemplatetCfg`: сперва
m_vecTempletInfos (обновить по TempletName либо push_back), затем m_mapCfgCache[имя]=data.
`GetDelUserDefineItem(item, vector<KReportTemplateItem*>&, data)` → **bool, out — УКАЗАТЕЛИ**:
item удаляется, если НЕ найден FindConstRefItem (=user-define), ЛИБО есть дети и ВСЕ удалены.
`DeleteUserDefineDept` обрабатывает ТОЛЬКО ПЕРВЫЙ шаблон с департаментом и сразу return true;
RemoveSubItem(cfg, **"/RT_MEASURE"**, dept) + лениво сеет m_tmpData из KTemplateLibCfg::Data()
(реф. assert "lib_cfg != nullptr", line 367). `UpdateUserDefineItem` — ВСЕГДА true, ассертов нет:
идёт по НЕ-заводским шаблонам (factory пропускаются — сравнение ModifyDate с REPORT_RW_BACKUP в
реф. есть, но результат не используется), читает item-config **"CalcApp"**, RevertPathByID(",")
→ пути, FindRefItem → GetDelUserDefineItem → GetParentItemID+RemoveSubItem, запись обратно
ТОЛЬКО в m_mapCfgCache. **`STR_GENERAL_TEMPLATE` = "General"** (@0xab4dc0, .rodata 0x880608 —
восстановлено трекером регистров по static-init; НЕ "NP-nx3", первая гипотеза была неверной):
`AddUserDefineDept` цепляет департамент к шаблону с именем "General" (AddDept(dept,true)+
SetModifyDate), в заводском TempletInfo.xml такого шаблона НЕТ → вернёт false до его создания.
Сентинел «все департаменты» Model = **"REPORT_DEPT_ALL"** (GetTempletInfosByDept).
**CONTROL** (sizeof 0x48): m_model@0x00 (heap `new KSysReportTempletModel`), m_selectedDept@0x08,
m_selectedTemplet@0x28; синглтон (реф. call_once+heap → у нас Meyers-static); КАЖДЫЙ метод
открывается assert(m_model). Init/LoadDefault = model + `m_selectedTemplet=GetDefalutTempletName
ByDept(m_selectedDept)`; OnDeptChanged: dept→m_selectedDept, **сентинел "KW_ALL" → выбор шаблона
НЕ трогается**; SetSelectedTempletDefault=SetDefault(templet,dept,true); RenameSelectedTemplet —
выбор следует за именем; UpdateUserDefineItem = model+Save, всегда true. **КВИРКИ:**
`DeleteSelectedTemplet` — ПУСТОЕ тело (ни ассерта, ни модели); `GetCopyTempletInfo` — НЕ
уникализирует имя (берёт как есть) И сбрасывает per-dept флаг default в false, factory=false,
дата=сегодня; `GetSelectedDept` — сырой m_selectedDept (в т.ч. "KW_ALL", без трансляции);
`CopySelectedTemplate` тянет cfg у ИСХОДНОГО выбранного (не newName) и НЕ сохраняет копию.
Реф.-логи воспроизведены дословно ("delete templet cfg(%s) success!!!", "rename templet(%s)
success!!!", "Save SingleTemplatetCfg::update existed cfg (%s)", "all child is deleted, …",
"dept is %s, default templet is %s", "copy info: name is %s, date is %s, factory is %d").
СЛЕД.: отчётная ветка off-device закрыта — брать кандидатов A/B из блока «ГОТОВЫЕ КАНДИДАТЫ»
(KEncStyle-остаток / разведка нетронутых конфигов субагентом).

РАНЕЕ (эта сессия): `KSysReportTempletCfg` — ЗАПИСЬ каталога + мутаторы KTempletBaseInfo
(self-test `templetsave`)** — ПОДГОТОВКА к KSysReportTempletModel/Control (§10-блокер отчётной
ветки; его зависимость KReportTemplateManager закрыта ранее в этой сессии). Наш класс был
READ-ONLY — добавлены 5 реф.-методов (сверено дизасмом): **LoadDefault** (сносит user-cfg каждого
шаблона через KTemplateCfg::DeleteTemplateCfg + UpdateTemplateLib(пустой) + читает RO-каталог;
на диск TempletInfo.xml НЕ пишет), **LoadUserXML** (читает USER-каталог, без CheckFile/RO-фолбэка),
**GetTemplateByName** (делегирует KTemplateCfg::GetTemplateCfg → Template(<имя>).xml, user→RO+кэш),
**SaveTempletInfos** (кэширует аргумент в infos_, пишет ВСЕ инфо без фильтра в USER-путь),
**SaveTemplateCfg(map,delList)** (СНАЧАЛА удаления, затем апдейты; ФИЛЬТР: нетронутые заводские
factory&&modifydate=="factory" ПРОПУСКАЮТСЯ).
**ПУТИ (важно!): реф. НЕ использует один корень** — ходит в KEnvConfig, RO и RW это РАЗНЫЕ ПРЕФИКСЫ:
RO=GetReadOnlyBaseDir()+"mainapp/patient/report/config/TempletInfo.xml",
RW=GetUsrDir()+"patient/report/config/TempletInfo.xml". Старый read-путь на reportRoot_ сохранён
(добавлен parseTempletFileAbs для абсолютных путей).
**СХЕМА ЗАПИСИ** (реф. KTempletInfoFileHandler::Save/AddTempletNode, pugixml→у нас QDom):
<Root><Templet name= modifydate= factory="1|0"><Dept name= default="1|0"/></Templet></Root>;
bool РОВНО "1"/"0" (НЕ true/false); порядок атрибутов name,modifydate,factory / name,default;
<Dept> ПО ВОЗРАСТАНИЮ имени. **KTempletBaseInfo**: в реф. depts это `std::map<string,bool>`
(уникальность+сортировка) — наши мутаторы (AddDept перезаписывает флаг, вставка сортированная,
DeleteDept/SetDefault/IsDefault/TempletName/SetFactory/…) ВОСПРОИЗВОДЯТ map-семантику на QVector.
Тест: round-trip Save→LoadUserXML + сырой XML на "1"/"0".
СЛЕД.: сам **KSysReportTempletModel** (19) + **KSysReportTempletControl** (25) — декод ГОТОВ.
Реф.-квирки, которые НЕЛЬЗЯ «чинить»: Model::GetDefalutTempletNameByDept **ВОЗВРАЩАЕТ ДЕПАРТАМЕНТ,
а не имя шаблона** (баг прошивки, sret=копия аргумента, вычисленное значение выбрасывается —
доказано NRVO); Control::GetCopyTempletInfo **НЕ делает уникализацию имени** (имя приходит готовым);
Control::DeleteSelectedTemplet — ПУСТОЕ тело; GetTemplateCfgByName ПРАЙМИТ кэш (чтение = грязнит);
Save НЕ чистит кэш/delete-list; Reload — append-only merge (undo это DiscardChange); дирти-флага НЕТ
(грязь = непустые контейнеры); сентинелы «все департаменты» РАЗНЫЕ: Model "REPORT_DEPT_ALL",
Control "KW_ALL".

РАНЕЕ (эта сессия): `KControlProc` — крипто-ядро машинного контроля (self-test
`kcontrolproc`, `app/src/kernel/`)** — поверх проверенного [[yxyDES2]]. STANDALONE/STATELESS
(без vtable/typeinfo/Q_OBJECT, ctor/dtor пустые, полей нет — как KMeaStringUtil). **КЛЮЧ DES
машинного контроля = литерал `"ZXYuio12"`, слот 0** (реф. собирает инлайн-иммедиатами
0x32316F6975595 85A → LE 5A 58 59 75 69 6F 31 32). Реализовано (сверено дизасмом):
**ConvertOtherFormat2Ciphertext(des,out,inHex)** — ASCII-hex→байты, nBits=((len+3)/4)*16 (длина
округляется ВВЕРХ), возврат nBits/8, -1 при nullptr; Hex2Bits портит вход → копия;
**DecryptionStr(QString&)→QString** — hex→Latin-1 плейнтекст (обрезка по первому NUL);
**Cipher2Plain(QString& путь)→QString** — ФАЙЛОВАЯ обёртка (внутр. имя реф.
ControlInput::CipherFile2PlainFile): читает файл→DecryptionStr→пишет в KControlINI::PlainINIpath(),
**возвращает ПУТЬ, не текст**; ""-при ошибке; **LicenseFileName(sn,type)** — 0,3→"_release.ini",
1,4→"_delay.ini", 2,5→"_import.ini", >=6→пусто; **SystemDate2MCDate(QString)→void** — НЕ конвертер
строк: no-op при выключенном Control_time либо remain<=0, иначе SetDeadline(date+remain,
"yyyy-MM-dd"). Enum **_KControlType** восстановлен исчерпывающе (0 processor_release, 1
processor_delay, 2 import_endo, 3 endo_release, 4 endo_delay, 5 import_processor). Тест: round-trip
через РЕАЛЬНЫЙ DecryptionStr (encrypt строим симметрично — в прошивке ЕСТЬ только расшифровка).
ДОБАВЛЕНА лицензионная ветка (сверено дизасмом): **McDirLicense** (реф. KSystem::ImportPath()+
"license/"; USB-корень — off-device settable SetImportRoot), **IsLicensePathExist**, **GetMcFilenameList**
(*.ini в каталоге; реф. USB-gate опущен), **IsMd5sumUsed(md5,type)** — анти-повтор: по имени типа как
ключу в licensehistory.ini хранится QStringList израсходованных md5 (первый — добавляет+пишет→false,
повтор→true, type>=6→true; АРГУМЕНТ — сам md5, не серийник), **CheckLicense(sn,type)→int** (1 нет
каталога/2 нет файла/3 расшифровка/4 md5-использован-или-поля-не-совпали/0 OK; файл→Cipher2Plain→
plain.ini: md5sum + SN/ControlState/ProcessorSN/EndoSN по типу; ProcessorSN машины —
KSystemSet::GetProcessorSN), **GetDeadline/GetDelayTime** (plain.ini ключи deadline/addNum — ОТЛИЧНЫ
от KControlINI одноимённых), **IsStartTimeMc/IsStartEndoMc** (→KControlINI), **IsOutofControl**
(время истекло remain==0 ЛИБО endo-контроль+текущий не в GetMatchEndos; GetCurEndoSN — DEVICE,
off-device settable SetCurEndoSN). Тест: CheckLicense end-to-end на СИНТЕЗИРОВАННОМ DES-шифрованном
файле (0/повтор→4/нет→2/нет-каталога→1). ДОБАВЛЕН машинный контроль (сверено дизасмом, вердикт по каждому методу): **StartTimeMc(const
_MC_Time*)** — лог+СКВОЗНАЯ WriteMcTime, флаг controlTime САМ НЕ СТАВИТ (на вызывающем), nullptr→
тихий no-op; **StopTimeMc** — controlTime=false, deadline="2099-01-01", **remainDays=-1** (sic, НЕ 0:
IsOutofControl сверяет ==0 → -1 это «разоружено», а не «истекло»); **UpdateMcDays** — МОНОТОННЫЙ
ХРАПОВИК remain=min(remain,max(0,daysTo(deadline))): часы НАЗАД дни не возвращают (записи просто нет),
ВПЕРЁД — сжигают безвозвратно; битый/пустой deadline → remain=0 (FAIL-CLOSED); ЧИТАЕТ СИСТЕМНЫЕ ЧАСЫ;
**StartEndoMc/StopEndoMc** — ОДНА блочная WriteMcEndo (флаг+список не разойдутся; Start лога НЕ пишет
— асимметрия реф.); **IsEndoMatch** — GetMatchEndos().contains(GetCurEndoSN(), Qt::CaseSensitive)
(регистр важен). Тест: храповик проверен относительно currentDate (не зависит от часов) — продление
дедлайна НЕ инфлирует остаток.
**НЕ РЕАЛИЗОВАНО — DEVICE-BOUND (EEPROM живого эндоскопа, KEndoScope в app/src НЕТ):**
GetEndoRemainTimes (EEPROM+0x24 u16), GetEndoDeadline (+0x28), GetMatchProcessorList (+0x30 — NB имя
ВРЁТ: matchprolist.ini НЕ читает!), IsStartMatchProcessorCtrl (IsOpenMatchProControl),
IsStartEndoUseTimeCtrl (IsEndoPowerOn && IsOpenEndoControl — бит 6 флагов), IsEndoPowerOn
(KEndoScope::IsEndoReady — поле +0x10 == 4). Плюс ImportMatchLicense/DispEndoList (UI/USB).

РАНЕЕ (эта сессия): `yxyDES2` — DES (self-test `des`, `app/src/kernel/`)** — РАЗБЛОКИРУЕТ
крипто-ветку KControlProc/KControlINI (в §10 значилась блокером «DES yxyDES2» — оказалась чистой
логикой, НЕ device). STANDALONE (без vtable/Q_OBJECT/device-зависимостей), sizeof **0x46EB=18155**,
биты — ПО ОДНОМУ НА char, MSB-first. **ВСЕ таблицы сверены с бинарником (@0x8942a8..0x8945b0) и
совпали со СТАНДАРТНЫМ DES байт-в-байт**: сдвиги/PC-2/IP/E/P/PC-1 + S1..S8 @0x8943b0 + IP-1
@0x8945b0. **«DES2» = ДВА независимых расписания ключей**: параметр `uint` у InitializeKey/
EncryptData/DecryptData — это НЕ длина, а НОМЕР СЛОТА (0/1); двойного/тройного DES НЕТ (обычный
single DES, ECB). Поля: m_SubKey[2][16][48]@0, m_CipherBits@0x600, m_PlainBits@0x640,
m_CipherBytes@0x680, m_PlainBytes@0x688, m_szCipherBinary@0x690, m_szCipherHex@0x6d1,
m_szPlain@0x6e2, m_CipherAny[8192]@0x6eb, m_PlainAny[8192]@0x26eb. **ПРОВЕРКА — КАНОНИЧЕСКИЙ
DES-ВЕКТОР** (key=133457799BBCDFF1, plain=0123456789ABCDEF → **85E813540F0AB405**) — совпал точно
(сильнее, чем self-consistency). Реф.-квирки сохранены: EncryptAnyLength при len>8 ВСЕГДА пишет
лишний блок ((len/8+1)*8; при len%8==0 это шифр восьми нулей), len==8 — спецслучай (8 байт);
DecryptAnyLength асимметричен (выход ровно len, хвост только при len%8!=0, читает полные 8 байт);
Bits2Hex — ВЕРХНИЙ регистр без NUL; Hex2Bits ПОРТИТ вход. NB: `tools/coverage.py` считает только
K*-классы → yxyDES2 (16 методов) в метрику НЕ попал (нижняя оценка). СЛЕД.: **KControlProc**
(standalone, без vtable/Q_OBJECT — лицензирование: Cipher2Plain/DecryptionStr/CheckLicense/
GetDeadline/IsEndoMatch/LicenseFileName/SystemDate2MCDate/IsMd5sumUsed) — теперь разблокирован.

РАНЕЕ (эта сессия): `KReportTemplateManager` — ФЕЙТФУЛ-API (self-test `reporttmplmgr`)** —
капстоун отчётного модуля. КОЛЛИЗИЯ РЕШЕНА СОВМЕЩЕНИЕМ: класс в `report/KReportTemplate.h` теперь
несёт ДВА API — наш упрощённый загрузчик (ctor(reportRoot)+TemplateNames/LoadTemplate, 3 старых
self-test'а работают) И фейтфул-API реф.: **GetInstance** (реф. heap shared_ptr+call_once → у нас
Meyers-static; НЕ вызывает InitModule), **InitModule** (идемпотентен по m_bInited: провизия
заводского RO-дерева "mainapp/patient/report/rw/report" → userpreset "patient/report/" через
новый **KSystem::CopyDirectoryFiles** при отсутствии каталога/3 конфигов; InitTempletsInfos+
InitTempletLibInfos через KSysReportTempletCfg; new KTemplateCfg/KTemplateLibCfg/KRTDataSourceDemo/
KRTDataSourceReal + Check/LoadCache, у lib ещё LoadCacheGroup+UpdateTemplateLib(Data())),
**UninitModule** (delete частей + m_bInited=false, векторы НЕ чистит), геттеры (Get*Cfg/GetDemo
DataSource — assert non-null; **GetDataSourceReal — null-safe без assert**), GetTempletsInfos/
GetTempletLibInfos. Поля как в реф. (sizeof 0x90). Провизия пишет в userpreset — gitignored,
безопасно. Отчётный модуль связан end-to-end через реф.-API.

РАНЕЕ (эта сессия): `KDbSqlite` ЯДРО (self-test `dbsqlite`, `app/src/db/`)** — низкоуровневая
обёртка SQLite (реф. KDbSqlite : IDatabase), фундамент под KEntity*. Реф. на RAW sqlite3 C API +
SQLCipher; off-device — на СИСТЕМНОМ libsqlite3 (`-lsqlite3` в линковке ui_preview; sqlite3.h из
SDK). ВСЁ ЯДРО сверено дизасмом вручную (`objdump -d`, без субагента): layout sizeof 0x58
(m_strLastError@0x08, sqlite3* m_pDb@0x28, sqlite3_mutex* m_pMutex@0x30, m_strDbPath@0x38; статик
m_bIsLogOn); ctor=sqlite3_mutex_alloc(RECURSIVE); **Open**=sqlite3_open+ретрай BUSY; **Close**=
sqlite3_close+ретрай BUSY, null-ит handle; **IsOpen**=(m_pDb!=0) под mutex; **Exec(char*/string)**=
charset→UTF8(стаб)+FilterLogSql(стаб→true)+sqlite3_exec под mutex+ретрай BUSY, errmsg→m_strLastError,
null sql→-4102; **GetLastErrorMsg/GetDbPath/SetLogEnabled/FilterLogSql**; **InsertField**("alter table %s add %s
varchar")+**DeleteRecord**("delete from %s [where %s]") — snprintf(буфер 0xa000)+Exec, литералы
LOWERCASE сверены; **GetFieldNameList**("select * from %s"→prepare_v2→column_name'ы в set);
**InsertRecord**(в INSERT только ключи-существующие-колонки через GetFieldNameList; значения
sqlite3_snprintf("%Q"); "insert into %s (%s) values(%s)" через mprintf→Exec); **UpdateRecord**(SET
"col=%Q" через тот же фильтр колонок; "update %s set %s [where %s]" mprintf→Exec, запятая условная).
ОПУЩЕНО off-device (device-only, помечено): SQLCipher-разблокировка в Open — проба `select count(*)
from tb_DcmWorklist` без ключа → при ошибке `sqlite3_key(m_pDb,"SONOSCOPE_X2000_KEY",19)`+повтор
(plain libsqlite3 без sqlite3_key). **GetRecordsNumber**("select count(*) from %s [where %s]" →
sqlite3_get_table → strtol azResult[ncol]); **QuerySingleRecord**("select * from %s [where (%s)]
limit 1" → get_table → map колонка→значение первой строки); **QueryRecords** @0x447be0 —
QUERY-BUILDER: map это НЕ произвольные условия, а СТРУКТУРА запроса по спец-ключам (все имена
сверены из .rodata): **"Column"** колонки SELECT (дефолт **"*"**@0x864f70), **"Where"** условие
(оборачивается в скобки "("@0x842728/")"@0x8a2e30), **"Group"**@0x85ce28 → " group by ",
**"Order"**@0x8548f8 → " order by ", **"Limit"**@0x862748 → " limit "; SQL "select %s from %s
[where (..)][ group by ..][ order by ..][ limit ..]" → get_table (ретрай BUSY) → НА КАЖДУЮ строку
map(колонка→значение) в out (az[(r+1)*ncol+j]). **KDbSqlite ЗАКРЫТ 12/12 методов.** IDatabase-база (чистый
интерфейс) — off-device standalone, методы virtual как в реф.

РАНЕЕ (эта сессия): `XmlParser` (self-test `xmlparser`, `app/src/report/`)** — тонкая
обёртка XML-документа (реф. на вендоренном pugixml `pugi::xml_document*` + std::string описания
парсинга, sizeof 0x28). Off-device — на **QDomDocument** (Qt5::Xml), как KMeaXMLBase. Методы
сверены дизасмом: **LoadFromFile**(parse_default, encoding_auto) → m_strResult=description(),
возврат успех; **GetParseResult**()=хранимое описание; **GetRoot**()=document_element() (у нас
QDomElement); **SetDeclaration**()=append_child(node_declaration) с version="1.0" encoding=
**"utf-8"** (LOWERCASE, реф.!); **SaveToFile**("\t", format_indent). Тест через QTemporaryDir
(load/root/атрибуты/текст, битый XML→описание, round-trip, декларация). ВАЖНО (методология):
ПЕРЕД декодом грепать `class <Name>` в app/src — KReportTemplateManager уже был как свой
упрощённый API (коллизия), см. [[a2600-existing-ownapi-classes]]. НЕ РЕАЛИЗОВАНО (отдельная
задача, task-чип): faithful KReportTemplateManager (синглтон InitModule) — реконсиляция с
существующим загрузчиком.

РАНЕЕ (эта сессия): `KObject`/`KMessage`/`KPublishManager` — in-process шина сообщений
(self-test `kobject`, `app/src/kernel/`)** — ФУНДАМЕНТ событийной подсистемы (разблокирует
KDataOprEventDeal/KEntityService/KPatientMngExamStatus/KDicomEventDeal — все root at KObject).
Декод дизасмом: диспетчеризация ПОЛНОСТЬЮ СИНХРОННАЯ, без Qt/мьютексов/потоков. **KMessage**
(sizeof 0x58, поля публичные): target/msg id, sender, handled-флаг, sParam/ll1/ll2, strData,
shared_ptr payload; Reset() ставит handled=TRUE (sic); IsValid()=любое поле≠сентинел.
**KPublishManager** (singleton, реестр map<int,list<KObject*>>): AddSubscribe(dedup)/Remove
Subscribe/RemoveAllSubscirbes(sic-опечатка реф.)/PublishMsg — рассылка с ЗАЩИТНОЙ КОПИЕЙ реестра
(реентерабельность). **KObject** (полиморфный sizeof 0x38, статический реестр m_objects id→объект):
SubscribeMsg/UnSubscribe (msgId∈(9999,13999]); PublishMsg→HandleSubscribeMsg всем; SendMsg→
m_objects[target].HandleMsg (target∈(0,4999], msg∈(0,3999]), возврат handled; RequestToParent→
parent.HandleChildRequest (msg∈(19999,23999]); **PostMsg — ЗАГЛУШКА (async скомпилирован в no-op,
сообщение отбрасывается)**; InitObject регистрирует id (assert уникальность, ≤4999). Handlers по
умолчанию пустые virtual. Пропущен Dump (пустой стаб, нужны KDumpStream/KCmdLine). Диапазоны id —
assert'ы реф. НОВЫЙ off-device-домен (kernel) после report/patient. Дальше по KObject-дереву:
KDataOprEventDeal (msg-glue прогресс-диалога — UI), KPatientMngExamStatus (структуры KPatientEntry/
KExamEntry — имена полей не восстановимы), KEntityService.

РАНЕЕ (эта сессия): `PatientExamData` (8 статических методов, self-test `examdata`)** —
перечислитель файлов обследования (`app/src/db/KPatientExamData.{h,cpp}`, класс БЕЗ vtable/
состояния — реф. передаёт id в x0 без this). DEVICE-ONLY вырезано: реф. резолвит каталог из id
как GetUsbDevice().GetUsbPath()+KExamListDBTableHandler::GetExamEntity(id).dir (USB+SQLite) — здесь
методы принимают ГОТОВЫЙ examDir (оканчивается '/'). Файловая логика сверена 1:1: **IsFileExist**
= ifstream open+good() (НЕ QFile::exists); **GetExamData(dir,out,filters)** — QDir name-filters,
ПОЛНЫЕ пути (dir+fileName), сорт по QFileInfo::baseName()↑, out НЕ очищается (append), dead-параметр
bClear убран; **GetExamDataImage** *.jpg/*.bmp (без png); **GetExamDataVideo** *.mkv/*.mp4 (без avi);
**GetExamDataPdf** dir+"report.pdf" (out только если файл есть, return 0 и при отсутствии);
**GetExamDataPath** каталог(и) если существует; **GetExamDataAll** = картинки+report.pdf (БЕЗ видео),
-1 только если оба провалились; **IsExamVideoExist** entryInfoList({*.mp4,*.mkv}).count()>0. Все
name-фильтры case-insensitive (Qt default). Тест через QTemporaryDir. НОВЫЙ off-device-домен после
исчерпания report_template.

РАНЕЕ (эта сессия): `report_template::*` парсинг-конфигов (+4 функции, self-test
`reporttmpl` → 35 проверок) + структура `KSplitLineInfo`** — декод дизасмом: **AppendSubData** —
скомпилированная заглушка (return false, как GetSubData); **GetSubItemsParam(data,id,out)** —
ПЛОСКИЙ substring-скан всего m_mapItemConfigs (НЕ дерево!), ключ содержит id → unique-insert,
out не чистится, пустой id → все; **ConvertToDetail(src,outBase,outParam)** — инверс
ConvertToSourceID: RevertPathByID(src,",") → base=parts[0], при ≥2 частях outParam.clear()+
ConvertStringToMap(parts[1]); пустой src → false; **GetSplitLineInfo(id,configs,out)** — наполняет
новую структуру **KSplitLineInfo** (в `KReportTemplateData.h`: 3×int + 2×std::string, sizeof 0x50,
БЕЗ QColor/QTextLength — тип/цвет строки); gate по атрибуту "SplitLineWidth" (нет → out сброшен),
дефолты Type="Horizontal"/Color="black", override непустыми, Space/StartIndex опционально; int
строго через QString::toInt (мусор→0). **report_template свободные функции по сути закрыты (28/36,
остаток — Qt-виджет KLineEdit).** СЛЕД. домены REPORT (все device/QTextCursor-рендер — сверять!):
KTemplateEditDocument, KDocumentGenerator, KRT*Creator/DataSource — брать осторожно, многое
device-bound (QTextTable/QTextCursor). Off-device-кандидаты искать в CORE/DB/MISC по COVERAGE.md.

РАНЕЕ (эта сессия): `report_template::*` Customed-семейство (+4 функции, self-test
`reporttmpl` → 27 проверок)** — декод дизасмом: **GetCustomedSections(data,out)** — out.clear()
+ верхний уровень m_lstItems, кастомная если m_mapItemConfigs[id].m_bUserDefine; пушит id;
**RenameCustomedItem(data,id,newName)** — FindRefItem, пишет **m_strTitle** (НЕ m_strName!), без
миграции ID; **DeleteCustomedItem(data,parentId,item)** — ТОЛЬКО корень (parentId==""), матч по
**m_strID** (не m_strName как RemoveSubItem), erase + чистка конфигов (ключ==id ИЛИ родитель-ключа
содержит id — substring-баг сохранён); **AppendCustomedItem(data,parentId,item&)** — корень;
синтез item на месте: m_strName="KW_NEW_SECTION_<n>" (первое свободное), m_strTitle=tr(тег)+n
(tr device-only→идентичность off-device), m_strID="/"+name, type=RT_TITLE_TABLE_BLOCK, ShowTitle=
"1", Column="3"; конфиг {UserDefine=1,Append=1,RefColumn=3,FontType=ThirdTitle,RefColumnID=id,
m_bUserDefine} в m_mapItemConfigs; копия в конец m_lstItems. report_template 24/36 (67%). Осталось
(12): GetSplitLineInfo(нужен KSplitLineInfo)/GetSubItemsParam/ConvertToDetail/AppendSubData(стаб)/
GetSubItemsID(есть)+пр. — брать субагент-декодом.

РАНЕЕ (эта сессия): `report_template::*` мутации дерева (+5 функций/3 имени, self-test
`reporttmpl` расширен до 20 проверок)** — декод дизасмом (гипотезы поправлены бинарником!):
**GetSubItems(item/by-id, out, rec)→bool** — близнец GetSubItemsID, но ЭЛЕМЕНТЫ; out НЕ
очищается, копии ПОВЕРХНОСТНЫЕ (плоский список), сам item не включён, pre-order; by-id: id пустой
→ корень, miss→false; **RemoveSubItem(data,parentId,id)→bool** — матч ребёнка по **m_strName==id**
(sic, НЕ m_strID!), erase + чистка m_mapItemConfigs по префиксу "<parentId>/<id>/" (собственный
конфиг узла БЕЗ '/' не трогается), БЕЗ root-фолбэка (пустой parentId→false); **AppendSubItem(×2)→
bool** — дедуп по **m_strName**, push_back ГЛУБОКОЙ ДОСЛОВНОЙ копии (m_strID НЕ пересчитывается —
как MergeSubItem, НЕ UpdateItemID!); list-overload строит множество имён ОДИН РАЗ → внутрибатчевые
дубли ОБА добавляются (отличие от N single-вызовов); дубль→skip+continue. Лог [APP][W] (device-only)
отброшен. report_template 20/36 (56%). Осталось: GetSplitLineInfo/GetSubItemsParam/ConvertToDetail/
*CustomedItem(Append/Delete/Rename)/GetCustomedSections/AppendSubData(стаб)/UpdateItemID(есть).

РАНЕЕ (эта сессия): `report_template::*` навигация по дереву (+5 функций, self-test
`reporttmpl` расширен)** — декод дизасмом: **FindConstRefItem(data,id)→const item*** — коммит-
спуск по пути (точное m_strID==id, иначе если id содержит "<node.m_strID>/" → уходим в поддерево,
сиблинги отбрасываются), nullptr на промах; **FindRefItem** — non-const близнец (хвостовой b →
const_cast); **UpdateItemID(item,parentId)→bool** — pre-order пересчёт m_strID=GenerateIDByString(
parentId,m_strName,"/") с рекурсией в детей от нового ID; **GetSubData** — в реф. СКОМПИЛИРОВАННАЯ
ЗАГЛУШКА (mov w0,#0;ret — как сосед AppendSubData), воспроизведено `return false`; **HasSameNameIn
Group(data,id,name)→bool** — ВНИМАНИЕ сравнивает с **m_strTitle** сиблинга (не m_strName!), родитель=
id без последнего "/"-сегмента (пустой→корень, иначе FindRefItem, miss→false), сам себя (по m_strID)
не считает. report_template 17/36 (47%). Осталось в namespace: AppendSubItem(×2)/RemoveSubItem/
GetSubItems(×2)/GetSplitLineInfo/GetSubItemsParam/ConvertToDetail/*CustomedItem/GetCustomedSections/
MergeData(есть) — брать пачками субагент-декодом.

РАНЕЕ (эта сессия): `report_template::*` (6 функций, self-test `reporttmpl`)** — декод
дизасмом свободных функций отчётной подсистемы (`KReportTemplateCommonDef.cpp`). ДЕ-ЗАГЛУШЕНЫ
две (сигнатуры в реф. — bool + out-ref, не return-value; исправлены и они, и вызовы в
KTextBlock::Title/KImageBlock::Url): **QueryTemplateItemRealTitle(item,out&)→bool** — out=
m_strTitle, false; динамический резолв 4 маркеров m_strName (RT_RESERVED1/2, RT_CUSTOM_FIELD1/2_
TITLE) идёт через DEVICE-only конфиги (KPatientListConfigSetupHandler/KReportEditUIConfig) —
off-device не воспроизводим; **ConvertToSourceID(src,param,out&)→bool** — out=src+","+
ConvertMapToString(param) (НЕ подстановка плейсхолдеров, просто конкатенация; завершающая ','
всегда). НОВЫЕ: **ConvertMapToString(map)→string** формат "%s\|%s;" (пустой ключ пропущен,
сорт по ключу, завершающий ';'); **ConvertStringToMap(s,out&)** инверс — Split(";")→Split("\|"),
берётся РОВНО 2 токена с непустым значением, out не очищается (merge); **GenerateIDByString(a,b,
sep)→string** = a+sep+b (без sep если конец пуст); **IsPatientInfoTitleBold(item)→bool** =
m_strID содержит "RT_PATIENT_INFO"\|"HOSPITAL_OTHER". Round-trip Map⇄String сверен (пустые
значения теряются — как в реф.). report_template теперь 12/36 (33%). Ещё не реализованы в
namespace (кандидаты, все off-device-логика): GetSubData/AppendSubData/AppendSubItem/RemoveSubItem/
FindRefItem/GetSplitLineInfo/GetSubItemsParam/ConvertToDetail/GenerateIDByPath(vec)/RenameCustomedItem
и др. — брать пачками через субагент-декод.

РАНЕЕ (эта сессия): `KTitleTableBlock`** (`app/src/report/`, self-test `titletableblock`) —
таблица-с-заголовком, ТОНКИЙ поведенческий НАСЛЕДНИК KTableBlock (typeinfo __si_class_type_info
→ база KTableBlock, sizeof 0x78 — НИ ОДНОГО нового поля; «заголовок» живёт в узле,
item->m_strTitle). Переопределяет ровно 2 virtual: **Size()** = базовая +1 к width (лишняя
строка под заголовок), height как есть; **GetTemplateItemForCell(row,col,out)** = делегирует
базе с (row-1) (row 0 — заголовок). **ShowTitle()** невиртуально скрыт → ВСЕГДА true.
Добавлены **Title()** = fromLatin1(item->m_strTitle) и **SetTitle(q)** = item->m_strTitle =
q.toUtf8() (АСИММЕТРИЯ кодировок read/write — round-trip только для ASCII, воспроизведено 1:1).
TitleTextBlock унаследован (идентичен базовому). Реализовано через публичные геттеры базы —
KTableBlock.h не трогали. Следующие Block-сиблинги в референсе исчерпаны (Text/Image/Table/
TitleTable — все 4 готовы). Кандидаты на след. итерацию — потребители-креаторы (KRTTextItemCreator/
KRTImageItemCreator/KRTTableItemCreator) или KRTDataSource*, но многие device-bound (QTextCursor-
рендер) — сверять дизасмом перед взятием.

РАНЕЕ (эта сессия): `KTableBlock`** (`app/src/report/`, self-test `tableblock`) — модель
табличного блока отчёта, третий сиблинг KTextBlock/KImageBlock. ОТЛИЧИЕ: ПОЛИМОРФНЫЙ (свой
vtable: `virtual ~`, `virtual QSize Size()`, `virtual bool GetTemplateItemForCell(row,col,out&)`),
но БЕЗ базового класса и виджет-зависимостей (typeinfo 16 байт = standalone, sizeof 0x78, чисто
off-device). Поля: item*/dataNew*/m_itemConfig + вычисляемый **QSize m_size** (width=строки,
height=столбцы), НЕТ m_mapTextParam. ctor → InitItemConfig (как сиблинги) + **CalcTableSize**:
nCol=toInt(item->Column); если item-config имеет "RefColumnID" → конфиг по нему [operator[],
вставляет пустой при промахе] → "RefColumn" переопределяет nCol; childCount=подэлементов;
nCol>0 → m_size=(ceil(childCount/nCol), nCol), иначе (childCount, 1). **GetTemplateItemForCell**:
row/col<0→false; index=row*cols(=height)+col, шаг по m_lstSubItems, вне списка→false, иначе
out=&узел, true. Аксессоры (ВСЕ сверены дизасмом, точные ключи/дефолты/типы): ElementId=
item->m_strID (БЕЗ null-guard, в отличие от сиблингов); **ShowTitle**=item->m_strShowTitle=="1"
(ПОЛЕ узла, не attr); **TableType**=item->m_strType с ремапом 7 алиасов (RT_OB_Z_SCORE/WMS/SEWMS/
OB_EFW/SUB_DATA/TEXTGROUP/IMAGEGROUP_BLOCK)→"RT_TITLE_TABLE_BLOCK"; **TitleTextBlock**=KTextBlock(
item,dataNew,hideKey=true) по значению; **BorderWidth**→float (attr "BorderWidth".toFloat, 0.0f);
**ShowBorder**=BorderWidth()!=0; **Margin**→vector<float> (attr "MarginWidth" split "," ×**0.62**);
**BorderColor**→QString дефолт **"black"**, перезапись при НАЛИЧИИ ключа "BorderColor" (find, не
пусто-проверка); **ColWidthContraints**→QVector<QTextLength> (attr "ColumnRatio" split "," →
PercentageLength(toInt)). Потребители (KRTTableItemCreator/KTemplateEditDocument::InsertTable)
не трогаем. Следующие Block-сиблинги: **KTitleTableBlock** (6 методов, SetTitle+ctor — вероятно
наследник KTableBlock) — кандидат на след. итерацию.

РАНЕЕ (эта сессия): `KImageBlock`** (`app/src/report/`, self-test `imageblock`) — модель
блока-картинки отчёта, прямой сиблинг KTextBlock (НЕ виджет). Поля как у KTextBlock, но БЕЗ
m_bHideKey. ElementId=item->m_strID; **ImageName=tr(item->m_strName)** (от Name узла, не Title);
Width=ConvertStringToInt(itemcfg "ImageWidth") с дефолтом **-1** при отсутствии; Heigth (sic)
то же с "ImageHeight"; GetAlign=itemcfg "AlignH" как есть. **Url(bool& valid)**: DataSrc →
(ConvertToSourceID если m_mapTextParam непуст) → SplitStr(",") → ПОСЛЕДНИЙ токен → путь из
глоб. кэша MAP_LOCAL_PIC_PFINFO (у нас static-карта g_picMap + RegisterPicPath/ClearPicMap);
valid=!QImage(path).isNull(); путь возвращается ВСЕГДА (даже невалидный). Width/Heigth/GetAlign
читают ITEM-CONFIG-атрибуты (не стиль, в отличие от FontSize KTextBlock). Добавлены заглушки
report_template::QueryTemplateItemRealTitle (было) + ConvertToSourceID (тело @0x5954b0 не
декодировано → возврат src). НЕ ОПРЕДЕЛЕНО: наполнение MAP_LOCAL_PIC_PFINFO (рантайм-кэш
потребителей типа KRTDataSourceReal::GetImageData) — в self-test через RegisterPicPath.
РАНЕЕ (эта сессия): `KTextBlock`** (`app/src/report/`, self-test `textblock`) — модель
текстового блока отчёта (НЕ виджет — нет vtable; пиксельный рендер в отдельных потребителях).
Обёртка над KReportTemplateItem* + KReportTemplateDataNew*. Data(): атрибут item-config
"TemplateData" → ключ → значение в m_mapConfigs → tr(). Title(): QueryTemplateItemRealTitle →
tr → FormatStr("%s : ") если непусто и != Data. FullText = Title+Data. Стиль (FontSize/Bold/
Italic) — 2-уровневый резолв: атрибут item "FontType" → именованный стиль в m_mapItemConfigs
→ "Size"/"Bold"/"Italic". Alignment: AlignH → Left/Center/Right; LineHeight1..5. Структуры
DataNew (m_mapConfigs @0x00 данные, m_mapItemConfigs @0x48 стили) СОВПАЛИ с нашими 1:1.
Добавлен report_template::QueryTemplateItemRealTitle (возврат m_strTitle — точный резолв
@0x595688 не декодирован). ОПУЩЕНО (device-рендер): DPI-масштаб pt→px в FontSize
(physicalDotsPerInch) — возвращается pt. РАНЕЕ (эта сессия): `KEncSettings`** (`app/src/sys/`, self-test `encset`) — аксессор
к ЗАШИФРОВАННЫМ файлам-спискам моделей эндоскопов/камер. База QObject (не диалог/OSD/железо),
чистая файловая логика. «Enc» = Encrypted: шифрование — **побайтовое дополнение (bitwise NOT
~b)**; проверено на реальном genc.ini прошивки → "EG-500,EG-500L,EG-X20". Файл хранит ОДИН
CSV-блоб (genc/cenc/uenc/denc/benc.ini = гастро/коло/uh/ed/eb) в
system/style/<series>/<brand>/scope/; НЕ секции/ключи — key у value/setValue ИГНОРИРУЕТСЯ.
readData/writeData (NOT + seek/offset, файл не усекается, writeData портит буфер in-place),
getDataLen, value (пустой файл→def), setValue (чанки ≤1024б), getStringList (split ","),
loadFileFromList (join "," → setValue), loadFileFromUsb (QSettings Model/Num + Model/ID<i>).
**ЗАКРЫВАЕТ отложенный getScopeList нашего KEncStyle** (глобальный enc-список моделей).
НЕ ОПРЕДЕЛЕНО (помечено): сепаратор join (принят "," по симметрии со split); индекс старта
цикла Model/ID в loadFileFromUsb (принят 0..n-1). РАНЕЕ (эта сессия): `KSessionInfo`** (`app/src/sys/`, self-test `session`) — синглтон
состояния облачной login-сессии SonoScape В ПАМЯТИ (персиста нет, обнуляется при рестарте).
НЕ UI/device/БД. Два независимых канала: Manu (производитель, KPUserLoginDlg::Login) и
Service (сервис-инженер, KScopeInfoEdit/KCameraInfoEdit::ServiceUserLogin), каждый —
Uid/UserName/AccessToken(bearer)/LoginFlag; + единый QNetworkAccessManager (getManager).
Класс пассивный: логин/таймаут — в диалогах-потребителях (аплоад авторизации эндоскопа/
камеры/аппарата в облако). Токенов времени/expiry НЕТ. Потребовал линк Qt5::Network в
ui_preview (уже был в find_package). РАНЕЕ (эта сессия): `KDbStrHandler` + SQLCipher-ключ.** `app/src/db/KDbStrHandler`
(self-test `dbstr`) — построитель SQL-условий генерик-слоя реф.: SqliteReplace,
SqliteCharsEscape (' → ''), BuildSimpleCondition ("field op 'value'", порядок (field,value,op),
value СЫРОЙ без escape — особенность реф.!), BuildAndCondition "(a) and (b)",
BuildOrCondition "(a) or (b)". Чистый static-класс.
**КЛЮЧЕВАЯ НАХОДКА (load-bearing): SQLCipher-ключ БД — хардкод-литерал `SONOSCOPE_X2000_KEY`**
(реф. KDbSqlite::Open: sqlite3_open → sqlite3_key(этот литерал) → пробный select из
tb_DcmWorklist). Без него реальный HD-2000.dat не читается. Добавлено в
`KEntityManage::OpenDb` как `PRAGMA key='SONOSCOPE_X2000_KEY'` сразу после open() — на
устройстве (SQLCipher-драйвер) применяется, штатный QSQLITE в отладке молча игнорирует
(вся БД-регрессия PASS). РЕВЕРС ДВИЖКА (для справки, реализовывать НЕ нужно — наш
connection-based слой эквивалентен): KEntityBase(IDatabase&, type) — абстрактная сущность,
CRUD виртуальный (база возвращает ERR_NOT_SUPPORT/0/-1, работу делают наследники через
IDatabase-vtable Open/Exec/InsertRecord/QueryRecords/…); KEntityManage — реестр по
type-строке (GetEntityInterface); единственный реализатор IDatabase — KDbSqlite (raw
sqlite3_* + SQLCipher, НЕ QtSql). Реестр заводить не нужно (косметика). РАНЕЕ (эта сессия):
`KEntityDoctor` + `KDoctorDBTableHandler`** (`app/src/db/`,
self-test `doctor`) — сущность/CRUD врача, **отдельная** таблица-справочник tb_Doctor
(account/пароль/счётчик) — НЕ дублирует tb_QuickInputDoctor (история автоввода, др. классы).
Чистый SQLite, endo_main, по паттерну KEntityPatient. Колонки: account/passwdLength/count/
time/Reserved1/Reserved2 + PK id AUTOINCREMENT (account — бизнес-ключ). Хендлер: GetEntity
(0/-1), Add/Update/DeleteEntity, GetAllEntities, GetAllAccount, GetEntityByAccount
(WHERE account=), **GetRecentUseAccount (ORDER BY time DESC, count DESC + LIMIT N)**.
7-е поле реф. не декодировано полностью: колонка `time` ПОДТВЕРЖДЕНА через ORDER BY
(GetRecentUseAccount), альт. кандидат `name` не подтверждён. ДОДУМАНО (помечено): типы
DDL — count/passwdLength INTEGER (для числового ORDER BY), прочее TEXT; точное N в LIMIT
из дизасма не извлечено (параметр, дефолт «без ограничения»). РАНЕЕ (эта сессия):
`KEntityPatient` + `KPatientListDBTableHandler`** (`app/src/db/`,
self-test `patient`) — сущность/CRUD пациента, tb_PatientList. Чистый SQLite (не UI/device),
соединение endo_main (HD-2000.dat), по отлаженному паттерну KEntityExam. Колонки реверснуты
из KPatientEntry::ConvertToMap: PatientID/PatientName/PatientSex/PatientBirthday/ApplicantDate/
Applicants/UserItem1/UserItem2/SickBedId/TelephoneNumber/RegisterNumber/WorklistUID/PatientAge/
ExamStatus (в реф. ВСЕ строки, включая ExamStatus/PatientAge). PK — технический `id`
AUTOINCREMENT; PatientID — бизнес-ключ. Хендлер тонкий: GetEntity (коды 0/-1), AddNew/Update/
DeleteEntity, UpdateExamStatus (read→ExamStatus=%d→write), GetPageRecordFromDb. СТРАННОСТЬ РЕФ.
1:1: `DeleteEntites(vector)` — ЗАГЛУШКА (тело возвращает глоб. int, НИЧЕГО не удаляет) →
воспроизведено как no-op. ДОДУМАНО (помечено): типы колонок DDL (в реф. живут в KEntityManage,
не в этих классах) — приняты TEXT / id INTEGER; в реф. дизайн реестровый (KEntityBase+type-строка,
GetInnerEntityManage), у нас connection-based (как KEntityExam). KPatientStringOperation в этих
классах НЕ зовётся (split имени — на UI-уровне). FK PatientID↔tb_ExamList.PatientId не enforced.
РАНЕЕ (эта сессия): `KStopWatch`** (`app/src/ui/`, self-test `stopwatch`) — ПЕРВЫЙ
UI-виджет в наборе (реф. KStopWatch : QFrame). Экранный секундомер процедуры: конечный
автомат STOPWATCH_STATUS {0 Stop/сброс, 1 Pause, 2 Run}, тик — целые секунды по QTimer(1000мс)
(реального монотонного таймера нет — счётчик тиков, 1:1). HandleKeyPress: Space(0x20)→Pause,
F1(0x01000030)→Runing. HandleStopWatchRuningState: Stop→Run (старт) / иначе → Stop+сброс
"00:00:00" + сигнал StopWatchStateChanged. HandleStopWatchPauseState: Pause⇄Run, в Stop —
ничего. UpdateTime: m_time += 1 сек, timelabel = toString("hh:mm:ss"). Виджет тестируется
offscreen (как остальной UI в ui_preview): автомат прогоняется без ожидания таймера — тики
вызываются напрямую. ЗАМЕТКА: ctor реф. оставляет m_time невалидным (00:00:00 ставит внешний
KViewBase::InitStopWatch) — у нас InitStopWatch вынесен в класс для самодостаточности.
ОТЛОЖЕН: **KTimeMng** (QObject-синглтон, 3 QTimer: display 1000/day 60000/mc 1500 мс) —
тянет KUiMsgProxy (UI-мост, не реализован): UpdateSystemtime/CheckMachineControl/UpdateRecTime;
EachDayMC — полночный лиценз-отсчёт (GetRemainDays==1 → SetProductAuthFlag(0)).
РАНЕЕ (эта сессия): `KPatientStringOperation` + `KDbStringOperation`** (`app/src/dicom/`,
self-test `patstr`) — строковые/DICOM-утилиты (потребители — DICOM-слой). НЕ UI/НЕ device,
все методы static, состояние — function-local static-таблицы. std::string везде.
KPatient (12): StringReplace/StringTrim(фикс. " \r\n\t")/ReplaceInvalidCharInFolderName
(\ / : * ? " < > |), GetSOPClassUID(type/flag → US Multiframe/VL Endoscopic/Secondary
Capture/US Image), ConvertCharacterset (iconv, буфер 5120 → усечение), ConvertCharactersetToUTF8
(GB2312/ISO-8859-1/-5 → utf-8), GetISOCharactersetOfDicom (ISO_IR 100..192/GB18030→charset),
SplitDicomPatientName (по '^', ПОРЯДОК реф. token0→p4/token1→p2/token2→p3; без '^' → p2=вся),
AssembleDicomPatientName, AssembleDicomFilePath (dir/name + ".dcm" если flag==false),
AssembleCoverFilePath (пробует .jpeg/.bmp/.png через QFile::exists), GenerateUniqueIdentifier
(суффикс .1.1..1.10 по сущности). KDb (9): те же строковые (делегируют KPatient) + УРЕЗАННАЯ
таблица charset, а ConvertCharacterset/ToUTF8 — ЗАГЛУШКИ return true (БД хранит UTF-8 напрямую,
DB-путь конверсию отключил). Имя «KDbStringOperation» вводит в заблуждение: SQL-экранирования
НЕТ. Дубли реф.: StringReplace≡KMeaStringUtil::ReplaceStr, StringTrim⊂Trim*. Добавлен линк
libiconv в CMake (на Xilinx-target iconv в glibc). НЕ ОПРЕДЕЛЕНО (помечено): bool-флаг
GetISOCharactersetOfDicom; обрезка хвостовых '^' в AssembleDicomPatientName; финал
GenerateUniqueIdentifier — DCMTK dcmGenerateUniqueIdentifier + site-root (device, возвращён
только суффикс).
ОТЛОЖЕНЫ БЛОКЕРЫ отчётной ветки (нужен отдельный заход): **KSysReportTempletControl** —
тонкий фасад, но осмыслен лишь с **KSysReportTempletModel** (~19 методов, не реверснут),
а полный Save тянет **KReportTemplateManager** (не реализован отд. классом) + доп. методы
KSysReportTempletCfg (GetTemplateByName/SaveTempletInfos/SaveTemplateCfg). **KDataOprEventDeal** —
msg-шина KObject/KMessage + UI KProgressDlg. **KExportRecord** — USB (KUsbDevice).
**KControlProc** — DES yxyDES2.
РАНЕЕ (эта сессия): `KControlINI`** (`app/src/kernel/`, self-test `controlini`) —
слой доступа к ini машинного контроля. НЕ UI, НЕ крипто (DES — в компаньоне KControlProc),
stateless (полей нет; реф. методы инстансные, но this не используют → у нас static).
ПЕРСИСТ — QSettings(IniFormat), НЕ наш KConfig: bool у QSettings пишется true/false, а KConfig
"True/False" — несовместимо, поэтому именно QSettings (проверено self-test'ом: control.ini
содержит `Control_endo=true`, не `True`). Корень: ProtectedPath()+"kmachinecontrol/"
(каталог создаётся). Реально читается/пишется ТОЛЬКО control.ini (плоские ключи → [General]):
Control_time(bool)/Deadline(QString,дефолт "2099-01-01")/RemainDays(int,0)/Control_endo(bool)/
Endos(QStringList). Методы: MachineControlPath/ControlINIPath/PlainINIpath/MatchProListIni/
HistoryLicenseRecord (последние три — path-хелперы, сам класс их не читает; их файлы
обслуживает KControlProc), ReadMcTime/WriteMcTime/ReadMcEndo/WriteMcEndo, Get/SetDeadline,
Get/SetRemainDays, Get/SetMatchEndos (ключ Endos в control.ini, НЕ matchprolist!),
IsStart/StartTimeControl, IsStart/StartEndoControl. НЕ РЕАЛИЗОВАН: UpdateMatchEndos(_MC_InputInfo*)
— структура _MC_InputInfo (известны лишь int@0/int@8) и правило add/remove из дизасма не
восстановлены; строительные блоки Get/SetMatchEndos доступны.
ЗАМЕТКА для будущего: компаньон **KControlProc** (прототип-/лиценз-контроль) остаётся
отложен — тянет DES-класс yxyDES2 (Cipher2Plain/DecryptionStr) для расшифровки лицензий.
РАНЕЕ (эта сессия): `KDbFileOperation`** (`app/src/db/`, self-test `dbfileop`) —
файловые/дисковые утилиты. Несмотря на «Db» в имени SQLite НЕ трогает (нет ни одного
KEntity*/sqlite3_) — чистые ФС/statfs-хелперы поверх Qt+POSIX, все методы static, полей
нет, НЕ UI/НЕ device. IsFileExist/IsFileDirExist/GetFileSize/RemoveFile/CreateFolder/
DeleteFolder/CopyFileToFile/GetFilesByFilter/GetLastDirName/GetFileNameWithoutDir/
StringReplace/IsPatientDataExist(tail→IsFileExist) + ёмкость ФС по пути (statfs, БАЙТЫ:
f_blocks·f_bsize / f_bavail·f_bsize). С KEntityService НЕ пересекается. ЧАСТИЧНО ОПРЕДЕЛЕНО:
GetNumOfSpaces(char*,int) — роль 2-го аргумента из дизасма не восстановлена (принято: число
пробелов в первых n символах), в self-test проверено консервативно.
СНЯТ КАНДИДАТ: **KDataOprEventDeal** — БЛОКЕР: подписчик своей msg-шины (KObject/KMessage,
НЕ Qt signals) + напрямую рулит UI-диалогом KProgressDlg; требует не реализованных
KObject/KMessage/KProgressDlg/KDataFileOpr. Реализовывать после портирования шины KObject.
ОТЛОЖЕН: **KExportRecord** (экспорт снимков осмотра на USB, НЕ UI) — device-coupled:
freeSize/ExportFiles тянут GetUsbDevice/KUsbDevice; `PicInfo.ini` — НЕ конфиг, а имя-маркер
(сайдкар осмотра: копируется, но не учитывается в successFileNum). Ждёт заглушки USB.
РАНЕЕ (эта сессия): `KManuPwdMng`** (`app/src/sys/`, self-test `manupwd`) — контур
доступа производителя/сервиса. НЕ UI, НЕ синглтон-с-состоянием (sizeof==1, полей нет — всё
в KSystemSet [Manu]). Пароли производителя/админа/сервиса — ПРОИЗВОДНЫЕ ОТ ТЕКУЩЕЙ ДАТЫ
(меняются помесячно), простая арифметика БЕЗ хеша/шифра: getPwd(n) = a*month*n, где
a=год%100 (или 55, если год кратен 100), затем 4 цифры результата (со 2-й снизу, старшая
первой, ведущие нули сохранены). GetPassWord="se"+getPwd(51647)+"mnf",
GetAdmPassWord="hd"+getPwd(32711)+"adm", GetServicePassWord="se"+getPwd(6911)+"srv".
(Пример: июль-2026 → se9975mnf.) GenerateLicense(sn,code): таблица простых T[10], пустой
sn→"201707182011", '/'→'-'. CheckPermission — countdown оставшихся дней от отметки
(активация=59 дней, истёк→гасит); UpdateSystemTime — двигает отметку только при включённом
доступе (антиобман по часам). Персист — КSystemSet (QSettings [Manu]); подтверждён ключ
Manu/enable, остальные (leftTime/markTime/licenseKey) не декодированы — приняты по смыслу.
С KAccount НЕ пересекается (отдельный контур; связаны лишь через общий KLogin).
ОТЛОЖЕНЫ device/крипто-части: ActiveAccount (читает лиценз-файл `SN` с USB → KUsbDevice),
GenerateLicenseFile. И **KControlProc** (прототип-/лиценз-контроль прибора, НЕ UI) —
висит на DES-классе yxyDES2 (расшифровка лицензий) + компаньоне KControlINI
(control.ini/plain.ini/matchprolist.ini/licensehistory.ini под ProtectedPath+kmachinecontrol/,
через QSettings — НЕ наш KConfig!); ini в прошивке нет (рантайм на приборе). Крупная
крипто-цепочка, отложена. ДОБАВЛЕНО в KSystemSet: Get/Set Manu{Enable,LeftTime,MarkTime,
LicenseKey} + GetProcessorSN.
РАНЕЕ (эта сессия): `KTemplateParamCfg`** (`app/src/report/`, self-test
`templateparam`) — 3-й и последний наследник KMeaXMLBase. **СЕМЕЙСТВО KMeaXMLBase ЗАКРЫТО
ЦЕЛИКОМ: KTemplateCfg ✅, KTemplateLibCfg ✅, KTemplateParamCfg ✅** (наследников больше нет —
исчерпывающе по xref на typeinfo).
⚠️ В ПРОШИВКЕ ЭТОТ КЛАСС МЁРТВЫЙ (вестигиальный): ни одного xref (ctor/Check/LoadCache/
GetTemplateParam не вызываются нигде), InitModule создаёт только два других наследника,
файла `ReportTemplateParam.xml` и каталога `report/ReportTemplate/` в прошивке НЕТ →
self-test на своей фикстуре. Реализован для полноты покрытия, на рантайм не влияет.
Хранит плоский двухуровневый key-value (группа → Name→Value), схема СВОЯ:
`<root><ЛюбоеИмяГруппы><Item Name Value/></ЛюбоеИмяГруппы></root>` — имя группы
произвольно (фильтра нет), внутри группы учитываются только элементы `Item`, и лишь если
ОБА атрибута Name/Value непустые; пустые группы отбрасываются.
ОТЛИЧИЯ ОТ СИБЛИНГОВ: `Check` РЕАЛЬНО ИСПОЛЬЗУЕТ arg1 как базовый каталог (два других
игнорируют оба аргумента и берут пути из KEnvConfig; здесь KEnvConfig не вызывается вовсе),
arg2 игнорируется; `GetTemplateParam` при промахе — честный 0 и out НЕ трогает (в отличие
от KTemplateLibCfg::GetTemplateLib, отдающего m_data); доп. слотов vtable нет.
Странность реф. 1:1: STR_REPORT_LIB_DIR уже с '/', но разделитель добавляется ещё раз →
двойной слэш в пути (Linux нормализует).
РАНЕЕ (эта сессия): `KTemplateLibCfg` + `report_template::*`** (`app/src/report/`,
self-test `templatelib`) — ВТОРАЯ ветка шаблонов (заводские «кирпичи»), закрывает
KMeaXMLBase-семейство (наследники: KTemplateCfg ✅, KTemplateLibCfg ✅, KTemplateParamCfg ⏳).
`KReportTemplateCommonDef.h/.cpp` — GenerateIDByPath (JOIN), RevertPathByID (SPLIT:
пустые токены в начале/середине СОХРАНЯЮТСЯ, хвостового пустого НЕТ), GetParentItemID,
GetSubItemsID (pre-order DFS, СОБСТВЕННЫЙ ID не включается), MergeSubItem, MergeData.
СЕМАНТИКА СЛИЯНИЯ (сверено дизасмом, не «по смыслу»): m_mapConfigs и m_mapItemConfigs —
**SRC побеждает**; m_lstItems — **DST побеждает**, дедуп **по m_strName и НА КАЖДОМ УРОВНЕ
ОТДЕЛЬНО** (не по m_strID!), при совпадении имени рекурсия ТОЛЬКО в под-элементы (поля
dst не трогаются), новый элемент клонируется ДОСЛОВНО (m_strID НЕ перестраивается) в КОНЕЦ
списка; out-список = ID только НОВЫХ поддеревьев.
`KTemplateLibCfg` (наследник KMeaXMLBase, НЕ синглтон, НЕ UI): Check (5 путей, аргументы
игнорирует), LoadCache (SubContentList.xml → m_data, плоский пул 11 блоков),
LoadCacheGroup (доп. 6-й слот vtable; TemplateTypes.xml → 5 групп ReportTemplateNP-*),
LoadTemplateLib/LoadTemplateLibs, GetTemplateLib, UpdateTemplateLib ×2, RemoveNotUserItem.
КОДЫ реф.: **-40** — XML не загрузился, **0** — нет корня "root" (а НЕ -40), **1** — успех;
битая запись по Ref пропускается МОЛЧА (не ошибка). Схема список-файлов СВОЯ:
`<root><Item Format="TemplateContent" Ref="X.xml"/></root>`; имя группы = basename Ref до
первой точки. СТРАННОСТИ РЕФ. (воспроизведены 1:1): `GetTemplateLib` при промахе отдаёт
**ссылку на m_data, а НЕ nullptr** (и ничего не вставляет); обе перегрузки
`UpdateTemplateLib` **НИКУДА НЕ ПИШУТ** — делают копию, прогоняют RemoveNotUserItem,
копию ВЫБРАСЫВАЮТ и просто перечитывают с диска (persistence-пути в достижимом коде нет);
имя в `UpdateTemplateLib(name,data)` игнорируется; 3-й аргумент LoadTemplateLib
передаётся по значению и не используется; m_strUserSubContentDir строится, но не читается
(user-ветки у этого класса нет — в отличие от KTemplateCfg с фолбэком user→RO).
`RemoveNotUserItem`: безусловно чистит m_mapConfigs; критерий user — ровно m_bUserDefine;
имена берёт из ПОЛЯ m_strName (не из ключа map); множество keep = префиксы длиной **>=2**
(длины 1 исключены — из кода не выводится, почему) + полный ID; из дерева удаляет вместе
с поддеревом.
РАНЕЕ (эта сессия): `KExamNoGenerate`** (`app/src/db/`, self-test `examno`) —
генератор номеров осмотра. НЕ синглтон и не объект: sizeof==1, ctor пустой, все методы
static, состояние — два static-поля (shared_ptr<KConfig> + int index) в .bss.
Файл `<data>/protected/ExamListId.ini`, персист — **наш KConfig** (не QSettings):
`[ExamId] ExamIdIndex=<int>`. Схема номера: `<yyyyMMdd><NNNN>` + суффикс **'R' при ЛЮБОМ
ненулевом ViewType** (реф. cbnz, не «==1»; 1=камера, 0=эндоскоп; обратная функция реф. —
GetViewTypeByExamId: последний символ 'R'). ВАЖНО: `MakeExamId` инкрементит только В ПАМЯТИ
и на диск НЕ пишет — коммитит `SetExamId()` (отсюда пара CreateTemporaryExamId→TakeEffectExamId
у KExamBussinessHandler). Переполнение: `idx>9999 → idx%%9999` — это ОСТАТОК, а не сброс в 1
(10000→1, но 19998→**0000**); закреплено self-test'ом. GetExamIdIndex дефолт **0**, не 1,
и кэша не имеет (читает файл каждый раз). SetExamId(idx<0)→0. IsValidExamId — заглушка
`return true` (аргумент не читается; MIN/MAX_DATE_RANGE в TU есть, но ни на что не ссылаются).
ОТЛОЖЕН: **KExamBussinessHandler** (21 метод, CORE, НЕ UI) — висит на нереализованных
KExamNoGenerate(теперь есть)/KPatientMngExamStatus/KExamListDBTableHandler/KExamDataFileNameGenerator
+ структуры KPatientEntry(0x1b0)/KExamEntry(0x3c8)/MainUiPatientInfo(0x128), у которых
ИМЕНА ПОЛЕЙ из бинарника не восстановимы (известны только смещения) → 1:1 сейчас = фантазия.
Часть методов тянет железо: FinishSaveDataAction (KEndoScope/KCamera/DicomStore),
GetSaveDataPath (KUsbDevice). Схему `endodata/` строит НЕ он, а `KSaveFile::CreateFilePath`
(литерал "endodata/" в бинарнике ровно один, там); направление вызовов KSaveFile→Handler.
РАНЕЕ (эта сессия): `KMeaStringUtil`** (`app/src/report/`, self-test `strutil`) —
строковые утилиты, на которых сидит отчётная ветка (report_template::ConvertStringToMap,
KTextBlock::FontSize, KImageBlock::Url, KTableBlock::Margin…). Класс ПУСТОЙ и без состояния,
но методы НЕ static (реф. ctor/dtor = голый ret) — потребители делают `KMeaStringUtil u;`.
СЕМАНТИКА НЕИНТУИТИВНА (закреплена self-test'ом, не менять «по здравому смыслу»):
SplitStr — разделитель это НАБОР символов (find_first_of), SplitStr2 — ПОДСТРОКА (find);
оба пропускают пустые токены и НЕ тримят; ПУСТОЙ разделитель → ПУСТОЙ вектор, а не {s};
разделитель не найден → {s}. TrimBeginEndStr* тримит ТОЛЬКО пробел ' ' (не \t\n\v\r\f);
TrimAllStr удаляет ' '/'\t'/'\n' ОТОВСЮДУ (не с концов), '\r' не трогает.
ConvertStringTo{Int,Double} без валидации и исключений: "abc"→0, "12abc"→12, "0x10"→0.
ConvertDoubleToString — stringstream (6 знач. цифр), НЕ std::to_string: 100.0→"100",
1234567.0→"1.23457e+06". IsBeginWith/IsEndWith с пустым префиксом → true.
ReplaceIllegalChar меняет только ПЕРВОЕ вхождение каждого из 9 символов (\ / : * ? " > < |).
ReadChars — векторы НЕ очищаются (append), пустые токены сохраняются, хвост без ';' теряется.
БАГИ РЕФ. (не воспроизводим намеренно): ConvertIntToFormatString при width>=100 пишет за
100-байтовый буфер (buf[snprintf()]) — у нас буфер по размеру; SearchStr не экранирует
left/right (regex-метасимволы, битый шаблон бросает) — оставлено как в реф.
16 из 22 методов в самом X2000 не вызываются (внешний API/мёртвый код).
СНЯТ КАНДИДАТ: **KFactoryOptions** — по дизасму это Qt-ДИАЛОГ (KDialog→QDialog, 39 методов,
гейт по роли аккаунта), а НЕ конфиг-класс: off-device там лишь ReadTestEnv/SaveTestEnv/
GetTestConfPath/CopyConf (2 ключа: Env/Scope в data/presetdata/syspreset/testenv.ini и
AgeTest/IsAgeTest в data/protected/syspreset/testenv.ini, оба через QSettings, НЕ KConfig).
Остальное тянет UI/железо. ВЫВОД ПРО КАРТУ: домен/off-device в `docs/COVERAGE.md` — эвристики
ИМЁН, они врут (KFactoryOptions помечен UPDATE/✅). Перед взятием кандидата из карты
ОБЯЗАТЕЛЬНО проверять дизасмом, что это не диалог и на чём персист.
РАНЕЕ (эта сессия): структуры данных отчёта + KTemplateCfg.**
`app/src/report/KReportTemplateData.h` — центральные структуры (их не было вовсе; на них
завязаны KTemplateCfg/KTemplateLibCfg/KTextBlock/KImageBlock/KTableBlock/report_template::*):
`KReportTemplateItem` (0xF8: m_strID=parentPath+"/"+Name, Name/Title/Type/DataSrc/Column/
ShowTitle + список детей — рекурсия <Content>), `KReportTemplateItemConfig` (0x58),
`KReportTemplateDataNew` (m_mapConfigs/m_lstItems/m_mapItemConfigs).
КЛЮЧЕВОЕ (иначе фантазия): ВСЕ поля Item — СТРОКИ (реф. as_string("")), отсутствующий
атрибут = "" (не 0/false); Column/ShowTitle тоже строки — в числа их конвертируют
потребители. `KReportTemplateItemConfig` НЕ раскладывает Section/AlignH/FontType/
ColumnRatio/SplitLine* по типизированным полям — кладёт ВСЕ атрибуты as-is в
map<string,string> (включая сам Name); "25,50,25" остаётся строкой, split делают
потребители. Единственное типизированное поле — m_bUserDefine (== "1"). Единственный
дефолт парсера: пустой ShowTitle + Type=="RT_TITLE_TABLE_BLOCK" → "1".
Type/DataSrc — строки, НЕ enum (сравниваются с report_template::STR_*).
`app/src/report/KTemplateCfg` (наследник KMeaXMLBase, НЕ синглтон): Check (задаёт каталоги;
**аргументы игнорирует** — сверено дизасмом), LoadCache (заглушка return 1), GetTemplateFileName
("Template(<name>).xml"), GetTemplateFiles (entryList+regexp), GetLib/UserTemplateFiles,
GetTemplateCfg (кэш по ИМЕНИ ФАЙЛА → промах: сначала user-ветка, фолбэк на RO),
GetSubTemplateData (только кэш), Update/DeleteTemplateCfg, Parse/SaveTemplateFile +
Parse/Save{Config,Content,ItemConfig}. Каталоги (оба литерала с завершающим '/'):
RO = GetReadOnlyBaseDir()+"mainapp/patient/report/template/FullTemplate/",
user = GetUsrDir()+"patient/report/template/FullTemplate/" — асимметрия "mainapp/" НЕ опечатка.
Реф. держит ТРИ map-кэша (+0x70/+0xa0/+0xd0); доказан только +0xa0 (его читает GetTemplateCfg) —
роли двух других не восстановлены, НЕ воспроизводим. Self-test `templatecfg` на 5 реальных
файлах (Content=59 узлов, TemplateConfig=10, ItemConfig=34 — сверено независимым разбором).
РАНЕЕ (эта сессия): фундамент XML-подсистемы — `KMeaXMLBase` + `KEnvConfig`.**
`app/src/report/KMeaXMLBase.h/.cpp` (реф. dialog/patient/reporttemplate/pugi/) — база ВСЕХ
XML-конфигов: static LoadXMLFile/ParseXML/IsFileExist/ReplaceUserByLib, чисто виртуальные
Check/LoadCache (контракт наследника; база их НЕ диспетчирует — зовёт владелец,
реф. KReportTemplateManager::InitModule), GetModuleVersion (=-1 «версия не объявлена»),
CheckVersion, IsValid, хелперы FindByName/Value/Property/FindDataNode/SetNodeValue/Property.
Коды возврата реф.: 1 успех, -2 исключение, -23 copy failed, -40 парсинг/файл-не-найден,
-46 версия. Наследники (исчерпывающе): KTemplateCfg, KTemplateLibCfg, KTemplateParamCfg.
ВАЖНО: реф. на вендоренном pugixml (measure::pugi), у нас его НЕТ → API повторён на
QDomDocument; выровнено вручную: pugi парсит без parse_ws_pcdata (whitespace-only текст
отбрасывается — QDom хранит, фильтруем), FindDataNode берёт строго pcdata (CDATA НЕ
считается), PI/DOCTYPE реф. отбрасывает, детали ошибки парсинга не логируются.
`app/src/sys/KEnvConfig` — синглтон путей (весь класс = 3 геттера, Set*/Init в реф. НЕТ):
GetBaseDir=DataPath (кэш ctor), GetReadOnlyBaseDir=ProjectPresetPath (syspreset),
GetUsrDir=ProjectUserPresetPath (userpreset) — оба без кэша (реф.). Self-test `meaxml`
(на реальном syspreset/mainapp/patient/report/template/FullTemplate/Template(NP-1x4).xml).
ИСПРАВЛЕНЫ ПУТИ KSystem по дизасму: добавлены RootPath/SystemPresetPath/ProjectPresetPath/
ProjectUserPresetPath; **AppPath был неверен** — реф. AppPath = DataPath+"app/", а не корень
прошивки (вызывающих не было, поправлено); UserPresetPath оставлен синонимом
ProjectUserPresetPath (на него завязаны вызывающие).
ПРО ДВЕ ВЕТКИ ШАБЛОНОВ (уточнено реверсом KTemplateLibCfg — РАСХОЖДЕНИЯ НЕТ, отбой
прошлой тревоге): в оригинале ОБЕ ветки существуют и разведены по классам —
`KTemplateCfg` → `FullTemplate/Template(NP-*).xml` (склеенный результат, 5 файлов),
`KTemplateLibCfg` → `SubContentList.xml`/`TemplateTypes.xml`/`ReportTemplateNP-*.xml`
+ `SubContent/*.xml` (11 заводских «кирпичей») — это ровно то, что читает наш
`KReportTemplate`. Пересечение только через static `KTemplateCfg::ParseTemplateFile`.
Рефактор НЕ нужен; при реализации KTemplateLibCfg — свести с нашим KReportTemplate.
РАНЕЕ (эта сессия): INI-движок ядра `KConfig` + два фасада над ним.**
`app/src/kernel/KConfig.h/.cpp` (реф. architecture/src/kernel/ini/KConfig.cpp) — self-test
`kconfig`. ПОЧЕМУ ОТДЕЛЬНЫЙ ДВИЖОК, А НЕ QSettings (важно!): реф. сериализует bool как
**"True"/"False"** (не true/false и не 1/0), при чтении false ТОЛЬКО для FALSE/F/NO/N/0
(регистронезав.), всё прочее — включая "" и "2" — true; комментарий ТОЛЬКО '#' (';' и '//'
не поддерживаются) и вырезается ВМЕСТЕ с '\n' (строка-комментарий склеивает соседние!);
Save() перезаписывает файл целиком, лексикографический порядок секций/ключей, пустая строка
после КАЖДОЙ секции, комментарии и текст до первой '[' теряются; числа — ostringstream
(6 знач. цифр → 1234567.0 даёт "1.23457e+06", а IsValidNumberStr не знает 'e' → warning при
обратном чтении); WriteData правит только память — на диск пишет лишь Save(); дефолты НЕ
персистятся (в файле только явно записанные ключи). ItemMustExist = assert (в NDEBUG no-op).
Поверх движка: **KPatientListConfigSetupHandler** (patientsetup.ini) и
**KWorkListConfigSetupHandler** (worklistsetup.ini) в `app/src/db/` — фасады без состояния
(единственное поле shared_ptr<KConfig>), синглтон через std::call_once → сырой указатель
(НЕ ссылка, в отличие от KExamListConfigHandler), std::string вместо QString, секция
[ShowOnMainUi], дефолт ВСЕХ bool — true. Опечатки реф. сохранены намеренно: `IsShowPatietID`,
`IsShowRegisterNumer`, класс `KWorkList*` в файле `KWorklist*.cpp`. GetColumnIsShow — map
«имя колонки → 0/1», ровно 7 записей, имена ≠ ключам .ini. Worklist: Is*On — видимость,
«голые» ключи — ЗНАЧЕНИЯ фильтра запроса. Добавлен KSystem::ProtectedPath (DataPath+
"protected/", сверено дизасмом). Self-test `listsetup`.
РАНЕЕ (эта сессия): конфиг DICOM-сервисов `link-dicom.json`** — 8 классов
(KSysDICOMData, KDICOMConf, KDICOMLocalConf, KDICOMServiceBaseConf, KDICOMStorageConf,
KDICOMWorkListConf, KDICOMMPPSConf, KDICOMCommitStorageConf) в `app/src/dicom/KSysDICOMData.h/.cpp`,
self-test `dcmconf`. Схема/дефолты/порядок ключей сверены с дизасмом 1:1 (namespace
LINK_DICOM: Local/MPPS/WorkList/Storage/CommitStorage; дефолты Local 104/60/"AE",
WorkList MaxDownloadNum=99, CommitStorage IsEnable=true; ReqProcDesc — **int**, не строка;
вложенный CommitStorage-объект внутри каждой записи Storage). Добавлены
KSystem::SetDataPath/UserSetPath (реф. DataPath+"setdata/"+"userset/", сверено дизасмом).
Сохранены странности реф.: `Save()` пишет КЭШ m_jsonObj (а не текущие поля) — правки летят
в файл только через `Save(conf)`; `Load()` открывает файл ReadWrite (создаёт пустой →
лог parse error при первом запуске). Добавлен `tools/selftest.sh` (регрессия всех режимов).
НЕ ОПРЕДЕЛЕНО (не фантазировать): полей ServiceId/ServiceName в реф. НЕТ (есть Name +
безымянный int-дискриминатор типа сервиса на +0x28, в JSON не пишется);
LINK_DICOM::DATE_FORMAT инициализируется, но ни одна функция бинарника к нему не обращается
(AddTime форматирует вызывающий UI-код).
РАНЕЕ: ПИВОТ на CORE (по решению пользователя) — новые классы
languageConfig (`language`), KLoadUnicodeText (`unicodetext`), KEncStyle matched-scope
(`encstyle`), KExamListRecordFileUpdate::GetFiletypeNumFromPath (`recfiles`); расширен
KStyleConfig::ScopeInfo (video.ini: rotateType/zoomRatio/диаметры/workLength/defaultMatch —
это off-device части KEncStyle getScopeRotateType/GetScopeZoomRatio/getScopeParaDefault/
getIsDefaultMatch; проверка в `scopecut`).
Ранее по KVideoProxy: SendRBCValue+SwitchCHbStatus; Dehaze/HDR; SetDemoire; обёртки.
**Последняя сессия (KPlControl/KVideoProxy):** (1) полный аудит register-методов KPlControl
vs дизасм — исправлена фантазия SetGammaLut, сигнатуры LUT-загрузчиков выровнены под
бинарник (void + чтение AlgParaManager); (2) реализована corner-cut геометрия (round/octagon,
self-test `cornercut`) + scope-info video.ini (`scopecut`); (3) недостающие KPlControl —
SetLens/SetGlassType/AuroraTxReset; (4) карта регистров PL вынесена в `ctrl/KPlRegs.h`
(#define REG_*); (5) KVideoProxy разблокирован для ui_preview (gst за HAVE_GST) — 40/116
методов, конвертеры фикс.-точки + тонкие/командные обёртки, self-test `fxpt`.
**РУБЕЖ 1: у каждого конфига прошивки (presetdata) есть off-device-ридер с self-test.
РУБЕЖ 2: register-ядро KPlControl полностью сверено с дизасмом (1:1 сигнатуры+логика).**

**ТЕКУЩАЯ АКТИВНОСТЬ (для новой сессии): CORE/конфиг-классы off-device по ROADMAP.**
СЛЕДУЮЩИЕ КАНДИДАТЫ (см. полный ранжированный список — `docs/COVERAGE.md`):
(0) **KReportTemplateDataNew + KReportTemplateItem + KReportTemplateItemConfig** — БЛОКЕР:
центральная структура данных всей отчётной подсистемы, у нас её НЕТ вовсе; на неё завязаны
KTemplateCfg/KTemplateLibCfg/KTextBlock/KImageBlock/KTableBlock/report_template::*.
Делать ПЕРВОЙ, затем KTemplateCfg поверх (фундамент KMeaXMLBase уже готов);
(1) **KTemplateCfg (19–26)** — грузит FullTemplate/Template(NP-*).xml (НЕ TemplateTypes/
SubContentList — это другой класс); UI не тянет вовсе, весь off-device; в ту же итерацию —
сверка/рефактор нашего KReportTemplate (см. выше про расхождение источника);
(2) **KControlINI (24)** — control.ini/plain.ini/licensehistory.ini/matchprolist.ini
(machine control/лицензия), РИСК: шифрованный вход (CipherFile2PlainFile+md5sum),
формат придётся реверсить — может съесть итерацию;
(3) остальные CORE/MISC-конфиги — переснять аудит `comm -23` (бинарник vs наши классы).
ВАЖНО: у прочих `*ConfigSetupHandler`/`*Config`-классов реф. сидит на **KConfig**, а не
на QSettings — новые фасады строить поверх `kernel/KConfig` (готов), иначе разойдётся
формат файла (True/False). Проверять дизасмом, что именно зовёт класс.
СДЕЛАНО в эту сессию: KConfig, KPatientListConfigSetupHandler, KWorkListConfigSetupHandler.
Ранее по KVideoProxy (отложено): пополнение off-device-методов из дизасма (57/116).
Инфраструктура: KVideoProxy теперь компилируется в ui_preview (gst-тракт за #ifdef HAVE_GST),
self-test `fxpt`. Регистровая карта PL — `app/src/ctrl/KPlRegs.h` (#define REG_*).
СЛЕДУЮЩЕЕ (по порядку, самодостаточные): SetFreezeCalResolution, SetAECValue/SetAGCValue,
прочие тонкие/командные обёртки к pl_. ОТЛОЖЕНО (глубокие цепочки — НЕ фантазировать):
RBCValueAdd/Sub/Set (→KVideoParam gain→KVideoSet::SetColor*Value→KUserSet/KUserOsdSet
SaveColorConf+сигнал; поля 0x58/0x5c), SwitchVLSMode→KPlControl::SetVLSMode (→AlgParaManager::
GetVistValue: GetSystemStatus[0x3c]+setAwbPara). Device-bound: PLInit(KHalGpio), SetEndoIrisType,
SetImageEnhanceType (GetEndoScope), ImageSavePreset.
———— НИЖЕ: история аудита KPlControl (ЗАВЕРШЁН, все Set*/Read* сверены) ————
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
СИГНАТУРЫ ВЫРОВНЕНЫ ПОД БИНАРНИК (методология «структура функций = дизасм»): раньше
Gamma/Knee/Rbc/Iris/Denoise брали данные параметрами (архит. выбор) — теперь как в реф.
это void-функции (SetIrisTable(int shift)), читающие «текущие» данные из синглтона
AlgParaManager (SetCur*/Cur* — типизированный аналог его массива; заполняет оркестрация
KVideoProxy). SetCCM0Matrix(int[9])→SetCCM0Matrix(uint*,int) как в дизасме. plreg/filt —
адреса и число записей не изменились, PASS. Реф. также poll-ждёт бит2 в ctrl-регистре
(device-timing, на десктопе не воспроизводим — опущено).
ЗАХОД 2 (Vist/Denoise/Iris/Corner): ПОДТВЕРЖДЕНЫ верными без правок —
SetVistSwitch (0xa18e0000=en, 0xa1840000=!en), SetVistMatrix (пары→0xa18e0004.. +
16-бит хвост p[last]→0xa18e0014, как CCM), SetDenoiseLevel (0xa1940008),
SetDenoiseLut (4 dpc→0xa194x010 + kernelG 41×4@0x1600 + kernelRB 25×4@0x1500 +
LUT 256×4@0x1100, банки=смежные окна), ReadIrisValue (0xa18a0004, младший байт).
РЕАЛИЗОВАНА новая SetCornerCutWay (была не реализована): декодированы геометрии
AlgParaManager::SetCutCornerPara→ResetCutCornerPara(=W)/SetRoundPara(круг: cut=
min(W-2c,2·√(b²−y²)))/SetOctagonPara(8-угол: пандус (W-2c)−2·(p2/p3)·y + плоскость
W-2c); стрим 1080 слов в 0xa18c8000. W/H (реф. поля +0x10/+0x14) — SetCutCornerSize
(источник device-config кадра). Self-test `cornercut` (симметрия/обнуление/дуга/
пандус/стрим). Вызывающий KVideoProxy::SetCornerCutWay device-bound (живой эндоскоп) —
не реализован. SetIrisTable — СВЕРЕНА верной: reg=0xa18a8000+cnt/2 (шаг 4), 8040 знач.→
1005 регистров, упаковка 8 нибблов (data[k]>>shift)<<(k*4); различие только в источнике
(arg-указатель vs AlgPara[0x7a48]).
**АУДИТ register-методов KPlControl ПОЛНОСТЬЮ ЗАВЕРШЁН — все Set*/Read* сверены с дизасмом.**
ДОРЕАЛИЗОВАНЫ недостающие методы (62/63): SetLens (0xa1890000 en / 0xa1890004 param из
AlgPara 0x7a40), SetGlassType (игнор тип → SetLens(0)), AuroraTxReset (строб 0xa1000014
1→usleep→0) — проверка в plreg (lens/glass/aurora, 6 записей). ОСТАЛОСЬ device-bound:
PLInit (KHalGpio::SetEmio(432,1) — нужен GPIO-HAL), SetEndoIrisType (живой GetEndoScope/
GetSensorType), InitColorEnhPara (8 значений AlgPara[0x4cb..0x4d2]→0xa18f0008..0x24 —
нужен источник ColorEnh-init конфига), ImageSavePreset (device/файл).
CORNER-CUT ЦЕПОЧКА off-device достроена: (1) геометрия AlgParaManager + стрим KPlControl
(self-test `cornercut`), (2) источник параметров KStyleConfig scope-info из <style>/scope/
video.ini (defaultRoundCut=радиус, defaultOctangleCut=(p2<<16)|p3, shapeType, videoSize;
секции — hex-код имени скопа ConvertSrc2Enc, фолбэк [Default]; реф. KEncStyle::
getScopeDefault{Round,Octangle}Cut) — self-test `scopecut`. ОСТАЛОСЬ device: KVideoProxy::
ReadCornerData/SetCornerCutWay (way/b/c с EEPROM живого эндоскопа), поля W/H(+0x10/+0x14).
KVIDEOPROXY РАЗБЛОКИРОВАН ДЛЯ ui_preview: gst-тракт (pipeline/appsink/callbacks) спрятан
за `#ifdef HAVE_GST` (endostation), off-device-логика класса теперь компилится+тестируется
на Mac. Это заодно проверило прошлые правки KVideoProxy (выравнивание void-сигнатур).
Реализованы из дизасма конвертеры фикс.-точки: Float2FixedPointNumber(f,a,b) (Q(a).(b),
scale=2^b, насыщение 2^(a+b)−1), FixedPointNumber2Float(x)=(x&0xfff)/4096, IncreaseValue/
DecreaseValue (клампы) — self-test `fxpt`. KVideoProxy: 26→36 методов (+6 тонких обёрток SetColorR/B/CLevel-SetCutPara-SetRealtimeVideoState-SetVideoDisPlay к KPlControl, имена с опечатками как в реф.) (из 116; остальные
device: camera/gst/endoscope).
Добавлены command-последовательности SetHorizontalMirror (mode==4 → 2 записи 0xa0048010)
и SetRotateType (type==2 → 3 записи + паузы) — рег. REG_CAM_CMD, self-test `fxpt`.
KVideoProxy 26→38/116.
ОТЛОЖЕНО (глубокая многоклассовая цепочка, не «фантазировать»): RBCValueAdd/Sub/Set/
RBCModeSwitch — тянут KVideoParam::Set{R,B,S}Gain (в реф. зовут KVideoSet::SetColor*Value
→ KUserSet/KUserOsdSet::SaveColorConf + сигнал VideoParamChanged; поля 0x58/0x5c с
неясной семантикой max). Нужен отдельный заход с реверсом KVideoSet color-value + user-persist.
Добавлены SetMonitorCtrl (REG_MONITOR_CTRL 0xa0060040=((v·100000)<<4)|3) и
GetPLRegisterValue (ReadValueFromPL) — self-test `fxpt`. KVideoProxy 26→40/116.
ОТЛОЖЕНО (глубже): SwitchVLSMode→KPlControl::SetVLSMode тянет AlgParaManager::GetVistValue
(GetSystemStatus[0x3c] + внутр. состояние + setAwbPara) — реверснуть GetVistValue отдельно.
SetImageEnhanceType — device (GetEndoScope).
ЗАХОД AEC/AGC+Freeze (из дизасма 0x6d9c50..0x6da1e4): (1) SetAECValue/SetAGCValue/
SetAECAndAGCValue выровнены 1:1 — дедуп не «молчащий», а с keep-alive static repeatCnt
(при том же значении пишет первые 2 повтора и освежается каждый 190-й, prev>0xbc;
свой счётчик у каждой из трёх функций; поля: AEC@+0x30, AGC@+0x2c, режим тракта @+0x24);
(2) getAecValue(float): мс→код AEC по режиму — m0: t·72000·16/1080, m1: t·40000/794,
m2 (double): 2307−(t·72000−112)/520, m3: t·40000/765, неизвестный → лог+10;
(3) SetFreezeCalResolution(w,h): PIP-окно KDisplayOption::getFreezeVideoRect
([VIDEO]/IMAGE_PIP layout-ini; пустой rect → лог+выход) → SetFreezeScalerIn(w,h)+
Out(rect.w,rect.h)+Ratio(Q5.8 w/rect.w, h/rect.h)+SetFreezeVideoLoc(x,w,y,h).
Всё покрыто в `fxpt` (теперь требует ENDO_ROOT — layout для freeze-теста):
формулы getAecValue, keep-alive 16 записей за 191 вызов (1,2,3,191-й),
байтовые команды камеры AEC (0xc00/0xd00|бит31)/AGC (0xa00/0xb00, 3 бита),
пара REG_AEC_AGC, freeze-регистры 5 записей. KVideoProxy 40→43/116.
ЗАХОД тонких обёрток (дизасм-декод отдан субагенту): SetImgDenoiseLevel (→pl SetDenoiseLevel
0xa1940008 + KVideoParam), SetContrastLevel (0xff-цикл [0..2]; контраст входит в гамму →
pl SetGammaLut; модуляция гаммы контрастом в CalGammaLut пока НЕ реверснута — освежается
текущая гамма, структура вызовов=дизасм), SetBrightnessEQLevel (0→выкл; ≠0→enable+SetBrightEQLut),
SetLensSize/SetEnhanceSize(пустой в прошивке)/SetAwbCut/SendCHbLevel (1:1 к KPlControl).
Покрыто в `fxpt` (thin/brightnessEQ/contrast). KVideoProxy 43→50/116.
ОТЛОЖЕНО (deep-chain, не фантазировать): SetColorEnhanceLevel (тянет KSystemStatus::
IsColorEnable-гейт + KVideoSet::GetColorEnhValue), SetImageEnhanceLevel (KVideoSet::
GetImgEnhValue) — нужен реверс KVideoSet color/img-value.
ЗАХОД SetDemoire (дизасм-декод субагентом; батч SetVideoArea/SendZoomValue/SetDemoire/
SetAWBValue/SetVideoCaptureArea): реализован ТОЛЬКО SetDemoire — тоггл статуса муара в
KVideoParam (добавлено поле demoire_ + SetDemoire/DemoireStatus), pl SetDemoireEN
(0xa18501cc), переприменение image-enhance (вкл→0, выкл→восстановить текущий уровень;
в нашей архитектуре SendImageEnhanceValue сам резолвит значение из AlgPara). Self-test
`fxpt` (demoire toggle on/off, 2+2 записи). KVideoProxy 50→51/116.
ОТЛОЖЕНЫ с сохранёнными формулами (device/EEPROM — НЕ фантазировать):
• SendZoomValue: f=(float)v; soft-endo → f*=KEndoScope::GetZoomRatio() (живой скоп);
  fx=Float2FixedPointNumber(f/10, 4, 16) [Q4.16]; pl SetZoomValue(0xa18d0004).
• SetAWBValue: temp/tint из EEPROM эндоскопа ([+0x1e]/[+0x20] u16) или CameraParam
  ([+0xc]/[+0xe]); если temp≤999 → дефолты (1843/1208); иначе gain=temp*1024/1000
  (magic 0xd1b71759>>45 ≈ ·1.024); pl SetAWBValue(rGain,bGain). AlgPara тут НЕ источник.
• SetVideoCaptureArea: точка захвата из KVideoSet::GetCaptureAreaPositon + знаковая
  коррекция по KEndoScope::IsVideoCalReveral/GetEndoInfo/EEPROM ([+0x10]/[+0x14]).
• SetVideoArea: rect дефолт {0,0,1920,1080}; soft-endo → KVideoSet::GetVideoArea
  (у нас не смоделирован); кламп height==768→756; pl SetVideoArea→AlgPara::resize
  (не пишет PL напрямую — нечем проверить через trace). SetVideoCentorPoint — символа НЕТ.
ЗАХОД Dehaze/HDR (дизасм-декод субагентом; батч из 8 тогглов/статусов): реализованы 4 —
SetDehazeStatus/SetHDRStatus (модулируют ГАММУ, не свой PL-регистр: статус→KVideoParam +
перезагрузка гамма-LUT реф. UpdateGammaDownloadLut→SetGammaLut; взаимоисключение: включение
одного гасит другой) + SetDehazeSwitch/SetHDRSwitch (0xff→тоггл [0..1]). Добавлены поля
KVideoParam dehaze_/hdr_ + Set/StatusGetter (как demoire_). OSD-сообщения оригинала
(KUiMsgProxy::DisplayMsg) опущены (UI/device). Self-test `fxpt` (dehaze/HDR mutual-excl).
KVideoProxy 51→55/116.
ОТЛОЖЕНЫ из того же батча (deep-chain — тянут отсутствующие аксессоры, не device):
• SwitchCHbStatus: save/restore цвет+RBC вокруг SendCHbLevel — нужны KVideoSet::
  GetColorEnhValue(int) + KVideoProxy::SendRBCValue(int,int,int) (нет).
• SetColorEnhanceValue(level,value): нужны KVideoSet::SetColEnhValue(int,int) +
  KVideoParam::SetColorEnhConfig(int,int).
• SetImageEnhanceValueByType(type,value): нужны KVideoParam::SendSetImageEnhLevel/SetImageEnhConfig.
• ResetVideoParam: нужны KVideoParam::InitVideoParam + KVideoProxy::SetVideoParam.
ЗАХОД SendRBCValue+SwitchCHbStatus (декод зависимостей субагентом): реализованы 2 из 4
отложенных. (1) SendRBCValue(r,b,c) — тон-пакет через KPlControl::SetToneValue (уже был,
сверен 1:1: r→0xa1870004, b→0xa1870008, c→0xa1870000); НЕ трогает KVideoParam (в отличие
от RBCValueSet). (2) SwitchCHbStatus(status): KSystemStatus::SetCHbStatus + оркестрация —
вкл→нейтраль(цвет 0, тон 8,8,8)+SendCHbLevel(1); выкл→восстановить(цвет=GetColEnhLevel,
тон=RGain/BGain/SGain)+SendCHbLevel(0). Реф. читает цвет через KVideoSet::GetColorEnhValue
(3-int массив KUserSetStatus) — в нашей архитектуре SendColorEnhanceValue сам резолвит из
уровня. Self-test `fxpt` (SendRBCValue 3 записи, CHb on/off). KVideoProxy 55→57/116.
ОСТАЛИСЬ 2 из четвёрки — крупная многоклассовая инфраструктура (НЕ фантазировать, отдельный
заход с решением архитектуры): SetColorEnhanceValue/SetImageEnhanceValueByType/ResetVideoParam
тянут: KUserSetStatus 3-int color-enh массив + osd.ini-персист (KVideoSet::SetColEnhValue/
GetColorEnhValue), механизм Qt-сигнала KVideoParam::VideoParamChanged (коды image-enh
0x204-6, color-enh 0x207-9, imgEnh-level 4) для SetColorEnhConfig/SetImageEnhConfig/
SendSetImageEnhLevel, KVideoParam::InitVideoParam (перечитывает ВСЕ поля + новые:
imgEnhType 0x24, imgEnhLevelB 0x2c, rbcGroupID 0x44, rbcMax/Min 0x58/0x5c), и device-gated
KVideoProxy::SetVideoParam (IsEndoReady/IsCameraReady — сенсор). Дизасм-адреса в истории
субагента (KVideoSet::GetColorEnhValue@0x651b40, SetColEnhValue@0x6531c8, InitVideoParam@
0x623a00, SetVideoParam@0x6da298 и т.д.).
ОТЛОЖЕНЫ device (EEPROM/сенсор): SendZoomValue, SetAWBValue, SetVideoCaptureArea,
SetVideoArea (формулы сохранены выше).
РЕШЕНИЕ ПОЛЬЗОВАТЕЛЯ: пивот на CORE ROADMAP (остаток KVideoProxy требует value-based
enhance-пути + Qt-сигнала VideoParamChanged — отложено до отдельного решения по инфраструктуре).
ЗАХОД languageConfig (пивот, разведка кандидата субагентом): новый off-device класс-синглтон
`app/src/sys/languageConfig.{h,cpp}` (QObject) — читает system/platform/mutilanguageinfo.ini
([MutiLanguageInfo] LanguageType/CurrentLanguage; [GooglePath] path/tabpath к kchinesePunct.tab).
Enum `_KLanguageType` (1-based: Chinese=1..Polish=8, реверс из qm-таблицы .rodata@0x886d70
шаг 0x38; Korean/Portuguese — .qm есть, позиция не подтверждена). Поля 1:1 с бинарником
(currentLanguage+0x10/languageType+0x14/googlePath+0x18/puctPath+0x20). Сеттеры сверены с
дизасмом: setLanguageType пишет ОБА поля (stp w1,w1), setCurrentLanguage — только если
t==Chinese||t==languageType (иначе no-op); оба чисто in-memory (персист — KSystemSet, не тут).
Сигнал CurrentLanChange объявлен (ref из сеттеров не эмитит). Self-test `language`. ~53 класса.
ЗАХОД KLoadUnicodeText (пивот, продолжение CORE): новый off-device класс-синглтон
`app/src/sys/KLoadUnicodeText.{h,cpp}` — читает system/language/multi_language_unicode_2_text.xml
(MachineType PAD→KeyboardVersion 1.1→LanguageType×5 Russian/Latin/French/Polish/Hungary→
Key SONO_*→символ). Структуры реф. stUnicode2TextLayout/stUnicode2TextMap; лукап
FindTextFromUnic2TextLayoutLib(machine,keyboard,keyName,lang) — обход раскладок→язык→ключ,
не найдено→пусто. Спец-правило name2value: текст "name2value" → символ из value=-атрибута
(для &,<,пробел). ВАЖНО: файл — НЕвалидный строгий XML (голые &/</пробел в value=), поэтому
парсим построчно регэкспами (оригинал тоже лоялен). Полная цепочка реф.: GetSingleUnicodeText
(keycode,lang)=KKey2Name::GetNameOfKey(keycode→SONO_*, DEVICE-клавиатура)→FindText — реализовано
off-device ядро (парс+лукап), int-код→имя отложено (device). Библиотека — синглтон (в реф.
файловый глобал g_s_vec_stUnic2TextLayoutLib). Self-test `unicodetext` (ё/!/name2value &,<). ~54 класса.
ЗАХОД KEncStyle (пивот, продолжение CORE): новый off-device класс `app/src/sys/KEncStyle.{h,cpp}`
(matched-scope слой) — читает <style>/<series>/<brand>/scope/matchedScope.ini (секции по модели
продукта [Default]/[X-2600]/[X-2600S/A/B], ключи `scope=`/`camera=` через запятую). Методы 1:1
(вкл. опечатку GetSupprotedCameraList): getSupportedScopeList/GetSupprotedCameraList (секция
[<Model>] с фолбэком [Default]; Model = реф. KSystemSet::GetProductModel; split — нативный
Qt-INI по запятой toStringList), IsScopeValid (нормализация "…LT…"+"EC"→"…L/T…", членство,
case-sensitive), IsCameraValid (членство). Путь строится через готовый KStyleConfig::GetStylePath.
ОТЛОЖЕНО (device/отдельно): getIsDefaultMatch (video.ini + ConvertSrc2Enc(GetEndoModel живого
скопа)), "*N"-суффикс (KEndoScope::IsEndoModelHaveSuffix), кросс-проверка IsScopeValid с
глобальным enc-списком getScopeList (genc/cenc/uenc/…enc.ini). Self-test `encstyle` (30 скопов,
4 камеры, валидация+фолбэк). ~55 класса.
ЗАХОД KExamListRecordFileUpdate (пивот, CORE): новый класс `app/src/db/KExamListRecordFileUpdate.{h,cpp}`
— реализован ТОЛЬКО GetFiletypeNumFromPath (реф. 1:1: QDir(path).entryInfoList(exts,Files).size())
+ обёртки ImageFileNum(*.jpg/*.bmp/*.png/*.jpeg)/VideoFileNum(*.mp4/*.avi) на подтверждённых из
прошивки расширениях. Self-test `recfiles` (temp-каталог, счётчики по маскам, несущ.путь→0). ~56 класса.
ОТЛОЖЕНО (НЕ фантазировать): UpdateRecordFileNumToDb — реф. GetExamEntity→счёт→UpdateExamEntity,
но колонок-счётчиков НЕТ ни в нашей схеме tb_ExamList, ни в строках бинарника (имя колонки
назначения неизвестно) → DB-запись не реализуется без доп. реверса схемы.
ПРОБНЫЕ КАНДИДАТЫ, ОКАЗАВШИЕСЯ НЕЧИСТЫМИ (для истории — не тратить время повторно):
• KEncStyle::getScopeList — enc-файлы genc/cenc/…enc.ini это GBK-блобы БЕЗ ini-структуры
  (не списки кодов «EG-X20»); источник глобального scope-списка неясен, метод крупный (0x864).
• KExamListRecordFileUpdate::UpdateRecordFileNumToDb — колонка-счётчик в tb_ExamList не подтверждена.
ЗАХОД KStyleConfig::ScopeInfo расширение (KEncStyle per-scope video.ini геттеры, off-device):
video.ini содержит больше ключей, чем читалось — добавлены rotateType/zoomRatio(@Variant float)/
channelDiameter/distalEndDiameter/insertionTubeDiameter/workLength/defaultMatch в ScopeInfo +
ридер GetScopeInfo. Это off-device части KEncStyle::getScopeRotateType/GetScopeZoomRatio/
getScopeParaDefault/getIsDefaultMatch (реф. no-arg версии берут имя скопа из GetEndoModel —
DEVICE; QString-overload читает video.ini — реализовано через наш GetScopeInfo(series,brand,scope)).
Проверка в `scopecut` (EB-X20 X-2500: rotate=0/zoom=1.14/workLen=600/defMatch=true). Классов ~56.

ЗАХОД KFactoryOptions (заводские опции / стенд старения, UPDATE, off-device ядро):
новый класс `app/src/sys/KFactoryOptions.{h,cpp}`. Реф. — KDialog (sizeof 0x88, +0x50 Ui*,
+0x58 код возврата), 43 уникальных метода: ~33 write / 10 avoid / 0 unclear.
РЕАЛИЗОВАНО: GetTestConfPath (ядро), ReadTestEnv/SaveTestEnv, StopTest, CopyConf, StartTest,
UpdateReleaseVersion, OpenDebug/DebugLevel, коды возврата RESULT_* (0x11-0x16).
• **GetTestConfPath** = SystemPath()+"autotest/aging" + (check_scope?"-scope":"") +
  (PanelType==0?"-1":"-2") + (ViewType==0?"-softendo":"-hardendo") + "/".
  GROUND TRUTH: из 8 комбинаций в поставке ровно 6 — нет `aging-1-hardendo` и
  `aging-scope-1-hardendo` (кнопочная панель + жёсткий эндоскоп не выпускается). Self-test
  проверяет и имена, и соответствие «существует ⇔ ожидается» по всем 8.
• ДВА РАЗНЫХ testenv.ini (важно, легко спутать): `data/presetdata/syspreset/testenv.ini`
  — [Env] Scope (bool, дефолт true), читает ReadTestEnv / пишет SaveTestEnv; и
  `data/protected/syspreset/testenv.ini` — [AgeTest] IsAgeTest, пишется в StopTest (жёсткое
  true). В поставке обоих файлов НЕТ — создаются в рантайме самим QSettings.
  ИСПРАВЛЕНО ПОЗЖЕ (разведка KSelfTest): первоначальное утверждение «IsAgeTest никто не
  читает» ОШИБОЧНО. Читатель — **KSelfTest::checkProcessor @0x714d00**, дефолт false: при
  true самотестирование добавляет в отчёт TR_IPATestNE. Это межмодульный флаг «прибор
  побывал на стенде старения». Урок: «читателя не нашёл» ≠ «читателя нет» — разведку
  соседних классов делать ДО вывода о мёртвых данных.
• Смоделировано (нет носителя): глобалы рантайма g_euTestType + свободные функции
  SetAutoTestOpen(mode,mask)/SetUITestRecordOpen(mode) → namespace KFactoryOptionsState.
  Реф. маска стенда всегда 2048 (0x800); ветка `g_euTestType & 0x400` уводит StopTest
  в SetUITestRecordOpen(2) БЕЗ записи ini.
Self-test `factoryopt` (8 комбинаций пути + ground truth поставки, дефолт/roundtrip env,
обе ветки StopTest, CopyConf с затиранием сироты, StartTest → AppPath()/autotest/,
UpdateReleaseVersion, коды, OpenDebug-кламп). Регрессия: PASS 90 / FAIL 0. ~57 классов.
ОТЛОЖЕНО (device, адреса реф.): блок восстановления — RecoverConfigure @0x627ed8,
startSystemRecovery @0x627d50, TimeTaskRecovery @0x627d98 (таймер 1 с, 5 тиков),
RecScopeRestoreRsp @0x6281a8, RecEndoInfoRec(int,int) @0x628340, RecCameraInfoRec @0x628398,
ClickBtnClearEndoInfo @0x628488, InitWidget @0x628f78, ctor @0x629de8 — зовут
GetEndoScope()/GetCamera()/GetKHalClass(). SaveProcessorSN/SaveProcessorInfo/ClickBtnSaveOther
формально off-device, но содержат KUiMsgProxy::* (IPC к main-ctrl) + usleep.

ПОБОЧНАЯ НАХОДКА (важная, вынесена в отдельную задачу): **enum StatusType в нашем
KSystemStatus.h пронумерован НЕ так, как в бинарнике.** Восстановлено из emit-констант:
0 ViewType, 1 FullScreenUI, 2 EndoDisconnectImage, 3 Network, 4 Freeze, 5 Agc, 6 CHb,
7 Record, 8 Lamp, 9 VlsMode, 10 LowLight, 11 DimmingType, 12 LightLevel, 0xe IsVideoCal,
0xf MainScreenState, 0x10 StopWatch, 0x11 AirPump. БЕЗ emit вообще: SetPanelType,
SetWindowID, SetAutoTestStatus (у нас они шлют сигнал — расхождение).
Карта полей KSystemStatus (офсеты с сеттеров): 0x10 panelType, 0x14 viewType, 0x18 windowID,
0x1c videoStatus, 0x20 fullScreenUI, 0x24 network, 0x28 freeze, 0x2c agc, 0x30 CHb,
0x34 record, 0x38 lamp, 0x3c vlsMode, 0x40 lowLight, 0x44 dimmingType, 0x48 lightLevel,
0x4c imageBrightness, 0x74 stopWatch, 0x90 airPump, 0x94 autoTest.
СЕМАНТИКА (доказана): PanelType 0=кнопочная панель, 1=ЖК/сенсорная (HmiMcu::InitVLSGroups;
единственный писатель HmiMcu::NT_RecMcuRespondAck_CB = (byte[8]>>3)&1). ViewType 0=мягкий
эндоскоп, 1=жёсткий/камера — доказано тремя хелперами (KVideoProxy::IsSoftEndo,
AlgParaManager::IsSoftEndo, KViewBase::IsSoftEndoView — все три `[0x14] == 0`); ViewType
персистится через KSystemSet::Get/SetViewType, 109 читателей по бинарнику.

ЗАХОД X2000Video IPC (пункт A2 роадмапа) — ЗАКРЫТО АРХИТЕКТУРНОЕ БЕЛОЕ ПЯТНО:
как основной процесс X2000 общается с процессом видеотракта X2000Video.
Разведаны ОБА бинарника (X2000Video + парная сторона в X2000). Наш код:
`app/src/video/KMessageManager.{h,cpp}`, namespace x2000video.
• ТРАНСПОРТ: System V message queue (msgget/msgsnd/msgrcv, флаги 0x3B6=IPC_CREAT|0666,
  msgsnd/msgrcv flags 0x800=IPC_NOWAIT). Ни сокетов, ни POSIX mq, ни сигналов.
  Очереди ЗЕРКАЛЬНЫ: X2000 шлёт на 0x81000000 (KVideoProxy::SendCmd2VideoApp @0x6d7790),
  читает 0x82000000 (KMainCtrlThread::HandleTask @0x6cbf10). Ключи сверены МНОЙ
  дизасмом (mov w1,#-0x7f000000 / #-0x7e000000 в KMessageManager @0x9000).
• ⚠️ ВАЖНО ПРО ИМЕНА: enum `_KProcMsg` в X2000 — это КЛЮЧИ ОЧЕРЕДЕЙ, а НЕ коды
  сообщений. Коды — просто `long mtype`; именованного enum кодов в X2000 нет
  (строк KPMSG_*/KPMST_* там 0, они только в X2000Video — 13 штук). В X2000
  ровно 4 ключа: 0x81/0x82000000 (видео, владелец KVideoProxy) и 0x85/0x86000000
  (автотест, владелец KAutoTestThread; peer — X2000Simulator, его main @0x28cc
  создаёт ровно эти два; KFunTest только ПИШЕТ в 0x85, владельцем не является).
• KMsgBuf = 80 байт: long mtype @0x00 (8 б) + _Para @0x08 (72 б). msgsz ВСЕГДА
  0x48 (только mtext). Закреплено static_assert'ами в заголовке (проверка на
  КОМПИЛЯЦИИ). Разбор нагрузки по командам: 0x2001 — четыре int16
  (connect/type/w/h); MsgSend(type,int,int) — два int32 + нули.
• КОДЫ КОМАНД (входящие сверены дизасмом switch HandleMessage @0x93d8):
  X2000→Video: 0x1001 ImageSaveStart, 0x1002 VideoSaveStart (para+0 =
  GetSaveVideoQuality), 0x1003 VideoSaveStop (para+0 = причина), 0x2001 EndoConnect.
  Video→X2000: 0x80001001/2/3 *_READY (para = ret: 0 ок/-1 ошибка/-2 стоп),
  0x3001 THUMBIMAG/VOLUMN_READY, 0x3002 — X2000Video шлёт как «я жив» (param 1),
  но X2000 трактует как уведомление о смене статуса эндоскопа. Сравнение mtype в
  X2000 — 64-битное, 0x80001001 = положительный long (movk #0x8000,lsl#16).
• РАЗДЕЛЯЕМАЯ ПАМЯТЬ: shmget(key, 0x697A20, 0x3B6). Размер 6912544 = кадр
  0x697800 (6912000 = **1920*1200*3 RGB24**, арифметика сходится точно) + 544 б
  метаданных. Раскладка ImageShmLayout закреплена static_assert'ами на все 7
  полей и общий размер: volumePath@0x697800, thumbPath@0x697900, width@0x697A00,
  height@0x697A04, bpp@0x697A08(=3), recordField@0x697A10, pendingThumb@0x697A18.
  ⚠️ АСИММЕТРИЯ: X2000 использует ТОЛЬКО ключ 0xA1000000 (KImgProcThread @0x6f8b10);
  0xA2000000 существует лишь внутри X2000Video. Мангл-типы: _KProcShm (X2000).
• КОНЕЧНЫЙ АВТОМАТ пайплайнов (KEncodermanager::RunPipeline @0x7208): -1 NONE,
  0 SNAP, 1 RECORD. Реализован HandleMessage-диспетчер: снимок с холодного старта
  поднимает SNAP; повторный старт записи при активной записи → отказ 0x80001002/-1
  без трогания пайплайна; connect/disconnect рулят SNAP/NONE.
• ⚠️ ДВА ПРЕДОСТЕРЕЖЕНИЯ ИЗ РАЗВЕДКИ (воспроизведены/помечены):
  1. KSharedBuffer ВОПРЕКИ ИМЕНИ — НЕ разделяемая память, а обычный malloc-ринг
     внутри процесса. Межпроцессной синхронизации доступа к shm в X2000Video нет
     (KSemaphoreManger = внутрипроцессный QSemaphore).
  2. KPipleline::GetPipelineType @0x9a10 — МЁРТВ: поле m_type инициализируется 2 и
     никем не переопределяется, вызовов нет. Копировать семантику вслепую нельзя.
• GetVideoSavePath @0x90b8: путь записи ОБЯЗАН иметь суффикс ".mp4", иначе NULL
  (реф. strcmp(p+strlen(p)-4, ".mp4") — при длине <4 читает за границу; у нас
  короткая строка честно отбрасывается).
• Self-test `videoipc` (13 групп: ключи всех 4 каналов, раскладка сообщения,
  арифметика кадра, разбор нагрузок, диспетчер во всех ветках, правило .mp4,
  кольцевой буфер). Регрессия PASS 94 / FAIL 0.

ЗАХОД KRTCreatorContext + KRTAbsItemCreator (итерация 2 к KDocumentGenerator, пункт B1 роадмапа).
Новые файлы: `app/src/report/KRTCreatorContext.{h,cpp}`, `app/src/report/KRTAbsItemCreator.h`.
Реализованы РЕЕСТР и ДИСПЕТЧЕРИЗАЦИЯ (чистый STL, Qt не участвует вообще — Qt-типы в
сигнатурах лишь ОБЪЯВЛЕНЫ, базовые реализации их не трогают). Рендер — итерация 3.
• KRTCreatorContext sizeof 0x40: +0x00 vptr (полиморфен ТОЛЬКО из-за виртуального dtor:
  _ZTV = 0x20, _ZTI = __class_type_info без баз), +0x08 map<string, KRTAbsItemCreator*>,
  +0x38 KReportTemplateDataNew*. KRTAbsItemCreator sizeof 0x30: vptr, m_strType @0x08,
  KRTCreatorContext& @0x28; vtable 5 слотов, базовые реализации — заглушки `return true`.
• РЕЕСТР ПРОВЕРЕН МНОЙ ДИЗАСМОМ InitCreator @0x547918 (не принят из отчёта): ровно
  6 вызовов `_Znwm` + 6 `_M_emplace_hint_unique`, причём **KRTTableItemCreator::ctor
  вызывается ДВАЖДЫ**, остальные четыре — по одному разу. Т.е. 6 записей на 5 классов:
  один класс обслуживает и RT_TABLE_BLOCK, и RT_TITLE_TABLE_BLOCK, но это ДВА РАЗНЫХ
  объекта с разными m_strType (двойного delete в dtor нет).
• ДВА РАЗНЫХ ФОЛБЭКА (легко перепутать, self-test проверяет оба):
  CreateBlock  — промах по типу → **STR_RT_ELEMENT_TEXT_BLOCK**;
  UpdateBlock(QTextFrame*) @0x546788 — промах → **STR_RT_ELEMENT_TABLE_BLOCK**.
  Если и фолбэк не найден — false. Констант STR_RT_ELEMENT_* в бинарнике 23, в карту
  попадают 6 → остальные 17 типов ВСЕГДА идут через фолбэк на TEXT_BLOCK.
• AddNewCreatorType @0x548050 — insert-if-absent: повторная регистрация типа
  ИГНОРИРУЕТСЯ, объект НЕ подменяется (у нас лишний экземпляр удаляется, чтобы не течь).
• ЗНАЧЕНИЯ ТИПОВ подтверждены РЕАЛЬНЫМИ шаблонами поставки (строгий grep по атрибуту
  Type= в <Content>): RT_TEXT_BLOCK 237, RT_TABLE_BLOCK 93, RT_TITLE_TABLE_BLOCK 89,
  RT_IMAGE_BLOCK 62. **RT_IMAGEGROUP_BLOCK и RT_SUB_DATA_BLOCK зарегистрированы в коде,
  но в поставляемых шаблонах НЕ ВСТРЕЧАЮТСЯ НИ РАЗУ.**
  ⚠️ Ловушка при грепе: `Type="..."` без границы слова ловит `FontType="ThirdTitle"`
  (это FontType из ItemConfig, а НЕ тип блока) и `SplitLineType="Horizontal"`.
  Первый прогон дал ложные «типы блоков» ThirdTitle/FirstTitle/Horizontal — перепроверил
  с границей слова. Значения FontType: FirstTitle/SecondTitle/ThirdTitle.
• Self-test `rtcreator` (реестр из 6, точность типов, близнецы-таблицы как разные объекты,
  ОБА фолбэка, отклонение дубля, регистрация нового типа, сквозная диспетчеризация).
  Регрессия PASS 93 / FAIL 0. KRTAbsItemCreator — 3/3 метода (100%).
• ОСТАЁТСЯ по B1: рендер в творцах (QTextCursor/QTextTable/insertImage), GetFontSize
  (QGuiApplication::primaryScreen → physicalDotsPerInch), GetFontColor, Insert/RemoveSplitLine,
  HideInvalidBlock; затем InitDocument + GetTextDocument(QObject*). И пункт C3 —
  STR_INVALID_ITEM_ID всё ещё не восстановлена.

ЗАХОД X2000Monitor (ПЕРВЫЙ заход в ДРУГОЙ бинарник, не X2000!) — сторожевой процесс.
Реф.: update/root/X2000Monitor, 28 КБ, 31 функция, C, GCC 8.2.0, aarch64-xilinx-linux,
внутреннее имя проекта **NaChaMonitor**, исходник x2000monitor.c. Разобран ЦЕЛИКОМ.
Наш код: `app/src/monitor/X2000Monitor.{h,cpp}`, namespace x2000monitor, имена функций 1:1.
• НАЗНАЧЕНИЕ: следит за тремя процессами — X2000 (главный UI), X2000Video (видеотракт),
  X2000Simulator (фолбэк). Heartbeat через sigqueue RT-сигналами, падение — по SIGCHLD +
  скан /proc/*/status, перезапуск — fork+execlp, лог в data/app/logfile/Monitor.log.
• ПРОТОКОЛ (все числа сверены МНОЙ дизасмом, не взяты из отчёта):
  сигналы 43 = heartbeat X2000, 44 = heartbeat X2000Video, 46 = управляющий канал,
  17 = SIGCHLD, 11 = SIGSEGV. Это СЫРЫЕ номера (при glibc SIGRTMIN=34 → SIGRTMIN+9/+10/+12).
  Полезная нагрузка sival_int: sig43 val1 → синхронизация отсчёта видео с консолью,
  val2/val3 → пауза/снятие паузы мониторинга (heartbeat отмечается ВСЕГДА, независимо
  от val); sig46 val0 → poweroff, val1/2 → идёт обновление ПО, val5/6 → разрешить/
  запретить перезапуск.
  Таймаут heartbeat **5000 мс** (`mov x2,#0x1388` в mainCtrlMonitor @0x28d8), порог СТРОГИЙ
  (b.gt → реакция при >5000). Период опроса 30 мс (usleep(30000) в main).
  Почасовой сброс счётчиков **3600000 мс** (`#0xee80 | 0x36<<16` = 0x36EE80 @0x29c8).
  Порог отказов **11** (`cmp w0,#0xb` в main @0x1954/0x1960) — сдаёмся с 12-го отказа,
  g_bOpenKill=0; обратно ТОЛЬКО внешним sigqueue(46,5) — почасовой сброс счётчиков сам
  по себе g_bOpenKill НЕ восстанавливает.
  Убийство: SIGUSR1(10) → sleep(1) → SIGKILL(9) при graceful; сразу SIGKILL иначе.
  Формат лога `[%02d-%02d %02d:%02d:%02d:%03d] ` — БЕЗ ГОДА, 21 символ, мс = usec/1000.
• ⚠️ ДВЕ АНОМАЛИИ ПРОШИВКИ (зафиксированы, воспроизведены 1:1):
  1. **videoMonitor @0x2950 — МЁРТВЫЙ КОД**: ноль call site (проверено дизасмом — из
     runStateMachine вызывается ТОЛЬКО mainCtrlMonitor @0x28d8). Следствие для модели
     прибора: таймаут heartbeat ВИДЕОТРАКТА фактически НЕ отслеживается, падение
     X2000Video ловится только по SIGCHLD. Похоже на недоделку вендора.
     У нас функция реализована и работает, но runStateMachine её НЕ зовёт — как в реф.
     Self-test проверяет ОБА факта: что машина состояний её не вызывает И что сама
     функция исправна.
  2. restoreExitApp перед запуском симулятора повторно проверяет имя **X2000Video**,
     а не X2000Simulator → симулятор поднимается как фолбэк, когда видеотракт поднять
     не удалось.
  Мёртвый код также: restoreConsole/restoreVideoMain (обёртки) и mySleep (busy-wait).
• РЕАЛИЗОВАНО (чистое ядро, Qt не участвует): get_time, MonitorLogPrefix/MonitorLogPath
  (в реф. инлайн в monitor_print_log), UpdateHeartBeatTime, RecHeartBeatAct,
  mainCtrlMonitor, videoMonitor, runStateMachine, CheckFailLimit. Состояние — структура
  MonitorState с 13 полями по реф.-именам глобалов (g_bOpenKill в .data ИНИЦИАЛИЗИРОВАН
  ЕДИНИЦЕЙ, остальные нули). НАШЕ РЕШЕНИЕ: вместо выполнения действий ядро ВОЗВРАЩАЕТ
  намерение (enum MonitorAction) — иначе fork/kill/shutdown не протестировать.
• НЕ РЕАЛИЗУЕМО буквально: restartAPP, initAppProc, run_poweroff, mySystem,
  sendAppSignal, InstallHeartBeatSig, RecExceptionAct (backtrace), find_pid_by_name.
• Self-test `monitor` (14 групп: приоритет машины состояний, заморозка, границы
  таймаута 5000/5001, порог 11/12, «сдались» и оживление, почасовой сброс, обе
  аномалии, формат лога). Регрессия PASS 92 / FAIL 0.
• ЛОВУШКА, НА КОТОРУЮ Я НАСТУПИЛ: первая версия self-test падала на длине префикса лога —
  я ожидал 22 символа, а `[MM-DD HH:MM:SS:mmm] ` = 21 (года в формате НЕТ). Ошибка была
  в ТЕСТЕ, не в коде. Полезное напоминание: падение теста ≠ баг реализации.

ЗАХОД KDocumentGenerator (ЖИВОЙ генератор документа отчёта, REPORT) — РАЗВЕДКА + РАЗБЛОКИРОВКА.
Реализация НЕ начата; в этой итерации снят блокер и разобран класс.
• РАЗБОР (реф. @0x53bf38…0x541dac): 34 метода, sizeof 0x40, БЕЗ базовых классов, НЕ QObject,
  не полиморфный (хранится в shared_ptr — аллокация 0x50 в KReportTempletEditDlg::InitTemplate).
  Поля: 0x00 QTextDocument* m_pDoc, 0x08 KRTCreatorContext* m_pContext (new в ctor),
  0x10 KReportTemplateDataNew* m_pData (НЕ владеет), 0x18 std::string m_strCurItemId
  (инициализируется STR_INVALID_ITEM_ID), 0x38 bool m_bCanMoveFront, 0x39 bool m_bCanMoveBack.
  КЛАССИФИКАЦИЯ: **avoid = 0**. 14 методов write-pure (чистый STL, Qt не нужен вообще:
  Save, GetAllItemIDs, ChangeFontSet, ChangeCalcApps, SetLayoutParam, HasFooterTemplateItem,
  FindItmeIdofPreFooter, UpdateMovableFlag, SyncImageItemContent, SyncImageItemParam,
  AddSubItemData, DeleteSubItemData, SyncRefresnImageItemData, ctor), 20 write-gui (Qt5::Gui).
  ГРАНИЦЫ WIDGETS НЕТ: GetTextDocument(QTextEdit* p) использует p ТОЛЬКО как QObject*-родителя
  для `new QTextDocument(p)` → замена параметра на QObject* полностью снимает Qt5::Widgets.
• KRTCreatorContext (sizeof 0x40, БЕЗ баз, полиморфен лишь из-за виртуального dtor):
  0x08 std::map<std::string, KRTAbsItemCreator*> m_creators, 0x38 KReportTemplateDataNew*.
  Диспетчер CreateBlock — ЕДИНСТВЕННЫЙ дискриминатор: строка типа элемента как ключ map.
  InitCreator регистрирует 6 записей на 5 классов: RT_TEXT_BLOCK→KRTTextItemCreator,
  RT_IMAGE_BLOCK→KRTImageItemCreator, RT_IMAGEGROUP_BLOCK→KRTImageGroupCreator,
  RT_TABLE_BLOCK и RT_TITLE_TABLE_BLOCK→KRTTableItemCreator (ОДИН класс, два экземпляра),
  RT_SUB_DATA_BLOCK→KRTSubDataItemCreator. FALLBACK при промахе: CreateBlock→TEXT_BLOCK,
  UpdateBlock(QTextFrame*)→TABLE_BLOCK. Всего констант STR_RT_ELEMENT_* в бинарнике 23,
  в карту попадают 6 — остальные 17 уходят в TEXT_BLOCK-фолбэк.
  KRTAbsItemCreator sizeof 0x30: vptr, std::string m_strType @0x08, KRTCreatorContext& @0x28;
  vtable +0x10 CreateBlock(Item*,QTextTableCell&), +0x18 CreateBlock(Item*,QTextFrame*),
  +0x20 UpdateBlock(Item*,QTextFrame*).
  UpdateBlock = «стереть содержимое + CreateBlock заново» (first/lastCursorPosition →
  beginEditBlock → setPosition(KeepAnchor) → removeSelectedText → endEditBlock → HideInvalidBlock).
  Разделители: маркер ячейки QTextFormat::setProperty(0x1, "SplitCell"); ориентация из
  KSplitLineInfo.m_strSplitLineType ∈ {"Vertical","Horizontal"}; цвет → 0x820 BackgroundBrush.
• ЯКОРЬ ПОИСКА ЯЧЕЙКИ — свойство **0x100001 (UserProperty+1)**, а НЕ 0x100000. Значение
  0x100000 относится к мёртвому KTemplateEditDocument (см. ниже) — НЕ ПЕРЕПУТАТЬ.
  Сравнение id в FindFrameOrCell — через std::string::find, т.е. ПРЕФИКСНОЕ, не полное.
• ⚠️ ОШИБКА СУБАГЕНТА, ПОЙМАНА ПРОВЕРКОЙ (важный прецедент): отчёт разведки утверждал, что
  строка типа элемента лежит по item+0x20. ЭТО НЕВЕРНО. Проверено дизасмом InitDocument:
  `ldp x2,x3,[x20,#0x70]` при x20=узел списка (item = узел+0x10) → тип читается с **item+0x60**,
  что совпадает с нашим m_strType в KReportTemplateData.h. Субагент спутал тип с m_strName
  (+0x20), потому что сравнения RT_HEADER/RT_SIGNATURE/RT_ADDITION в InitDocument идут ПО ИМЕНИ.
  Косвенно подтверждено: m_pData+0x30 = список элементов, item+0xf0 = размер списка детей,
  item+0xa0 = строка, куда пишется to_string(children.size()). ВЫВОД: офсеты из отчётов
  субагентов проверять дизасмом ДО того, как на них построена реализация.
• РАЗБЛОКИРОВКА (сделано): наш самодельный HTML-генератор занимал реф.-имя KDocumentGenerator
  при 0% совпадения API → переименован в **KReportHtmlGenerator** (файлы, класс, 3 вызова в
  preview_main, CMakeLists ×2, комментарий в KRTDataSourceReal.h). В его заголовке теперь
  явная шапка «ЭТО НАШ КЛАСС, А НЕ РЕФЕРЕНСНЫЙ». Реф.-имя свободно. Регрессия PASS 90 / FAIL 0.
• ✅ ИТЕРАЦИЯ 1 СДЕЛАНА: создан `app/src/report/KDocumentGenerator.{h,cpp}` с реф.-раскладкой
  полей (0x00 m_pDoc / 0x08 m_pContext / 0x10 m_pData / 0x18 m_strCurItemId /
  0x38 m_bCanMoveFront / 0x39 m_bCanMoveBack; ctor обнуляет оба флага одной `strh wzr,[x,#0x38]`).
  Реализованы 6 write-pure методов: ctor, Save, ChangeCalcApps, SetLayoutParam,
  HasFooterTemplateItem, FindItmeIdofPreFooter (опечатка «Itme» — реф., сохранена 1:1).
  m_pContext = nullptr (KRTCreatorContext — итерация 2). Self-test `docgen`, БЕЗ Qt вообще.
  Регрессия PASS 91 / FAIL 0.
  ЗНАЧЕНИЯ КОНСТАНТ взяты НЕ из дизасма, а из РЕАЛЬНЫХ шаблонов поставки
  (system/presetdata/syspreset/mainapp/patient/report/template/**.xml) — доказательство
  сильнее: Section (40 вхождений, значения ровно три — Body/Footer/Header, что совпадает
  с тройкой символов STR_RT_VALUE_BODY/FOOTER/HEADER), RefColumn (14 вхождений).
  ЭТИ ЖЕ ФАЙЛЫ НЕЗАВИСИМО ПОДТВЕРДИЛИ поправку про офсеты: в SubContent/Signature.xml
  стоит `<Item Name="RT_SIGNATURE" … Type="RT_TABLE_BLOCK">`, т.е. RT_SIGNATURE — ИМЯ,
  а не тип; ровно поэтому сравнения в InitDocument идут по m_strName (+0x20), а тип
  берётся с +0x60. Полный список атрибутов шаблонов: Name/Type/Title/ShowTitle/DataSrc/
  AlignH/Column/SynColumnID/ImageWidth/FontType/Value/Section/Margin/Size/Italic/Bold/
  RefColumnID/RefColumn/ColumnRatio/MarginWidth/SplitStartIndex/SplitLineWidth/
  SplitLineType/SplitLineSpace. Атрибут **CellAt в поставке НЕ ВСТРЕЧАЕТСЯ НИ РАЗУ** —
  значит чтение CELLAT в UpdateMovableFlag на реальных данных всегда даёт пусто.
  ✅ STR_INVALID_ITEM_ID ВОССТАНОВЛЕНА через среду hermes (§0a): декомпилятор Ghidra отдал
  `string(&STR_INVALID_ITEM_ID,"Invalid ID")` из _GLOBAL__sub_I_KDocumentGenerator.cpp за
  один заход — статикой на Mac она не бралась (const-пропаган.). Значение = **"Invalid ID"**.
  Наш ctor теперь инициализирует m_strCurItemId ею.
  ✅ ДОБАВЛЕН UpdateMovableFlag @0x53c7b0 (7-й write-pure метод) — ПЕРВЫЙ метод,
  восстановленный ДЕКОМПИЛЯТОРОМ, а не ручным asm. Логика (проверена self-test'ом):
  сброс обоих флагов; ранний выход при id=="Invalid ID" ИЛИ если сам элемент cell-закреплён
  (атрибут "CellAt", значение сверено в .rodata 0x865d18); front = есть предыдущий сосед И
  он не cell-закреплён; back = есть следующий сосед И он не cell-закреплён. Соседи = дети
  родителя (id без последнего "/"-сегмента) либо корневой список. Self-test `docgen` расширен
  (first/mid/last, сентинел, self-pin, сосед-pin).
  ТАКЖЕ УТОЧНЕНО дизасмом Save: копируются ТОЛЬКО m_mapConfigs и m_lstItems;
  m_mapItemConfigs (+0x48) в Save НЕ участвует — приёмник сохраняет свою карту.
  🔧 ПОБОЧНО ИСПРАВЛЕНА ОШИБКА В МЕТРИКЕ (найдена здесь же): tools/coverage.py парсил
  только `_ZN` и терял ВСЕ const-методы (`_ZNK`) по всему бинарнику — 577 методов, из них
  104 наших. Поймано на том, что HasFooterTemplateItem/FindItmeIdofPreFooter не попадали
  в список, хотя символы есть. Добавлен пропуск CV-квалификаторов (K/V/VK) и отрезание
  ABI-тега B5cxx11 (у методов, возвращающих std::string). Знаменатель 6431 → 7008,
  реализовано 1246 → 1350; доля почти не изменилась (19.4% → 19.3%), т.е. ОТНОСИТЕЛЬНЫЕ
  оценки прошлых сессий верны, а АБСОЛЮТНЫЕ были занижены.
• СЛЕДУЮЩИЙ ШАГ (вертикальный срез, рекомендация разведки): итерация 1 — скелет реф.
  KDocumentGenerator с полями выше + 14 write-pure методов, KRTCreatorContext* пока nullptr
  (пути InitDocument — no-op). Self-test БЕЗ Qt: собрать KReportTemplateDataNew из 3-4 items,
  прогнать AddSubItemData → UpdateMovableFlag → Save, проверить порядок/вложенность списка,
  HasFooterTemplateItem/FindItmeIdofPreFooter на SECTION==Footer, m_bCanMoveFront/Back на
  границах, эквивалентность копии после Save. Итерация 2 — KRTCreatorContext + KRTAbsItemCreator
  + ТОЛЬКО KRTTextItemCreator (прочие типы всё равно уходят в его фолбэк), затем InitDocument +
  GetTextDocument(QObject*) под QT_QPA_PLATFORM=offscreen (нужен QGuiApplication из-за
  primaryScreen() в GetFontSize). ОТЛОЖИТЬ: KRTTableItemCreator/KRTSubDataItemCreator/
  KRTImageGroupCreator (~0x4000 байт кода, рекурсия по ячейкам), Insert/RemoveSplitLine.
  Инфраструктура УЖЕ ЕСТЬ: KReportTemplateData.h (все 4 структуры, KSplitLineInfo совпал 1:1),
  report_template::*, KTextBlock/KImageBlock/KTableBlock/KTitleTableBlock, KMeaStringUtil.

РАЗВЕДКА БЕЗ РЕАЛИЗАЦИИ (три класса, результаты сохранены — НЕ тратить время повторно):

• **KTemplateEditDocument (REPORT, 38 методов) — МЁРТВЫЙ КОД, НЕ РЕАЛИЗОВЫВАТЬ.**
  Во всём X2000 НИ ОДНОГО вызывающего (ни ctor, ни метода). Это предыдущая итерация
  той же семантики (IS-A QTextDocument); живой аналог — **KDocumentGenerator**
  @0x53bf38…0x541dac (HAS-A + GetTextDocument(QTextEdit*), 12 внешних вызывающих из
  KNewTempletEditor/KReportTempletEditDlg). Гипотеза «это Qt::Widgets-редактор»
  ОПРОВЕРГНУТА: база QTextDocument (Qt5::Gui), Q_OBJECT нет, вызовов QWidget/QDialog/
  QTextEdit/QPainter — ноль. Файлов не открывает вообще, данные in-memory.
  ЦЕННЫЙ ПОБОЧНЫЙ РЕЗУЛЬТАТ — раскладки структур (годятся для KDocumentGenerator):
    sizeof(KReportTemplateDataNew)=0x78: {0x00 map<string,string>, 0x30
      list<KReportTemplateItem>, 0x48 map<string,KReportTemplateItemConfig>}
    sizeof(KReportTemplateItem)=0xf8: 7×std::string (0x00,0x20,0x40,0x60,0x80,0xa0,0xc0)
      + list<KReportTemplateItem> @0xe0 (вложенные дети)
    KReportTemplateItemConfig: bool@0x00, string@0x08, map<string,string>@0x28
    sizeof(KRTCreatorContext)=0x40
  СЕРИАЛИЗАЦИЯ модели в QTextDocument (пригодится для генератора): QObject-свойство
  "TableType" на QTextTable = "KTableBlock"/"KTitleTableBlock"; QTextFormat
  UserProperty(0x100000) = id элемента — ЯКОРЬ для поиска ячейки (FindCellInTable);
  текст 0x1010 BlockAlignment / 0x2001 FontPointSize / 0x2003 FontWeight /
  0x2004 FontItalic / 0x0821 ForegroundBrush; картинка 0x5000 ImageName /
  0x5010 Width / 0x5011 Height; таблица 0x4010 FrameBorderStyle /
  0x4101 ColumnWidthConstraints / 0x4102 CellSpacing / 0x0820 BackgroundBrush.
  Doc-level ключи map<string,string>: "BgColor", "FontColor", "CalcApp".
  Исходный путь реф.: dialog/patient/reporttemplate/documentcreater/KTemplateEditDocument.cpp
  ДЕФЕКТЫ ОРИГИНАЛА (не воспроизводить бездумно): dtor НЕ удаляет KRTCreatorContext*
  (+0xa8) → утечка 0x40 байт на документ; UpdateDeptInfo и ChangeCalcApps свёрнуты
  линкером через ICF в одну функцию — ОБЕ пишут ключ "CalcApp" (похоже на баг исходника).

• **KSelfTest (8 методов) — Qt::Widgets-диалог, write только 2.** UI строится в ctor без
  .ui-файла (QGridLayout+btn_OK+listWidget). startCheck @0x7150f8: append(tr("TR_INPTSTest:"))
  → checkProcessor → checkEndo → checkLight → append(tr("TR_ICompleted")) → removeDuplicates().
  checkProcessor @0x714d00 (OFF-DEVICE, реализуемо): пустой GetProcessorSN() → TR_IPSNNI;
  [AgeTest] IsAgeTest из data/protected/syspreset/testenv.ini (дефолт false) → TR_IPATestNE.
  ОТЛОЖЕНО: checkEndo (EEPROM: u16@eep+0x22, флаги u8@eep+0x00 — бит1 сброшен → TR_WBMemoryNE,
  (flags&3)==2 → TR_VACalibrationNE), checkLight (наработка лампы: обе > 29 → TR_CLATestNE).

• **KFunTest (9 методов) — Qt::Widgets-диалог на Ui_KFunTest::setupUi, write 0 целиком**,
  но внутри InitWidget/ClickStart лежит чистое off-device ядро:
    testcaselist.txt = RootPath()+"/tmp/testcaselist.txt" (ДВОЙНОЙ слэш — баг склейки
      оригинала, POSIX схлопывает; фактически /home/root/tmp/testcaselist.txt);
    формат: плоский текст, СТРОКА = имя файла кейса, без секций. Чтение: readAll →
      fromAscii → split('\n', KeepEmptyParts, CaseSensitive) → удалить элементы size()==0
      (т.е. пустые строки и хвостовой \n игнорируются; \r НЕ обрезается — нужен LF).
      Запись: каждый элемент + "\n", включая последнюю строку;
    библиотека кейсов: SystemPath()+"autotest/casefile" (в поставке ОТСУТСТВУЕТ), фильтр
      Dirs|Files|NoDotAndDotDot, берутся имена с ".xml", исключая уже попавшие в план;
    ClickPresureTest: SystemPath()+"autotest/pressuretest/" → AppPath()+"autotest/"
      (каталог pressuretest в поставке ПУСТ), SetAutoTestOpen(1, 0x800).
  ОТЛОЖЕНО: ClickStart (IPC KProcMsgManager::SendMsg(0x85000000,…,72,0x800), KAutoTestThread),
  всё виджетное. OpenImportRules @0x735b60 — ПУСТАЯ заглушка (4 байта, ret), реализовывать нечего.
  ЗАМЕЧАНИЕ ПО МЕТОДОЛОГИИ: вынести ядро в новый класс (напр. KFunTestCaseList) — значит
  ВЫДУМАТЬ имя. Предпочтительнее реализовать реф. KFunTest и разложить тело InitWidget/
  ClickStart на приватные хелперы с явной пометкой «наши, в реф. инлайн» (как сделано
  с namespace KFactoryOptionsState).

===================== ПОДГОТОВКА К /clear (СЛЕДУЮЩАЯ СЕССИЯ, ЧИТАЙ ЭТО) =====================
Пользователь просил: «продолжай по порядку брать функции из бинарника и писать их по методологии».
СИСТЕМАТИЧЕСКИЙ ПОДХОД (проверен в этой сессии, держись его):
1. Выбрать класс/область → `objdump -t update/root/X2000 | grep <Class>` → отсортировать методы
   по адресу (скрипт-демангл длино-префиксов в истории). Идти по адресам.
2. ДЛЯ КАЖДОГО метода: дизасм (`objdump -d --start/--stop`) — какие bl зовёт. КЛАССИФИКАЦИЯ:
   • off-device (читает конфиг/БД/чистая логика) → РЕАЛИЗОВАТЬ по методологии §1.
   • device (GetEndoScope/GetCamera/GetSensorType/EEPROM/GPIO/GStreamer/DCMTK-сеть/IsEndoReady/
     IsCameraReady/GetEndoModel/живой скоп) → ОТЛОЖИТЬ, записать формулу/адрес в §10, НЕ фантазировать.
   • неизвестная схема/GBK-блоб/непонятный источник → НЕ фантазировать, отложить с пометкой.
3. Тяжёлый дизасм — через СУБАГЕНТА (компактный контекст): промпт «декодируй N методов, верни
   спеку регистры/формулы/вызовы/вердикт write|avoid БЕЗ листингов». См. примеры промптов в истории.
4. Реализация: имена классов/методов 1:1 с реф. (вкл. опечатки как GetSupprotedCameraList),
   значения из файлов прошивки (не хардкод), поля-офсеты как в бинарнике.
5. Self-test-режим в `app/src/ui/preview_main.cpp` (trace для PL-регистров ИЛИ проверка состояния/
   парсинга). Собрать ui_preview (§4), прогнать НОВЫЙ режим + ВСЮ регрессию 39 режимов ДО коммита.
6. Обновить §6/§7/§10 + ROADMAP (счётчик классов, список self-test) + список режимов в §4 и §10.
7. Коммит+пуш (git main, Co-Authored-By Claude Fable 5). НЕ коммитить update/ и docs/ref/*.pdf.

ГОТОВЫЕ КАНДИДАТЫ ДЛЯ СЛЕДУЮЩЕЙ ИТЕРАЦИИ (по убыванию чистоты):
A. ✅ СДЕЛАНО (см. «ПОСЛЕДНЕЕ» выше): KEncStyle остаток off-device — все 13 video.ini-геттеров
   реализованы с реф.-именами и квирками, self-test `encstyle` расширен. Остаток по классу —
   device (GetEndoModel + no-arg twins + getIsDefaultMatch); GetEndoDisplayModel тоже реализован.
B. ✅ РАЗВЕДКА ПРОВЕДЕНА (субагент, эта сессия). Результат — очередь непрочитанных конфигов
   (в порядке ценности; coldlightCamera2aPara.ini уже ЗАКРЫТ, см. «ПОСЛЕДНЕЕ»):
   B1. `system/update/dbupdate.ini` (131 стр.) — ИСТОРИЯ СХЕМЫ БД: секция = номер версии
       ([6]/[7]/[10]…), ключ = имя таблицы → список колонок + `<table>_mkeyindex=N`; таблицы
       doctor/examlist/report/workorders/acceptedworklist/dicomcommand/performedprocedurestep/
       codesequence. ЧИТАТЕЛЬ В БИНАРНИКЕ НЕ НАЙДЕН (строки `dbupdate` нет ни в X2000, ни в
       X2000Monitor/Simulator/Video) → путь собирается в рантайме ЛИБО файл для внешнего
       апдейт-скрипта. ВНИМАНИЕ: без читателя НЕЛЬЗЯ взять реф.-имена класса/методов —
       перед реализацией найти читателя (иначе имена придётся выдумывать, что нарушает методологию).
   B2. ✅ СДЕЛАНО (см. «ПОСЛЕДНЕЕ»). Было: `system/printer/*.xml` — читатель НАЙДЕН:
       **KSysPrintData** (GetAllUrlDriverInfo/SaveUrlDriverInfo/FindDriverPath/
       SetDefaultPrinterInXml/GetPrinterDefaultStatus/AddPrinter/DelPrinter/UpdatePrintSettings),
       записи KPrintServiceInfo/KPrintSettings, UI-оболочка KSysPrinter. XML крошечный
       (<Root><Version/><Printer><Item name="DefaultImagePrinter"/></Printer><PrinterList/>).
       XML-персист + логика дефолтного принтера по типу (CancleDefaultStatusByType/
       ChangeDefaultPrinterStatus) — OFF-DEVICE; QueryAllPrinters (CUPS) — device.
       **ЛУЧШИЙ СЛЕДУЮЩИЙ КАНДИДАТ** (есть точные реф.-имена + чистая логика).
   B3. ✅ СДЕЛАНО (см. «ПОСЛЕДНЕЕ»). Было: `system/autotest/*/…` —
       читатель **KAutoTestThread** (run/KeyboardSimulation/PanelKeySimulation/
       ResetFileExecCount/AutotestLogCheck/GetSnapScreenPngFile). Формат INI (несмотря на .xml):
       [Confiure] (sic) env/num/time + [KeyN] code=<F1|Tab|LAMP|POWER|Script>/event/sleep/
       cmd=<под-скрипт.ini>/cnt — мини-язык скриптов с рекурсией. Парсер+модель скрипта
       OFF-DEVICE (инъекция клавиш — device); autotest-OSD.ini = 274 шага дерева OSD —
       ценный ground truth для UI/клавиш.
   B4. `platform/statistic.py` (36 КБ, ЧИТАЕМЫЙ Python3!) — парсер логов APPlog<YYYY>-<MM>.txt
       (формат строки `[07-01 12:06:32:789][APP][I]: …`), configparser+minidom. Лучшая
       документация формата лога и семантики statistic.ini (который мы уже парсим).
   B5. `presetdata/userpreset/dicom/dicom.dic` (5006 стр., словарь DCMTK OFFIS,
       `(gggg,eeee)\tVR\tKeyword\tVM\tVersion`) — даст смысл DCM_*-именам в наших
       WorklistFieldMap.xml/Mpps*DatasetFormat.xml. Утилитарно ценно, реверс-ценность низкая
       (это upstream-данные DCMTK, не код вендора).
   B6. ✅ СДЕЛАНО. Было: `presetdata/thirdpartylegalnotice/thirdPartyLegalNotices.txt` — читатель
       **KThirdPartyLegalNotices::ReadLegalNoticeText(QString&, const QString&)**; тривиально,
       но закрывает класс целиком (дешёвый «филлер» между крупными задачами).
   НЕ БРАТЬ: `HD-2000.dat` (энтропийный блоб, крипто-задача; можно потратить 5 минут на пробу
   DES-ключа "ZXYuio12" из [[KControlProc]] — и не более), `platform/dict_pinyin.dat`
   (проприетарный бинарный trie Google Pinyin, глубокая нора, нулевая отдача),
   `qss/black/*.qss` кроме style.qss + mr_icon.rcc (косметика, низкий приоритет).
C. КРУПНЫЙ заход KVideoProxy VideoParamChanged (ОТЛОЖЕН — нужно решение пользователя: моделировать
   Qt-сигнал VideoParamChanged на KVideoParam(QObject) vs прямые вызовы; см. историю — расхождение
   value-based реф. vs level-based наш Send*). Методы: SetColorEnhanceValue/SetImageEnhanceValueByType/
   ResetVideoParam (+ KVideoParam::InitVideoParam/SetColorEnhConfig/SetImageEnhConfig/SendSetImageEnhLevel,
   KVideoSet::SetColEnhValue/GetColorEnhValue). Коды событий: image-enh 0x204-6, color-enh 0x207-9, level 4.

ПРОБНЫЕ КАНДИДАТЫ, ОКАЗАВШИЕСЯ НЕЧИСТЫМИ (НЕ тратить время повторно):
• KEncStyle::getScopeList — enc-файлы genc/cenc/…enc.ini это GBK-блобы БЕЗ ini-структуры; источник неясен.
• KExamListRecordFileUpdate::UpdateRecordFileNumToDb — колонка-счётчик в tb_ExamList не подтверждена.
• KVideoProxy device-методы: SendZoomValue(GetZoomRatio), SetAWBValue(EEPROM temp*1024/1000),
  SetVideoCaptureArea/SetVideoArea (KVideoSet::GetVideoArea/EEPROM) — формулы сохранены в истории §10.
============================================================================================
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

**Полный список self-test-режимов** (в §4): plreg, filt, dicom, report, account, thesaurus,
userset, coldlight, version, project, statistic, sysstatus, quickinput, style, examcfg, exam,
filebackup, videoset, dsreal, dsdemo, videocal, update, templetcfg, reportdb, savefile, osdset, dbservice, dispparam, endoinfo, remoteswitch, dcmfmt, pattime, cornercut, scopecut, fxpt, language, unicodetext, encstyle, recfiles,
templetmodel, dimmer, printdata, legalnotice, autotest, exambiz, exportrec, imgproc, reportedit, videoplayer, smalllang, dimming, endoscope, camera, lcdproxy, factoryopt, docgen, monitor, rtcreator, videoipc.

**Остаток ROADMAP (Фазы E/F) — device-bound:** HW (KEndoScope/K3ADimming/KLcdProxy/принтер),
UI (131 Widgets-класс), DCMTK-сеть, GStreamer live-video, панель 8″ (§8 — нужно решение по подходу).
Требуют физического прибора или архитектурных решений пользователя.
