# Endo X-2600 — Аудит полноты и Roadmap реверса/реализации

> Сгенерировано сверкой всех классов референса `update/root/X2000` (SonoScape X-2600)
> с реализованным в `app/src/`. Методология — см. `docs/PROGRESS.md §1`.
> Обновлять при закрытии пунктов.

## 1. Итог аудита (факты из бинарника)

- **Классов K\* в референсе:** 491 (≈8360 методов в `.text`).
- **Реализовано классов (по имени):** 25 + namespace `KSystem` + `KTemplateCfg`(≈`KReportTemplateManager`).
- **Покрытие по именам классов:** ~13% методов. НО реализованные классы покрыты ЧАСТИЧНО
  (happy-path ядра обработки изображения), а не целиком:

  | Класс | Методов в референсе | Реализовано | Что есть |
  |---|---|---|---|
  | KPlControl | 76 | ~47 | все чистые PL-регистры (гамма/CCM/AWB/VIST/Denoise/LUT…) |
  | KVideoProxy | 121 | ~30 | init/capture/снимок/ApplyImageParams/AEC-AGC/RBC/Denoise |
  | KDccuParam | 95 | ~40 | AEC/AGC/AWB/RB/Gamma/Zoom (dccuparam.ini) |
  | KMainCtrlThread | 122 | ~5 | каркас Init-последовательности |
  | KSystemSet | 108 | ~10 | Common/Account настройки |
  | KVideoParam | 49 | ~25 | держатель видеопараметров |
  | AlgParaManager | — | много | все LUT-загрузчики (config-driven) |

- **Проверяемое off-device ядро (10 self-test-режимов, все PASS):**
  `plreg, filt, dicom, report, account, thesaurus, userset, coldlight, version, statistic`.

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
   Self-test `videoset`. ОСТАЛОСЬ: KUserOsdSet (OSD-меню), corner-cut (device-геометрия).
3. **KVideoCal** (40) — калибровка видео (масштаб/центр/геометрия из display-ini). Config-driven.
4. ✅ **KProjectSet** (34) — СДЕЛАНО. project.ini (ProjectName/ID/Theme/Series/модели/LangMode) +
   per-модель product.ini ([Function] ZOOM/CHB/RECORD, [Limit] уровни/Zoom, [Firmware] PAPP*).
   Флаги гейтят функции (CHB→SetChbStatus и т.д.). Self-test `project`.
5. ✅ ЧАСТИЧНО **KExamListConfigHandler** (31) — СДЕЛАНО. Синглглтон видимости колонок списка
   осмотров (IsShow Age/Applicant/EndoScope/ExamDate/… + self-define, экспорт), персист QSettings.
   Self-test `examcfg`. ОСТАЛОСЬ: KUserOsdSet (OSD-меню-конфиг).
6. ✅ ЧАСТИЧНО **KEncStyle/KStyleConfig** (47) — стиль/бренд-часть СДЕЛАНА (KStyleConfig:
   stylelist.ini→бренды SonoScapeCN/SonoScape/PyCkeun/SonoScapeHK, путь style/<series>/<brand>,
   IsStyleValid/GetCurrentStyle; self-test `style`). ОСТАЛОСЬ: модель эндоскопа/камеры,
   scope-para/round-octagon-cut defaults — device (нужен подключённый скоп).

### Фаза B — DB-слой и сущности (высокий приоритет, верифицируемо) ✅ можно сейчас
Паттерн уже отлажен (KEntityManage/KEntityDicom/KEntityReport).

1. ✅ ЧАСТИЧНО **KEntityExam / KExamListDBTableHandler** — СДЕЛАНО. Полный CRUD осмотров
   (tb_ExamList: ExamId/AccessionNumber/PatientId/ExamType/Date/Time/Status/RegNum/DrExamId/Dir),
   CreateEntity/UpdateEntity/DeleteSelf/GetEntityDetail(List)/GetLatestExamId/GetPageRecord
   (пагинация). Self-test `exam`. ОСТАЛОСЬ: KEntity{Service,Base} (сервис/база).
