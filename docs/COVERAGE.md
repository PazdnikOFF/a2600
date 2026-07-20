# Карта покрытия реверса X-2600

> Сгенерировано: `python3 tools/coverage.py > docs/COVERAGE.md`
> Источники: `nm update/root/X2000` (свой парсер Itanium-мангла — `c++filt`
> на этом бинарнике не работает), `objdump -d` + строки `.rodata`/`.data*`
> для привязки конфигов, `app/src/**/*.h` — наша сторона (сверка по именам; см. §5.1).
> Домены и off-device-признак — эвристики имён, согласованы с `docs/ROADMAP.md §2`.

## 1. Сводка

| Метрика | Значение |
|---|---|
| Пользовательских классов в референсе | **486** |
| Методов в них (уникальные имена) | **7008** |
| Классов затронуто у нас | **113** (23%) |
| Методов реализовано (совпало по имени) | **1379** (19.7%) |

## 2. По доменам

| Домен | Классов | Методов | Реализовано методов | % | Классов затронуто |
|---|---:|---:|---:|---:|---:|
| **UI** | 167 | 2665 | 100 | 3.8% | 10 |
| **CORE** | 60 | 1251 | 493 | 39.4% | 28 |
| **REPORT** | 73 | 829 | 216 | 26.1% | 23 |
| **MISC** | 27 | 669 | 148 | 22.1% | 13 |
| **HW** | 40 | 625 | 194 | 31.0% | 7 |
| **DICOM** | 60 | 455 | 63 | 13.8% | 10 |
| **DB** | 47 | 368 | 140 | 38.0% | 16 |
| **UPDATE** | 12 | 146 | 25 | 17.1% | 6 |

## 3. Верифицируемость off-device

| Признак | Классов | Методов |
|---|---:|---:|
| ✅ | 173 | 1967 |
| ⚠️ | 73 | 1268 |
| ❌ | 240 | 3773 |

## 4. ТОП-40 нереализованных off-device-классов (✅)

Кандидаты на следующие итерации: чистая логика/конфиги, тестируются self-test'ом на Mac.

| Класс | Домен | Методов | Конфиги | Заметка |
|---|---|---:|---|---|
| `KTemplateEditDocument` | REPORT | 38 | — | ❌ МЁРТВЫЙ КОД — 0 вызывающих; живой аналог KDocumentGenerator (12 вызывающих). См. PROGRESS §10 |
| `KUpdateAction` | UPDATE | 17 | — | не начат |
| `KDataOprEventDeal` | DB | 16 | — | не начат |
| `KRTTeCreatorContext` | REPORT | 16 | — | не начат |
| `KEntityManage` | DB | 15 | — | есть заголовок, но свой API — сверить имена |
| `KEntityBase` | DB | 14 | — | не начат |
| `KUpdatePrepare` | UPDATE | 13 | — | не начат |
| `KDataFileOpr` | DB | 12 | — | не начат |
| `KEntityService` | DB | 11 | — | есть заголовок, но свой API — сверить имена |
| `KRTAbsDataSource` | REPORT | 11 | — | не начат |
| `KRTDataSourceReal` | REPORT | 11 | — | есть заголовок, но свой API — сверить имена |
| `KRecordItem` | DB | 10 | — | не начат |
| `KImportRules` | DB | 10 | — | не начат |
| `KFunTest` | CORE | 10 | `testcaselist.txt` | не начат |
| `KTimeMng` | CORE | 10 | — | не начат |
| `KSelfTest` | CORE | 10 | `testenv.ini` | не начат |
| `KRptTempletNodeInfo` | REPORT | 9 | — | не начат |
| `KVersion` | UPDATE | 9 | — | не начат |
| `KEntityReport` | REPORT | 8 | — | есть заголовок, но свой API — сверить имена |
| `KRTDataSourceDemo` | REPORT | 8 | — | есть заголовок, но свой API — сверить имена |
| `KEntityQuickInputDoctor` | DB | 8 | — | не начат |
| `KEntityQuickInputPatient` | DB | 8 | — | не начат |
| `KEntityQuickInputApplicant` | DB | 8 | — | не начат |
| `KQuickInputDoctorDbTableHandler` | DB | 8 | — | не начат |
| `KQuickInputPatientDbTableHandler` | DB | 8 | — | не начат |
| `KQuickInputApplicantDbTableHandler` | DB | 8 | — | не начат |
| `KUpdateMng` | UPDATE | 7 | — | не начат |
| `KNetWorkSet` | MISC | 7 | `system.ini` | не начат |
| `KHalUpdateClass` | UPDATE | 7 | — | не начат |
| `KThreadMessageQueue` | CORE | 7 | — | не начат |
| `KReportTemplateConfig` | REPORT | 7 | `ReportTemplateConfig.xml` | не начат |
| `KPicMng` | CORE | 7 | — | не начат |
| `KExamData` | DB | 7 | — | не начат |
| `KVideoPlay` | CORE | 6 | — | не начат |
| `KRecordCase` | DB | 6 | `casename.txt`, `testenv.ini` | не начат |
| `KReportEditUIConfig` | REPORT | 6 | — | не начат |
| `KPdf2Pics` | REPORT | 6 | — | не начат |
| `KEntityDEC` | DB | 5 | — | не начат |
| `KForceLogout` | CORE | 5 | — | не начат |
| `KProcMessage` | CORE | 5 | — | не начат |

