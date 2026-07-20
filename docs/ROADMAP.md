# Endo X-2600 — Аудит полноты и Roadmap реверса/реализации

> Сгенерировано сверкой всех классов референса `update/root/X2000` (SonoScape X-2600)
> с реализованным в `app/src/`. Методология — см. `docs/PROGRESS.md §1`.
> Обновлять при закрытии пунктов.

## 1. Итог аудита (факты из бинарника)

- **Классов K\* в референсе:** 491 (≈8360 методов в `.text`).
- **Реализовано классов (по имени):** ~56 (+languageConfig,+KLoadUnicodeText,+KEncStyle,+KExamListRecordFileUpdate) (эта сессия +12: KVideoCal, KUpdateManifest,
  KSysReportTempletCfg, KSysReportTempletModel, KSysReportTempletControl, KReportDBTableHandler-пагинация, KSaveFile, KUserOsdSet, KEntityService, KReportDisplayParam, KEndoInfoServerConfig, KRemoteSwitchConfig, KDicomDatasetFormat, KPatientTimeOperation) +
  namespace `KSystem`. Ранее +15:
  KSystemStatus, KProjectSet, KStyleConfig, KVersionConfig, KExamListConfigHandler, KEntityQuickInput,
  KEntityExam, KFileBackup, KVideoSet, KColdLightConfig, KUpdateConf, KStatisticConfig, KDccuParam,
  KRTDataSourceReal + расширения KEntityDicom (Study/Series/Mpps/Commit).
- **Покрытие по именам классов:** ~13% методов. НО реализованные классы покрыты ЧАСТИЧНО
  (happy-path ядра обработки изображения), а не целиком:

  | Класс | Методов в референсе | Реализовано | Что есть |
  |---|---|---|---|
  | KPlControl | 76 | ~47 | все чистые PL-регистры (гамма/CCM/AWB/VIST/Denoise/LUT…) |
  | KVideoProxy | 121 | ~57 | init/capture/снимок/AEC-AGC(keep-alive)/getAecValue/FreezeCalResolution/Demoire/Dehaze-HDR/SendRBCValue-SwitchCHb/тонкие Set*Level-Size-AwbCut/RBC/Denoise |
  | KDccuParam | 95 | ~40 | AEC/AGC/AWB/RB/Gamma/Zoom (dccuparam.ini) |
  | KMainCtrlThread | 122 | ~5 | каркас Init-последовательности |
  | KSystemSet | 108 | ~10 | Common/Account настройки |
  | KVideoParam | 49 | ~25 | держатель видеопараметров |
  | AlgParaManager | — | много | все LUT-загрузчики (config-driven) |

- **Проверяемое off-device ядро (33 self-test-режима, все PASS):** plreg, filt, dicom, report,
  account, thesaurus, userset, coldlight, version, project, statistic, sysstatus, quickinput, style,
  examcfg, exam, filebackup, videoset, dsreal, dsdemo, videocal, update, templetcfg, reportdb,
  savefile, osdset, dbservice, dispparam, endoinfo, remoteswitch, dcmfmt, pattime, language, unicodetext, encstyle, recfiles.

**Вывод:** реализован и протестирован сквозной **config→параметр→PL-регистр** тракт обработки
изображения + БД-слои + движок отчётов + вспомогательные конфиги. Остальное (87% методов) —
UI-виджеты, device-HW, полные оркестраторы — не реализовано.

## 2. Пробелы по доменам (466 нереализованных классов)

| Домен | Классов | Методов | Характер | Верифицируется off-device? |
|---|---|---|---|---|
| **UI** (Widgets) | 131 | 2831 | Qt5::Widgets: диалоги/списки/меню/редакторы | ⚠️ рендер (offscreen), но нужен UX-реверс |
| **MISC** | 115 | 1366 | смешанное (утилиты, контексты, хелперы) | частично |
| **CORE** | 59 | 903 | логика: статусы/калибровка/потоки/оркестрация | ✅ да |
| **HW** | 25 | 625 | LCD-панель/сенсор/камера/принтер/USB/3A-dimming | ❌ нужен прибор |
| **REPORT** | 49 | 485 | шаблоны/датасорсы/редактор отчётов | ✅ логика да, редактор — UI |
| **DICOM** | 45 | 426 | очередь/MPPS/commit/сервис-конфиг | ⚠️ БД да, сеть — DCMTK/device |
| **DB** | 28 | 344 | сущности/бэкап/экспорт/хранилища | ✅ да |
| **UPDATE** | 13 | 262 | апдейт-пайплайн/factory/версии | ✅ логика да |

## 3. Roadmap — фазы (по приоритету и верифицируемости)

Каждая фаза — по методологии §1: **реверс → найти конфиги → код с теми же именами → self-test**.

### Фаза A — CORE-логика off-device (высокий приоритет, верифицируемо) ✅ можно сейчас
Реализуемо и тестируемо на Mac без прибора. Наибольшая отдача.

1. ✅ **KSystemStatus** (50) — СДЕЛАНО. Синглтон состояния (QObject, GetInstance) с полями
   (freeze/record/video/light/iris/brightness/CHb/**VlsMode[0x3c]**/view/panel/network…) и
   сигналом SystemStatusChange(тип,значение); подавление дублей. Self-test `sysstatus`.
   ОСТАЛОСЬ: кросс-пропагация SetVlsMode→KVideoParam(RBCGroup)/AGC (оркестрация, при KVideoSet).
2. ✅ ЧАСТИЧНО **KVideoSet** (64) — СДЕЛАНО. Оркестрация Set*Level (ColEnh/ImgEnh/Denoise/
   BrightEQ/Contrast/ColorR-B-C/Zoom/OperationMode) → держатель KVideoParam + применение в PL
   через KPlControl; Get* из держателя; ResetVideoParam. Клей config→параметр→регистр.
   Self-test `videoset`. ✅ KUserOsdSet (OSD-меню) СДЕЛАН отдельно (см. ниже).
   ✅ ДОБАВЛЕНО: corner-cut геометрия — AlgParaManager::SetCutCornerPara/SetRoundPara/
   SetOctagonPara (круг/8-угол, 1080-LUT) + KPlControl::SetCornerCutWay (стрим 0xa18c8000),
   self-test `cornercut`. ✅ ДОБАВЛЕНО: KStyleConfig scope-info (video.ini: defaultRound/
   OctangleCut/shapeType/videoSize, hex-имена + [Default] фолбэк), self-test `scopecut`.
   ОСТАЛОСЬ (device): KVideoProxy::ReadCornerData/SetCornerCutWay — источник way/b/c с
   живого эндоскопа (EEPROM/GetEndoShapeType).