2. ✅ **KEntityQuickInput{Patient,Doctor,Applicant,ReportTitle}** — СДЕЛАНО. Один класс на 4
   таблицы tb_QuickInput* (value/Count/date); SaveData (UPSERT+инкремент частоты), GetAllEntity
   (ранжирование по частоте), GetEntity (префикс), DeleteSelf. Self-test `quickinput`.
3. ✅ ЧАСТИЧНО **KFileBackup** — СДЕЛАНО. Рекурсивное copyDirectoryFiles/removeDirWithContent/
   clearFolder, getFilesSize/getDiskFreeSpace, GetDeviceTypeFromTargetPath (internal/USB).
   Self-test `filebackup`. ОСТАЛОСЬ: KSaveFile/KExportRecord/KStorageDevice (форматы экспорта).

### Фаза C — Завершение REPORT и DICOM (средний, частично off-device) ✅/⚠️
1. **REPORT (off-device):** ✅ ЧАСТИЧНО — KRTDataSourceReal СДЕЛАН (БД пациент/осмотр/отчёт →
   KReportDataSource → KDocumentGenerator; self-test `dsreal`). ОСТАЛОСЬ: KRTDataSourceDemo,
   KReportDisplayParam, KReportDBTableHandler, KTemplateLibCfg, KSysReportTempletCfg — расширить
   движок. KTemplateEditDocument (46) — рендер в QTextDocument (частично off-device).
2. **DICOM (БД off-device, сеть device):** ✅ ЧАСТИЧНО — KEntityDicom расширен tb_DcmStudy/tb_DcmSeries
   (иерархия Study→Series→SOP + tb_DcmMpps (MPPS жизненный цикл) + tb_DcmCommit (Storage Commitment);
   self-test `dicom`). ОСТАЛОСЬ (сеть/device):
   KDicomServer/SCU (KStoreScu/KWorklistScu/KCommitScu/KMppsScu — DCMTK, device), генерация
   Secondary Capture .dcm из JPEG+данные (device-DCMTK).

### Фаза D — UPDATE-пайплайн (средний, логика off-device) ✅ можно сейчас
- ✅ **KVersionConfig** — СДЕЛАНО. Установленные версии из data/protected/version.ini
  (Version/<component>), GetVersion/SetVersion/GetCompleteVersion + IsComponentCompatible/
  IsCompatible (сверка с KUpdateConf). Self-test `version` (kernel/hmi совместимы, camera — нет).
- ОСТАЛОСЬ: KUpdateMng/KUpdateAction/KUpdatePrepare/KProgressTask/KFactoryOptions — распаковка/
  применение патча, реверс формата lcd_upd/образов (FAT), оркестрация апдейта.

### Фаза E — Device-HW (низкий приоритет, НУЖЕН ПРИБОР) ❌
Не верифицируется off-device; реализовать и отлаживать на устройстве/в sysroot.
1. **KEndoScope / KEndoScopeControl** (81/22) — эндоскоп: распознавание, CRC, тип/сенсор, EEPROM.
2. **KCamera** (42) — камера-хендлер (поверх KVideoProxy).
3. **K3ADimming** (44) — авто-экспозиция/усиление/ББ (дёргает KVideoProxy::SetAEC/AGCValue по
   статистике яркости; KDccuParam-параметры). Пересекается с ReadBrightnessHistogramValue.
4. **KLcdProxy** (106) — связь с панелью 8″ (Cortex-M). Требует РЕШЕНИЯ §6 (МК-прошивка vs 2-й Qt).
5. **KUsbDevice / KPrinterManager / KSysPrinter / KCupsPrinter / KHalPrinterAPI** — принтер/USB
   (через libhal: Hal_Add_Printer и т.д.).
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

- Сейчас: **25/491 классов** (частично), **10 self-test-режимов**, сквозное ядро обработки изображения.
- Цель Фаз A+B+D: +~40 классов CORE/DB/UPDATE (логика), покрытие методов → ~30–35%.
- Полнота (вкл. UI+HW) достижима только с прибором и реализацией всех 131 UI-классов.