## 5. Частично реализованные (есть методы, но не все)

| Класс | Домен | Методов | Наших | Покрытие |
|---|---|---:|---:|---:|
| `KMainCtrlThread` | CORE | 113 | 12 | 11% |
| `KSystemSet` | CORE | 101 | 15 | 15% |
| `KViewSoftEndo` | UI | 89 | 4 | 4% |
| `KVideoProxy` | CORE | 116 | 50 | 43% |
| `AlgParaManager` | MISC | 63 | 10 | 16% |
| `KUiMsgProxy` | UI | 55 | 8 | 15% |
| `KImgList` | UI | 50 | 8 | 16% |
| `KVideoSet` | CORE | 58 | 18 | 31% |
| `KDccuParam` | CORE | 93 | 54 | 58% |
| `KFactoryOptions` | UPDATE | 46 | 8 | 17% |
| `KEndoScope` | HW | 79 | 42 | 53% |
| `KSystem` | CORE | 51 | 17 | 33% |
| `KVideoCal` | UI | 35 | 3 | 9% |
| `KUpdateConf` | UPDATE | 29 | 1 | 3% |
| `KVideoParam` | CORE | 44 | 17 | 39% |
| `KUserOsdSet` | CORE | 34 | 8 | 24% |
| `KUserSet` | CORE | 27 | 3 | 11% |
| `KUsbDevice` | HW | 25 | 2 | 8% |
| `KEncStyle` | CORE | 39 | 18 | 46% |
| `KProjectSet` | CORE | 31 | 14 | 45% |
| `KDocumentGenerator` | REPORT | 33 | 16 | 48% |
| `KSaveFile` | DB | 20 | 3 | 15% |
| `KDicomInterface` | DICOM | 27 | 11 | 41% |
| `KSystemStatus` | CORE | 47 | 34 | 72% |
| `KThesaurusOpt` | REPORT | 15 | 2 | 13% |

### 5.1 Оговорка о точности метрики

Счётчик «реализовано» = **совпадение имени метода** с референсом, т.е. это **нижняя
оценка**. Там, где мы сознательно переосмыслили API (свои имена вместо оригинальных),
метрика показывает 0 при фактически рабочем коде:

- **Свой API при наличии класса-референса (7):** `KEntityDicom`, `KEntityManage`, `KEntityReport`, `KEntityService`, `KExamEntry`, `KRTDataSourceDemo`, `KRTDataSourceReal`

- **Наши абстракции без класса-референса (25):** `Application`, `K3ADimming`, `KAutoTestScript`, `KDicomDatasetFormat`, `KDicomFieldMap`, `KEndoInfoServerConfig`, `KEntityQuickInput`, `KFactoryOptionsState`, `KRemoteSwitchConfig`, `KReportDataSource`, `KReportHtmlGenerator`, `KScopeClass`, `KSoftEndoParam`, `KStatisticConfig`, `KStyleConfig`, `KSystemLog`, `KUpdateManifest`, `ReportItem`, `_LcdActItem`, `hal`, `keyname`, `theme`, `x2000monitor`, `x2000video`, `yxyDES2`

Такие классы стоит либо привести к именам оригинала, либо исключить из метрики руками.

## 6. Device-bound (❌) — крупнейшие

Требуют прибора/живого экрана; off-device проверяются только косвенно.

| Класс | Домен | Методов | Причина |
|---|---|---:|---|
| `KLcdProxy` | HW | 101 | прибор: LCD/сенсор/камера/USB/принтер/MCU |
| `KViewSoftEndo` | UI | 89 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KEndoScope` | HW | 79 | прибор: LCD/сенсор/камера/USB/принтер/MCU |
| `KReportEditUi` | REPORT | 64 | редактор отчётов — Qt-виджеты |
| `KExamListViewUi` | UI | 60 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KViewHardEndo` | UI | 59 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KCalendarWidget` | UI | 58 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KUiMsgProxy` | UI | 55 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KTempletTreeWidget` | REPORT | 53 | редактор отчётов — Qt-виджеты |
| `KReportEditAddMarkView` | REPORT | 53 | редактор отчётов — Qt-виджеты |
| `KPatientListViewUi` | UI | 52 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KImgList` | UI | 50 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KAlgParamAjustDlg` | UI | 49 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KCustomEdit` | UI | 48 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KCamera` | HW | 42 | прибор: LCD/сенсор/камера/USB/принтер/MCU |
| `KSystemSetDlg` | UI | 39 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KImgTableItem` | UI | 38 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KImgTableModel` | UI | 37 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KScopeInfoEdit` | UI | 37 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KOptionListButton` | UI | 37 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KVideoCal` | UI | 35 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KPinyinWidget` | UI | 34 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KVideoPlayerOSD` | UI | 34 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KExamDetailInfoUi` | UI | 34 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KUserSrvSet` | UI | 32 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KPatientManagmentUi` | UI | 32 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KEmpDateEdit` | UI | 31 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KTableView` | UI | 28 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KThsaurusManageMentUi` | REPORT | 28 | редактор отчётов — Qt-виджеты |
| `KImageEditor` | REPORT | 27 | редактор отчётов — Qt-виджеты |

## 7. Как перегенерировать

```sh
python3 tools/coverage.py > docs/COVERAGE.md
```

Правила доменов/верифицируемости — `DOMAIN_RULES` и `verifiability()` в `tools/coverage.py`.