3. ✅ ЧАСТИЧНО **KVideoCal** (40) — СДЕЛАНО off-device ядро. В оригинале это Qt-диалог,
   но его прикладное ядро вынесено в non-UI класс KVideoCal: диапазоны смещения центра по
   типу прошивки сенсора (GetCenterOffsetHorizontalRange/VerticalRange — 1:1 с X2000, карта
   config2firmwareTypeMap: OV2740/OH01A/IMX274/OV6946/OCHFA→[-16,16]/[-10,10]/[-4,4]/[0,0]),
   режимы обрезки углов (CornerCutMode: octangle&round/octangle/round), SaveDisplayArea →
   KDisplayOption. Расширен KDisplayOption: getVideoRectForImgPro/getFreezeVideoRect +
   setVideoRectForImgPro/setVideoRectForUI ([VIDEO]/[UI]/IMAGE, [VIDEO]/IMAGE_PIP в display-ini
   IMG*-UI*.ini), GetDesktopViewConf (раскладка [KUIDesktop] soft/hard-endo). Self-test `videocal`
   (диапазоны + rect roundtrip). ОСТАЛОСЬ (device, Фаза E): UI-диалог/спинбоксы, запись центра/
   области захвата/формы углов в железо (KEndoScope/KCamera::SetVideoCentorPoint/SetVideoCapArea),
   дефолты corner-cut из KEncStyle::getScopeDefault{Round,Octangle}Cut (стиль/скоп).
4. ✅ **KProjectSet** (34) — СДЕЛАНО. project.ini (ProjectName/ID/Theme/Series/модели/LangMode) +
   per-модель product.ini ([Function] ZOOM/CHB/RECORD, [Limit] уровни/Zoom, [Firmware] PAPP*).
   Флаги гейтят функции (CHB→SetChbStatus и т.д.). Self-test `project`.
5. ✅ ЧАСТИЧНО **KExamListConfigHandler** (31) — СДЕЛАНО. Синглглтон видимости колонок списка
   осмотров (IsShow Age/Applicant/EndoScope/ExamDate/… + self-define, экспорт), персист QSettings.
   Self-test `examcfg`.
   ✅ ДОБАВЛЕНО: **KUserOsdSet** — OSD-конфиг эндоскопа (osd.ini, часть вне KUserSet):
   назначение функций на кнопки ручки (ButtomM/A/B × Short/LongPress → ID), FootSwitch,
   Operation/Iris Mode; фикс. список функций по ID (TR_Frz/Zm1/IRIS1/AGC1/IEnh/Snp/Brtnss±/
   Ctrst/WBalance/LMode/Rcd — 1:1 с GetFunctionNameList). Self-test `osdset`.
5b. ✅ **KConfig** (40) — СДЕЛАНО. INI-движок ядра (реф. architecture/src/kernel/ini/):
   LoadConfigFile/PreproccessConfigFile/ExtractConfigFile/EraseComment/TrimBlankSpace,
   FindValue/HasItem/ItemMustExist/GetKeysFromSection, ReadBool/Int/Long/Float/Double/String/
   Data(+WithoutDefaultValue), WriteData×6, Save. Семантика 1:1 с дизасмом и НЕСОВМЕСТИМА с
   QSettings (bool="True"/"False"; false лишь для FALSE/F/NO/N/0; комментарий только '#' и
   съедает '\n'; Save — лексикогр. порядок + пустая строка после каждой секции; дефолты не
   персистятся). Self-test `kconfig`. **База для всех *ConfigSetupHandler-фасадов.**
5c. ✅ **KPatientListConfigSetupHandler (28) + KWorkListConfigSetupHandler (25)** — СДЕЛАНО.
   Фасады над KConfig (`patientsetup.ini`/`worklistsetup.ini` в data/protected, секция
   [ShowOnMainUi]): видимость колонок списка пациентов (дефолт всех — true) + GetColumnIsShow
   (map «колонка→0/1», 7 записей); worklist — видимость (Is*On) и ЗНАЧЕНИЯ фильтра запроса
   (patientid/patientname/RegisterNumber/plantimestart|end/Equipment). Синглтон std::call_once
   → сырой указатель; std::string; опечатки реф. сохранены (IsShowPatietID/IsShowRegisterNumer).
   Добавлен KSystem::ProtectedPath. Self-test `listsetup`.
6. ✅ ЧАСТИЧНО (расширено) **KEncStyle/KStyleConfig** (47) — ДОБАВЛЕНЫ video.ini-геттеры
   per-скоп (13 методов, реф.-имена): ConvertSrc2Enc (hex, символ ≥U+0100 → "00"),
   getScopeInfoPath (КАТАЛОГ scope/), getScopeSize(QRect), GetEndoSensorType/GetFirmwareType/
   GetEndoType/GetEndoShapeType (**int-enum через config2*Map**), getScopeRotateType,
   getScopeDefault{Round,Octangle}Cut, GetScopeZoomRatio, getScopeParaDefault (POD из 4 ключей,
   workLength 16-бит), getBiopsyImg (сборка пути `<dir>/<hex>.png`), getScopeType (скан 8
   enc-файлов, cenc=1/genc=0/benc=2/nlc=3/denc=4/vetc=5/cysc=6/choc=7). Двухуровневый фолбэк
   [Default]; квирки сохранены (firmwareType 8/7-инверсия без 6; zoomRatio/shapeType БЕЗ
   [Default] с зашитыми дефолтами 1.0 / "OCTANGLE_AND_ROUND"). Self-test `encstyle` расширен.
   Также реализован GetEndoDisplayModel (гейт роль<=1 && бренд PyCkeun, таблица из 5).
   ОСТАЛОСЬ (device): GetEndoModel + no-arg twins + getIsDefaultMatch.
   Стиль/бренд-часть СДЕЛАНА РАНЕЕ (KStyleConfig:
   stylelist.ini→бренды SonoScapeCN/SonoScape/PyCkeun/SonoScapeHK, путь style/<series>/<brand>,
   IsStyleValid/GetCurrentStyle; self-test `style`). ОСТАЛОСЬ: модель эндоскопа/камеры,
   scope-para/round-octagon-cut defaults — device (нужен подключённый скоп).

### Фаза B — DB-слой и сущности (высокий приоритет, верифицируемо) ✅ можно сейчас
Паттерн уже отлажен (KEntityManage/KEntityDicom/KEntityReport).

0. ✅ ДОБАВЛЕНО **KEntityService** — сервис обслуживания БД (не запись-сущность):
   ApplyEnvironment (PRAGMA synchronous=NORMAL + journal_mode=delete + REINDEX, 1:1 с
   SetEnvironment), BackupDatabase (копия `<base>_<YYYYMMDD_HHMMSS>.bak`, реф. Recover),
   RecoverDatabase; основной файл БД — HD-2000.dat. Self-test `dbservice`. ОСТАЛОСЬ:
   HandleMsg/Subscribe (шина сообщений — device/оркестрация).
0b. ✅ ДОБАВЛЕНО **KReportDisplayParam** (Фаза C, валидность элементов отчёта):
   IsItemValid/AppendValidItem/SetRefValidItems/Reset + UpdateTemplateDisplayParam —
   проход дерева шаблона (ReportItem) × источник (KReportDataSource): элемент валиден
   для показа, если у него есть непустые данные или валидный потомок; ref-множество
   гейтит допустимые имена. Self-test `dispparam`.
1. ✅ ЧАСТИЧНО **KEntityExam / KExamListDBTableHandler** — СДЕЛАНО. Полный CRUD осмотров
   (tb_ExamList: ExamId/AccessionNumber/PatientId/ExamType/Date/Time/Status/RegNum/DrExamId/Dir),
   CreateEntity/UpdateEntity/DeleteSelf/GetEntityDetail(List)/GetLatestExamId/GetPageRecord
   (пагинация). Self-test `exam`. ОСТАЛОСЬ: KEntity{Service,Base} (сервис/база).
2. ✅ **KEntityQuickInput{Patient,Doctor,Applicant,ReportTitle}** — СДЕЛАНО. Один класс на 4
   таблицы tb_QuickInput* (value/Count/date); SaveData (UPSERT+инкремент частоты), GetAllEntity
   (ранжирование по частоте), GetEntity (префикс), DeleteSelf. Self-test `quickinput`.
3. ✅ ЧАСТИЧНО **KFileBackup** — СДЕЛАНО. Рекурсивное copyDirectoryFiles/removeDirWithContent/
   clearFolder, getFilesSize/getDiskFreeSpace, GetDeviceTypeFromTargetPath (internal/USB).
   Self-test `filebackup`.
   ✅ ДОБАВЛЕНО: **KSaveFile** (нумерация файлов) — FormatFlowNumber (3-значный zero-pad,
   overflow "999^" 1:1 с бинарником), FlowNumberFromName/MakeFileName (расширения jpg/mp4),
   FindMaxFileFlowNumber/NextFlowNumber (сканирование каталога → следующий номер). Self-test
   `savefile`. ОСТАЛОСЬ (device): KSaveFile::CreateFilePath (дата/серийник/examID под endodata/
   через KTime/KProductsSerial/KExamBussinessHandler), KExportRecord/KStorageDevice (экспорт на USB).

4. ✅ **KExamBussinessHandler** (21) — СДЕЛАНО (off-device). Оркестратор жизненного цикла
   осмотра — недостающий клей поверх KEntity*: временный→действующий ExamId
   (CreateTemporaryExamId/TakeEffectExamId поверх KExamNoGenerate::MakeExamId/SetExamId),
   IsCurrentExamIdExaming/IsExamEnd, EndoPowerOn/Off/Start/FinishSaveDataAction,
   синхронизация карточки пациента (главный экран ↔ БД ↔ отчёт), переименование каталога
   данных `<parent>/<examId>-<PatientName|Anonymity>`, GetSaveDataPath (срез USB-префикса).
   Вместе с ним: **KExamEntry (35 полей)** + **MainUiPatientInfo (10)** + **KExamListDBTableHandler**
   (Add/Get/UpdateExamEntity, статические, DDL-надмножество + ALTER TABLE для «тонкой»
   схемы KEntityExam); KPatientEntry дополнен полями реф. PlanDate/ExamType.
   Квирки реф. воспроизведены 1:1 (баг полярности GetGender, "INVALID_STRING", "2" в PatientSex,
   тег [I] у ошибки вставки и др. — см. PROGRESS §10). Self-test `exambiz`.
   ОСТАЛОСЬ (device): реальные KEndoScope/KCamera-геттеры модели/SN, DICOM-серии по сети,
   KThreadPoolMsg→настоящая шина UI; уточнить реверсом подпорки KTimeInfo/
   KPatientMngExamStatus/KExamDataFileNameGenerator/KUsbDevice/KSystemLog.

5. ✅ **KExportRecord** (19) — СДЕЛАНО (off-device). Экспорт данных осмотра на USB:
   построение путей `<usb>/Export/<S>/<S>/` (двойная подстановка id — квирк реф.),
   makeAllPath/cleanDir, needCopy, delExistFile (по имени), filesSize/freeSize/
   IsSpaceEnough (запас 1024 Б, строгое беззнаковое), ExportFiles (обе перегрузки) с
   флагом отмены, кодом -7 на отключение USB и исключением PicInfo.ini из счётчика.
   Плюс KFileBackup::copyFile(…, bool)->int по реф. сигнатуре и KSystem::ExportPath().
   Self-test `exportrec`. ОСТАЛОСЬ (device): KUsbDevice::SetUsbStatus + вызывающий
   KProgressTask::ExportFilesTask (UI-прогресс).

6. ✅ **KImageProcess** (14) — СДЕЛАНО (off-device целиком). Пустой POD, все методы
   статические. Rec.709 прямая/обратная, тональная кривая яркость/контраст (linear+
   quadratic, «пи» = литеральная 3.1416, квадратичная ветвь в double), HSL-насыщенность
   с насыщающим клампом, OptimizeReportImage (тюнинг 40/20/50 + sat 20), компоновка пар
   (_KGroupType/_KGroupImgSize/_KPoint), CreateThumbnail/ScaledWithAspectRatio,
   ResizeCopyImage (_LoadImgType 150x30/850x120/160x120, ручное вписывание, letterbox),
   GetVideoImg, FormatScreenShotImg, MergeVideoSmall/BigImage. Плюс статический
   KDisplayOption::GetThemeQssPath. Self-test `imgproc`.
   ОСТАЛОСЬ: смещения +0x30/+0x34 структуры GetSoftEndoViewConf (сигнатура у нас иная);
   развести владение миниатюрами видео с KSaveVideoFile.

7. ✅ ЧАСТИЧНО **KReportEditDataSource** (17) — СДЕЛАНО off-device ядро (12 методов) +
   слой таблицы **Report** (KReportEntity 15 колонок, KReportDBTableHandler 9 методов).
   Статeless-класс статических методов (не виджет, typeinfo нет). Сериализация списка
   снимков `$#a#b`, системные/пользовательские списки органов, пути региона и курсора,
   имена типов отчёта, LoadDbString/LoadDbInt, Query/Delete/GetOneRecordFromReportTB.
   Self-test `reportedit`.
   ⚠️ ВСКРЫТЫ 3 КОЛЛИЗИИ с существующим кодом (KEntityReport/tb_Report vs Report,
   KReportDataSource, enum ScopeClass) — НЕ разрешены, см. PROGRESS §10, нужна сверка.
   ✅ ДОЗАКРЫТО (5-я итерация): GetReportItem + InsertReportItem + report_edit::KReportItem
   (31 поле, sizeof 0xf0). Тогда же ИСПРАВЛЕНЫ две ошибки: в KReportEntity не хватало
   колонки Diagnose (ключ — immediate, а не литерал) и HPType стоял не на своём месте;
   InsertReportItem всё-таки ВЕТВИТСЯ на UpdateReportEntity при существующей записи.
   ОСТАЛОСЬ: GenerateExamImgsForPdf (cv::imread/resize/imwrite — нужен OpenCV;
   KImgTableItem при этом обычный QObject и препятствием не является).

8. ✅ **KVideoPlayerMng** (22) — СДЕЛАНО (20/22 off-device). Список воспроизведения и
   навигация по частям разбитой записи: AutoSetVideoFiles (сортировка ПО УБЫВАНИЮ ⇒
   индекс 0 — самая новая запись), ParseSplitVideo (<группа>_<индекс>),
   IsValidVideoFile, Find{Last,Next}Enable / Find{Last,Next}Video / FindNextSplitVideo,
   Get{Next,Pre}VideoFileFullPath, Switch*, CheckIsFirstOrLast, CheckIsNeedPlayNextVideo.
   Квирки реф. сохранены 1:1 (Latin-1 в ParseSplitVideo, затирание out при -1,
   QDir::Dirs в фильтре, слепой idx-1, первое/последнее совпадение в разных направлениях).
   Self-test `videoplayer`. ОСТАЛОСЬ (device/UI): PlayVideo → KVideoPlayerOSD + modetest,
   InitGst в конструкторе.

9. ✅ **KSmallLangTranslate** (22) — СДЕЛАНО (off-device целиком). Раскладки «малых
   языков» (Latin/Russian/French/Polish/Hungary): битовая машина модификаторов,
   многоуровневые клавиши (modIdx из 4 бит → уровень → aeOutKey), мёртвые клавиши
   с защёлкой префикса и переигрыванием трёх событий. Таблицы лежат В КОДЕ реф.,
   поэтому добавлен генератор `tools/gen_kbdlayout.py` (ELF → C++-заголовок,
   воспроизводимо). Квирки реф. сохранены 1:1 (GetBit отдаёт сырую маску, Init
   возвращает true на неизвестный язык, locked-модификатор это toggle, AltGr
   сбрасывает защёлку, ошибка трансляции проглатывается, три мёртвых параметра).
   Self-test `smalllang`.
   ✅ ДОЗАКРЫТО (8-я итерация): **KKey2Name** — авторитетная таблица «код ↔ имя»
   (614 записей) извлечена генератором `tools/gen_keysym.py`. Гипотеза про
   g_strKeysym_S40/S50 ОКАЗАЛАСЬ НЕВЕРНОЙ: эти массивы никто не читает, а
   GetNameOfKey пересобирает таблицу на стеке при каждом вызове. Имена
   модификаторов подтвердили независимый реверс 7-й итерации.
   ОСТАЛОСЬ: Qt-обёртка KSmallLangInputMethod — отдельная UI-задача.

10. ✅ **K3ADimming** (43) — СДЕЛАНО (40/43 off-device). ВНИМАНИЕ: ПОМЕТКА «device»
   В ФАЗЕ E БЫЛА НЕВЕРНА — яркость класс не вычисляет (приходит готовой в
   AUTO_DIMMING_PARA), а вся работа с железом это ОДНА запись WriteValueToPL
   в SetOV6946DimmingParam. Сделано: ПИД в приращениях с гейн-планированием, кламп
   интегратора, подавление пересветов (CalDelt), пересчёт дБ<->ток лампы (делитель
   17903.32), распределение бюджета света по трём полосам, экспозиция/усиление ->
   регистры по 5 типам сенсора, таблица ручного ALC. Квирки реф. сохранены 1:1
   (ПИД-история в файловых статиках типа float, запись обратно в out-параметр,
   мёртвый сеттер SetAlcMax, мёртвая ветка UpdataPid, ассиметрия скорости затемнения).
   Self-test `dimming`.
   ОСТАЛОСЬ (device): цепочка запуска KMainCtrlThread::Process3ADimming
   (KVideoProxy::ReadIrisAndRatio + KSystemStatus) — принадлежит другому классу.

### Фаза C — Завершение REPORT и DICOM (средний, частично off-device) ✅/⚠️
1. **REPORT (off-device):** ✅ ЧАСТИЧНО — KRTDataSourceReal+Demo СДЕЛАНЫ (БД пациент/осмотр/отчёт → KReportDataSource →
   KDocumentGenerator, self-test `dsreal`; демо+DemoImage-превью, self-test `dsdemo`).
   ✅ ДОБАВЛЕНО: **KSysReportTempletCfg** — каталог шаблонов с департаментами (парсит
   config/TempletInfo.xml → KTempletBaseInfo: name/modifydate/factory + <Dept name default/>;
   GetTemplateInfoByName/GetDeptsOfTemplate/GetTempletNamesByDept/GetDefaultTempletByDept).
   Также грузит **библиотеку шаблонов** config/TempletLibInfo.xml (та же схема, полные имена
   layout-файлов ReportTemplateNP-*; GetTempletLibInfos/GetLibTemplateInfoByName — покрывает
   KTemplateLibCfg-инфо). Self-test `templetcfg`.
   ✅ ДОБАВЛЕНО: **KReportDBTableHandler**-пагинация в KEntityReport (GetPageRecord/
   GetAllRecordMainKey/GetQueryRecordNum/QueryPageRecord — offset/limit + LIKE-фильтр по
   diagnosis/diseaseName). Self-test `reportdb`. ОСТАЛОСЬ: KReportDisplayParam (валидность
   элементов — тонкий set<string>), KTemplateLibCfg (пользовательский кэш/группы) —
   расширить движок. KTemplateEditDocument (46) — рендер в QTextDocument (частично off-device).
   ✅ ДОБАВЛЕНО: **KSysReportTempletModel (21/21) + KSysReportTempletControl (27/27)** —
   рабочая модель каталога (in-memory инфо + cfg-кэш + delList + буфер либ-элементов) и
   синглтон-контроллер с состоянием выбора (департамент/шаблон). Все реф.-квирки сохранены:
   GetDefalutTempletNameByDept возвращает АРГУМЕНТ dept (+побочно назначает NP-nx3 дефолтом),
   SetDefault(имя,деп,флаг) с безусловным clear-циклом, DeleteTemplate только при наличии в
   cfg-кэше, RenameTemplate без кэша — no-op (с кэшем: миграция ключа + старое имя в delList),
   DeleteSelectedTemplet пустой, GetCopyTempletInfo без уникализации имени и со сбросом
   per-dept default, GetSelectedDept сырой ("KW_ALL" не транслируется). STR_GENERAL_TEMPLATE
   = "General", сентинелы Model "REPORT_DEPT_ALL" / Control "KW_ALL". Self-test `templetmodel`.
   **Отчётная off-device-ветка на этом закрыта.**
2. **DICOM (БД off-device, сеть device):** ✅ ЧАСТИЧНО — KEntityDicom расширен tb_DcmStudy/tb_DcmSeries
   (иерархия Study→Series→SOP + tb_DcmMpps (MPPS жизненный цикл) + tb_DcmCommit (Storage Commitment);
   self-test `dicom`).
   ✅ ДОБАВЛЕНО: KDicomFieldMap расширен на **мульти-record** — Mpps{Create,Set}FieldMap.xml
   (несколько <Record type SubGroup DatasetPath>: PerformedProcedureStep/ProcedureCode/Series/…);
   Records()/RecordByType()/RecordCount() при сохранении плоского Fields() для WorklistFieldMap.
   Проверено в self-test `dicom` (MppsSetFieldMap: 5 записей, PPS 8 полей, ProcedureCode SubGroup).
   ✅ ДОБАВЛЕНО: **сервис-конфиг DICOM** (`link-dicom.json`) — 8 классов: KSysDICOMData
   (файловый слой: Load/Save/Save(conf)/GetConfigFileName + кэш m_jsonObj), KDICOMConf
   (Local + списки MPPS/WorkList/Storage/CommitStorage, Get*ConfList с фильтром IsEnable),
   KDICOMLocalConf (Port/ConnectTimeout/Name/AE/UploadPDFReport/SnapSyncUploadImage),
   KDICOMServiceBaseConf (IsEnable/Port/Name/IP/AE/AddTime + тип сервиса) и наследники
   KDICOM{Storage,WorkList,MPPS,CommitStorage}Conf. Ключи/дефолты/порядок — 1:1 с namespace
   LINK_DICOM в дизасме. Добавлены KSystem::SetDataPath/UserSetPath. Self-test `dcmconf`
   (дефолты + roundtrip + вложенный CommitStorage + фильтр IsEnable + схема документа).
   Питает struct DicomSetting (KDicomInterface) и очередь tb_DcmStore.
   ОСТАЛОСЬ (сеть/device): KDicomServer/SCU (KStoreScu/KWorklistScu/KCommitScu/KMppsScu — DCMTK,
   device), генерация Secondary Capture .dcm из JPEG+данные (device-DCMTK); UI-редактор
   сервисов (KDICOMServiceBaseConf::GetDICOMServiceConfInfoList — Widgets).

### Фаза D — UPDATE-пайплайн (средний, логика off-device) ✅ можно сейчас
- ✅ **KVersionConfig** — СДЕЛАНО. Установленные версии из data/protected/version.ini
  (Version/<component>), GetVersion/SetVersion/GetCompleteVersion + IsComponentCompatible/
  IsCompatible (сверка с KUpdateConf). Self-test `version` (kernel/hmi совместимы, camera — нет).
- ✅ ЧАСТИЧНО **KUpdateManifest** (off-device ядро апдейт-решения) — СДЕЛАНО. Канонический
  список обновляемых компонентов (1:1 с KUpdateAction::CheckUpdateItems: app/hmi/pap/
  papp00..07/papp80/lcd), чтение/запись манифеста update.ini (`[item]/IsNeedUpdate=TRUE|FALSE`,
  `[item]/Version`) в GetUpdateRoot()=DataPath()/update/update, логика решения DecideItem
  (реф. UpdateCheck: версия пакета vs установленная (KVersionConfig) + matched-совместимость
  (KUpdateConf) → UpToDate/NeedUpdate/Incompatible/NoPackage) и полный прогон CheckUpdateItems
  с записью флагов. Self-test `update`. Схема источников версий реверснута: app→svnno.ini,
  hmi→Pro, прочие→versiontree.ini (`item/UpdateFile0`); список — history/order.ini.
- ✅ ДОБАВЛЕНО: **MD5-верификация файлов пакета** в KUpdateManifest (реф.
  KUpdateConf::CalcFileMd5Code/ReadFileMd5Code/CheckUpdateFileMd5Code): CalcFileMd5
  (QCryptographicHash::Md5 — off-device эквивалент HAL getUpdateFileChecksum),
  ReadFileMd5 (парсинг md5sum-формата "<md5>  <имя>"), CheckFileMd5 (calc==read).
  Проверено в self-test `update` (calc + tamper detection).
- ✅ СДЕЛАНО `KFactoryOptions` (off-device ядро, self-test `factoryopt`): GetTestConfPath
  (8 комбинаций, сверено с поставкой system/autotest/aging-*), testenv.ini ×2, StartTest/
  StopTest/CopyConf, UpdateReleaseVersion, OpenDebug. Отложен блок восстановления
  (GetEndoScope/GetCamera/GetKHalClass) — см. PROGRESS §10.
- ОСТАЛОСЬ (device/UI, Фаза E/F): KUpdateMng/KUpdatePrepare/KProgressTask —
  распаковка rar (ExecRarCmd/StartDecompress), применение патча/прошивка (ExecUpdCmd),
  реверс формата lcd_upd/образов (FAT), Qt-оркестрация с прогресс-баром.

**ИСТОЧНИКИ РЕВЕРСА (дополнение):** кроме `update/root/X2000` есть **`update/root/X2000Simulator`**
— отдельный НЕ СТРИПНУТЫЙ бинарник, богатый символами (в нём, например, ВЕСЬ парсер скриптов
автотеста, которого в X2000 нет вовсе). Также X2000Monitor / X2000Video. При поиске «читателя»
конфига проверять ВСЕ бинарники, а не только X2000.

### Фаза E — Device-HW ⚠️ ПОМЕТКИ ПЕРЕСМОТРЕНЫ (аудит 2026-07-20)
**Аудит дизасмом показал: большинство пометок «нужен прибор» БЫЛИ НЕВЕРНЫ.**
Реальной работы с железом в Фазе E примерно на 20 методов, а не на ~600.
- ✅ **KEndoScope** (77) — СДЕЛАНО off-device: ввода-вывода нет вообще, байты EEPROM/CID
  приходят параметром, наружу — 12 сигналов. Карты серий извлечены
  `tools/gen_endoscope.py`. Self-test `endoscope`. Осталось: CheckEndoInfo/
  CheckEndoControl (нужен KControlProc), ClearEepromData (512 страниц × usleep).
- ✅ **KCamera** (38) — СДЕЛАНО off-device. Разбор CID/EEPROM, проверка модели по белому
  списку, карта серий (7 записей, `tools/gen_camera.py`). Отличия от KEndoScope:
  типы — жёсткие константы 8/2/2 (без KEncStyle), одна карта, порчи памяти нет.
  Квирки сохранены 1:1 (IsNeedRollover на деле «модель == 10-100-201», пропуск байтов
  0x0b/0x0c у синего гейна, saveRet == 1, центр всегда (1,7)). Self-test `camera`.
  Осталось: ClearCameraEepromData (512 эмиссий × usleep ≈ 15 с).
- ✅ ЧАСТИЧНО **KLcdProxy** (103) — СДЕЛАНО ядро off-device. Таблица диспетчеризации
  (82 записи × 24 Б) извлечена `tools/gen_lcdproxy.py`, поиск по паре (key, act),
  протокол `_KeyVlaue`, 13 продюсеров наборов, предикаты. Заглушка `KUiMsgProxy`
  добавлена (все её члены в реф. — сигналы). Попутно ИСПРАВЛЕН порядок enum'ов
  KUserOsdSet::Button/Press под референс (A,B,M / Long,Short) и добавлены сигналы
  UserSetChange/SystemSetChange в KSystemStatus. Self-test `lcdproxy`.
  ОСТАЛОСЬ: коды сообщений ~40 однострочных Switch*/Remote*-обработчиков (не
  установлены реверсом) и ключи GetKeyStatus, для которых нет источников —
  список методов в PROGRESS §10.
- ➡️ **KEndoScopeControl / KProcessorControl / KSysPrinter** — это НЕ device, а Qt-диалоги:
  переносятся в Фазу F (UI).
- ❌ **ДЕЙСТВИТЕЛЬНО device (пометка верна):** KHalPrinterAPI (12 сквозных Hal_*@plt),
  KComDataReceiveThread (read@plt — источник байтов MCU), регистровый API KPlControl.

### Фаза E (историческая формулировка) — Device-HW (низкий приоритет, НУЖЕН ПРИБОР) ❌
Не верифицируется off-device; реализовать и отлаживать на устройстве/в sysroot.
1. **KEndoScope / KEndoScopeControl** (81/22) — эндоскоп: распознавание, CRC, тип/сенсор, EEPROM.
2. **KCamera** (42) — камера-хендлер (поверх KVideoProxy).
3. **K3ADimming** (44) — авто-экспозиция/усиление/ББ (дёргает KVideoProxy::SetAEC/AGCValue по
   статистике яркости; KDccuParam-параметры). Пересекается с ReadBrightnessHistogramValue.
4. **KLcdProxy** (106) — связь с панелью 8″ (Cortex-M). Требует РЕШЕНИЯ §6 (МК-прошивка vs 2-й Qt).
5. **KUsbDevice / KPrinterManager / KSysPrinter / KCupsPrinter / KHalPrinterAPI** — принтер/USB
   (через libhal: Hal_Add_Printer и т.д.). ✅ ЧАСТИЧНО СНЯТО: **KSysPrintData (15/15) СДЕЛАН
   off-device** — весь класс оказался чистым XML-персистом (0 вызовов CUPS/HAL в дизасме):
   кэш принтеров + дефолты по типу сервиса + карта url→driver. Квирки сохранены (AddPrinter
   без дедупа, безусловные save, FindDriverPath по значению, «мёртвый» легаси-блок <Item>
   не читается вовсе). Self-test `printdata`. Остаток домена (CUPS/HAL-ввод-вывод) — device.
6. **KControlProc / KProcessorControl / KComDataReceiveThread** — протокол связи с МК-платой.
7. **Остаток register-API KPlControl** (~25): геометрия маски (round/octagon), FPGA-I2C
   (SetVideoCentorPoint), SetEndoIrisType, Aurora-serdes, PLInit — device.

### Фаза F — UI (Qt5::Widgets, большой объём) ⚠️
131 класс. Реверс раскладки/поведения из style.qss + ресурсов прошивки. Рендер проверяем
offscreen (как KUIDesktop). Порядок по важности:
1. **Пациент/осмотр:** KPatientListViewUi, KExamListViewUi, KPatientManagmentUi, KExamDetailInfoUi,
   KPatientList{Add,Edit,Search}Dlg.
2. **Отчёты:** KReportEditUi (69), KReportEditAddMarkView, KTempletTreeWidget, KReportPreviewDlg.
3. **Настройки:** KSystemSetDlg, KGeneralSetDlg, KDICOMServiceEditDlg, KScopeInfoEdit, KCameraInfoEdit.
4. **Общие виджеты:** KMessageBox, KDialog, KProgressDlg, KCalendarWidget, KMemComboBox,
   KOptionListButton, KPinyinWidget (ввод), KOsdSubMenu/KOsdMenuBase (экранное меню).

## 4. Рекомендуемый порядок

**Сейчас (off-device, максимальная отдача):** Фаза A → B → D → (off-device части C).
Это доводит ЛОГИЧЕСКОЕ ядро (статусы, БД, файлы, апдейт, датасорсы отчётов) до полноты,
всё проверяемо self-test'ами.

**Затем (нужен прибор/решения):** Фаза E (HW, панель 8″ — после решения §6) и device-части C.

**Параллельно/по мере надобности:** Фаза F (UI) — большой, но рендер-верифицируемый объём.

## 5. Метрика прогресса

Замеры — `python3 tools/coverage.py > docs/COVERAGE.md` (обновлять раз в несколько итераций).

- Сейчас: **107/486 классов затронуто (22%)**, из них доведено до self-test ~58;
  **1350/7008 методов (19.3%)**; **58 self-test-режимов** (регрессия `tools/selftest.sh` — 91 проверка).
- По верифицируемости: off-device ✅ — **826/1967 (42.0%)**; частично ⚠️ — 215/1268 (17.0%);
  device-bound ❌ — 309/3773 (8.2%).
- ⚠️ Цифры до 2026-07-20 были ЗАНИЖЕНЫ по абсолюту: парсер манглинга не понимал `_ZNK`
  (const-методы) и терял 577 методов бинарника, из них 104 наших. Соотношение почти не
  изменилось (19.4% → 19.3%), но знаменатель вырос с 6431 до 7008. Исправлено в coverage.py.
- **Ключевой вывод:** ~54% всего объёма (3476 методов) — device/UI-bound и недостижимо без прибора.
  Реальная метрика прогресса — доля ✅-корзины: **~40% сделано, ~1100 методов осталось**.
- Цель Фаз A+B+D: закрыть остаток ✅ (REPORT `KTemplateEditDocument`/`KRT*`, UPDATE
  `KFactoryOptions`, DB `KEntity*`, DICOM) → покрытие методов ~45–50% от полного паритета.
- Полнота (вкл. UI+HW) достижима только с прибором и реализацией всех 167 UI-классов.

---

## 6. Роадмап разведки: что ещё можно вскрыть и написать

Составлен 2026-07-20 по факту обхода поставки. Приоритет = отдача / трудозатраты.
Правило, оплаченное ошибками (см. PROGRESS §10): **сначала разведка, потом код**;
эвристика ✅ в §4 COVERAGE не знает ни про мёртвый код, ни про Qt::Widgets, ни про то,
вызывается ли класс вообще.

### A. НЕРАЗВЕДАННЫЕ БИНАРНИКИ — главный неиспользованный ресурс

Весь реверс до сих пор шёл ТОЛЬКО по `update/root/X2000` (13 МБ). Рядом лежат ещё три
маленьких исполняемых файла, не тронутых ни разу. Они на порядок меньше, а значит
разбираются ЦЕЛИКОМ — это возможность закрыть законченные программы, а не куски.

| Бинарник | Размер | Содержимое | Оценка |
|---|---:|---|---|
| **X2000Monitor** ✅ **СДЕЛАН** | 28 КБ | Разобран целиком, ядро реализовано (`app/src/monitor/X2000Monitor.{h,cpp}`, self-test `monitor`). Было: 31 функция, C-стиль, имена читаемые: `runStateMachine`, `InstallHeartBeatSig`, `find_pid_by_name`, `restartAPP`, `mainCtrlMonitor`, `videoMonitor`, `RecHeartBeatAct`, `UpdateHeartBeatTime`, `RecExceptionAct`, `RecSigChildAct`, `sendAppSignal`, `run_poweroff`, `initAppProc`, `restoreVideoMain`, `restoreExitApp`, `mySystem`, `mySleep`, `get_time`, `monitor_print_log` | **A1. ВЫСШИЙ ПРИОРИТЕТ.** Сторожевой процесс: heartbeat, машина состояний, перезапуск упавших процессов. Реально закрыть на 100% — целый бинарник за одну-две итерации. Ценность: восстанавливает модель надёжности прибора (что перезапускается, по какому таймауту, каким сигналом). |
| **X2000Video** ✅ ЧАСТИЧНО (IPC+shm) | 142 КБ | 7 классов / ~54 метода: `KEncodermanager` (20), `KMessageManager` (14), `KPipleline` (11), `KSemaphoreManger` (3), `KSharedBuffer` (3), `KSaveImage` (2), `KCommData` (1); плюс `ImageSaveBuf` (SetImageBpp/Width/Height, `copyGstBufferToShareBuffer`, `getImgShMemory`, `getRecordShMemory`) | **A2.** Отдельный процесс видео-тракта. GStreamer-часть — device, НО **протокол IPC** (`KMessageManager::HandleMessage/SendMessage`, `EndoConnectCmdHdl`, `ImageSaveStartCmdHdl`) и **раскладка разделяемой памяти** (`KSharedBuffer::GetReadBuffer/GetWriteBuffer`, семафоры) — off-device и проверяемы. Закрывает белое пятно: как наше приложение общается с видео-процессом. |
| **X2000Simulator** | 131 КБ | Проигрыватель автотест-кейсов. `/dev/input/event`, `XSendEvent`, `INIFileCaseExec`, `OneKeyExec`, `SendOneKey`, `python caseformat.py` | **A3.** Частично уже использован (из него взят KAutoTestScript). Остаток: формат `casefile`, `/home/root/tmp/playcasename.txt`, `/home/root/tmp/casename.txt`, точная таблица кодов клавиш. ПОДТВЕРДИЛ пути из разведки KFunTest (`/home/root/tmp/testcaselist.txt`, `system/autotest/casefile`) — независимое доказательство. |

### B. ЖИВЫЕ КЛАССЫ X2000 — продолжение текущей линии

| # | Цель | Методов | Почему |
|---|---|---:|---|
| B1 | `KRTCreatorContext` + `KRTAbsItemCreator` + `KRTTextItemCreator` | 10 + ~4 + ~4 | Итерация 2 к уже начатому `KDocumentGenerator`. Разблокирует `InitDocument`/`GetTextDocument` и всю документную половину. Прочие 4 творца можно НЕ делать — 17 из 23 типов всё равно уходят в TEXT_BLOCK-фолбэк. |
| B2 | `KUpdateAction` (17), `KUpdatePrepare` (13) | 30 | UPDATE — слабейший домен после UI (17.1%). Есть **читаемый ground truth**: `system/update/update_start` и `auto_update` — обычные shell-скрипты, по ним сверяется весь пайплайн. |
| B3 | `KDataOprEventDeal` (16), `KEntityBase` (14), `KDataFileOpr` (12), `KImportRules` (10), `KRecordItem` (10) | 62 | DB-слой, домен 38%. Все «не начаты», коллизий имён нет. |
| B4 | `KRTTeCreatorContext` (16), `KRTAbsDataSource` (11) | 27 | REPORT, продолжение линии B1. |
| B5 | `KTimeMng` (10), `KFunTest` (10) | 20 | CORE. По `KFunTest` разведка уже сделана (см. PROGRESS §10) — можно брать сразу. |

### C. ЛОВУШКИ — разгрести до того, как наступишь

| # | Что | Статус |
|---|---|---|
| C1 | 5 классов с ВЫДУМАННЫМ API (0% совпадения): `KEntityManage`, `KEntityService`, `KRTDataSourceReal`, `KEntityReport`, `KRTDataSourceDemo` | Вынесено отдельной задачей. Шестой (`KDocumentGenerator`) уже разведён — имя освобождено, класс написан. |
| C2 | `enum StatusType` пронумерован не как в бинарнике; `SetPanelType`/`SetWindowID`/`SetAutoTestStatus` у нас шлют сигнал, а реф. — нет | Вынесено отдельной задачей. |
| C3 | `STR_INVALID_ITEM_ID` не восстановлена — блокирует `MoveFront`/`MoveBack`/`ClickSubItem` в `KDocumentGenerator` | ✅ РЕШЕНО 2026-07-21 (= "Invalid ID", декомпилятор). Все три метода реализованы. |
| C4 | **`KEntityBase` (B3) — ЛОВУШКА:** это ЧИСТЫЙ АБСТРАКТНЫЙ базовый класс, все 14 «методов» — заглушки (`return ERR_NOT_SUPPORT/0/-1/""`, слиты линкером ICF в общие адреса 0x42d1d8/e8/f8). sizeof 0x50: +0x08 `IDatabase& m_db`, +0x10 `std::string m_type` (=ctor-арг, отдаётся `GetType()` ссылкой), +0x30 `std::string m_dataPath` (пустой). Реальный CRUD — в ПОДКЛАССАХ (реф. KEntityExam/Patient/… override с KDbSqlite). | НЕ реализовывать вслепую: (1) интерфейс `IDatabase` НЕ реверсирован (KEntityBase holds ref, но сам не зовёт — набор pure-virtual неизвестен); (2) `ERR_NOT_SUPPORT` — рантайм-.bss (@0xa60a68), статикой значение не берётся; (3) реф.-подклассы КОЛЛИЗИРУЮТ с нашими существующими `KEntity*` (QSqlQuery-API). Брать только вместе с реверсом IDatabase + конкретного подкласса, решив коллизию (как с KDocumentGenerator). |

### D. ДАННЫЕ И КОНФИГИ, не прочитанные

| # | Файл | Объём | Ценность |
|---|---|---|---|
| D1 | `system/update/dbupdate.ini` | 12 КБ | **История схемы БД**: секция = версия, ключ = таблица → колонки. ⚠️ ЧИТАТЕЛЬ ДО СИХ ПОР НЕ НАЙДЕН — нет ни в X2000, ни в трёх малых бинарниках, ни в `update_start`/`auto_update`. Без читателя нельзя взять реф.-имена. Проверить: не читается ли внешним скриптом с USB-пакета. |
| D2 | `system/platform/statistic.py` | 36 КБ | **Читаемый Python3** — лучшая документация формата APPlog и семантики `statistic.ini` (который мы уже парсим). Даром достаётся. |
| D3 | `system/videoconf/IRIS/*.txt` | 27 файлов | Таблицы настройки диафрагмы по сенсорам (`OH01A_EB_768X928`, `OV2740_EC_1280X960` и др.). У нас НЕ читаются вообще. Привязаны к типу сенсора → частично device, но сам парсинг off-device. |
| D4 | `presetdata/userpreset/dicom/dicom.dic` | 5006 строк | Словарь DCMTK. Утилитарно полезен (расшифрует DCM_*-имена в наших XML), но это upstream-данные OFFIS, не код вендора — реверс-ценность низкая. |
| D5 | `system/update/update_start`, `auto_update` | 2.4 + 1.8 КБ | Shell-скрипты пайплайна обновления. **Читаемый исходник** — использовать как эталон при B2. |

**НЕ БРАТЬ** (проверено, отдача нулевая): `HD-2000.dat` (энтропийный блоб), `platform/dict_pinyin.dat`
(проприетарный trie Google Pinyin), `qss/black/*` кроме style.qss.

### E. РЕКОМЕНДУЕМЫЙ ПОРЯДОК

1. ~~**A1 X2000Monitor**~~ ✅ СДЕЛАНО 2026-07-20: бинарник закрыт целиком, ядро
   реализовано, self-test `monitor`, найдены две аномалии прошивки (см. PROGRESS §10).
2. ~~**B1**~~ ✅ СДЕЛАНО 2026-07-21: `KDocumentGenerator` ЗАКРЫТ целиком (~29/33) вместе со
   всей render-подсистемой (KScreenMng, творцы text/image/table, InitDocument/GetTextDocument,
   FindFrameOrCell, выделение, UI-обёртки Add/Delete/Update/Click/Move/ChangeLayout, правка
   цвета/шрифта). Валидировано на реальном ReportTemplateNP-1x4 (self-test `initdocreal`).
   C3 (STR_INVALID_ITEM_ID) решён. Остаток — косметика (InsertBlockLineAfterItem футер-падинг,
   UpdateBlock, редкие творцы ImageGroup/SubData — в поставке нет). Детали — PROGRESS §10.
   ТЕКУЩАЯ ЛИНИЯ: **B3 DB-слой** — начат `KEntityBase` (генерик-CRUD поверх готового KDbSqlite).
3. ~~**A2 X2000Video**~~ ✅ IPC+shm СДЕЛАНО 2026-07-20 (self-test `videoipc`); остаётся GStreamer-часть (device).
4. **B2** (со сверкой по D5) и **D2** — дешёвые, с готовым ground truth.
5. **B3/B4/B5** — планомерный добор off-device-корзины.
6. **C1/C2** — когда мешают, а не «когда-нибудь».
