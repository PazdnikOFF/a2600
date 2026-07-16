# Карта покрытия реверса X-2600

> Сгенерировано: `python3 tools/coverage.py > docs/COVERAGE.md`
> Источники: `nm update/root/X2000` (свой парсер Itanium-мангла — `c++filt`
> на этом бинарнике не работает), `objdump -d` + строки `.rodata`/`.data*`
> для привязки конфигов, `app/src/**/*.h` — наша сторона (сверка по именам; см. §5.1).
> Домены и off-device-признак — эвристики имён, согласованы с `docs/ROADMAP.md §2`.

## 1. Сводка

| Метрика | Значение |
|---|---|
| Пользовательских классов в референсе | **485** |
| Методов в них (уникальные имена) | **6431** |
| Классов затронуто у нас | **79** (16%) |
| Методов реализовано (совпало по имени) | **782** (12.2%) |

## 2. По доменам

| Домен | Классов | Методов | Реализовано методов | % | Классов затронуто |
|---|---:|---:|---:|---:|---:|
| **UI** | 167 | 2410 | 47 | 2.0% | 6 |
| **CORE** | 60 | 1216 | 382 | 31.4% | 23 |
| **REPORT** | 72 | 752 | 52 | 6.9% | 10 |
| **HW** | 40 | 598 | 4 | 0.7% | 1 |
| **MISC** | 27 | 543 | 130 | 23.9% | 11 |
| **DICOM** | 60 | 423 | 53 | 12.5% | 10 |
| **DB** | 47 | 352 | 100 | 28.4% | 13 |
| **UPDATE** | 12 | 137 | 14 | 10.2% | 5 |

## 3. Верифицируемость off-device

| Признак | Классов | Методов |
|---|---:|---:|
| ✅ | 172 | 1843 |
| ⚠️ | 73 | 1112 |
| ❌ | 240 | 3476 |

## 4. ТОП-40 нереализованных off-device-классов (✅)

Кандидаты на следующие итерации: чистая логика/конфиги, тестируются self-test'ом на Mac.

| Класс | Домен | Методов | Конфиги | Заметка |
|---|---|---:|---|---|
| `KFactoryOptions` | UPDATE | 45 | `testenv.ini` | не начат |
| `KTemplateEditDocument` | REPORT | 38 | — | не начат |
| `KDocumentGenerator` | REPORT | 31 | — | есть заголовок, но свой API — сверить имена |
| `KControlProc` | CORE | 30 | `_delay.ini`, `_import.ini`, `_release.ini` | не начат |
| `KSysReportTempletControl` | REPORT | 24 | — | не начат |
| `KExamBussinessHandler` | CORE | 21 | — | не начат |
| `KReportEditDataSource` | REPORT | 17 | — | не начат |
| `KExportRecord` | DB | 16 | `PicInfo.ini` | не начат |
| `KUpdateAction` | UPDATE | 16 | — | не начат |
| `KDataOprEventDeal` | DB | 16 | — | не начат |
| `KEntityManage` | DB | 15 | — | есть заголовок, но свой API — сверить имена |
| `KEntityBase` | DB | 14 | — | не начат |
| `KImageProcess` | CORE | 14 | — | не начат |
| `KRTTeCreatorContext` | REPORT | 14 | — | не начат |
| `KExamListDBTableHandler` | DB | 13 | — | не начат |
| `KDataFileOpr` | DB | 12 | — | не начат |
| `KUpdatePrepare` | UPDATE | 12 | — | не начат |
| `KEntityService` | DB | 11 | — | есть заголовок, но свой API — сверить имена |
| `KRTAbsDataSource` | REPORT | 11 | — | не начат |
| `KRTDataSourceReal` | REPORT | 11 | — | есть заголовок, но свой API — сверить имена |
| `KReportTemplateManager` | REPORT | 11 | `ReportTemplateConfig.xml` | есть заголовок, но свой API — сверить имена |
| `KRTCreatorContext` | REPORT | 10 | — | не начат |
| `KImportRules` | DB | 9 | — | не начат |
| `KReportDBTableHandler` | REPORT | 9 | — | не начат |
| `KFunTest` | CORE | 9 | `testcaselist.txt` | не начат |
| `KTimeMng` | CORE | 9 | — | не начат |
| `KSelfTest` | CORE | 9 | `testenv.ini` | не начат |
| `KRecordItem` | DB | 8 | — | не начат |
| `KEntityReport` | REPORT | 8 | — | есть заголовок, но свой API — сверить имена |
| `KRTDataSourceDemo` | REPORT | 8 | — | есть заголовок, но свой API — сверить имена |
| `KEntityQuickInputDoctor` | DB | 8 | — | не начат |
| `KEntityQuickInputPatient` | DB | 8 | — | не начат |
| `KEntityQuickInputApplicant` | DB | 8 | — | не начат |
| `KQuickInputDoctorDbTableHandler` | DB | 8 | — | не начат |
| `KQuickInputPatientDbTableHandler` | DB | 8 | — | не начат |
| `KQuickInputApplicantDbTableHandler` | DB | 8 | — | не начат |
| `KVersion` | UPDATE | 8 | — | не начат |
| `KNetWorkSet` | MISC | 7 | `system.ini` | не начат |
| `KThreadPoolMsg` | CORE | 7 | — | не начат |
| `KHalUpdateClass` | UPDATE | 7 | — | не начат |

## 5. Частично реализованные (есть методы, но не все)

| Класс | Домен | Методов | Наших | Покрытие |
|---|---|---:|---:|---:|
| `KMainCtrlThread` | CORE | 112 | 12 | 11% |
| `KSystemSet` | CORE | 101 | 13 | 13% |
| `KViewSoftEndo` | UI | 88 | 4 | 5% |
| `KVideoProxy` | CORE | 115 | 50 | 43% |
| `AlgParaManager` | MISC | 62 | 10 | 16% |
| `KImgList` | UI | 49 | 8 | 16% |
| `KVideoSet` | CORE | 58 | 18 | 31% |
| `KDccuParam` | CORE | 93 | 54 | 58% |
| `KSystem` | CORE | 51 | 14 | 27% |
| `KEncStyle` | CORE | 38 | 4 | 11% |
| `KVideoCal` | UI | 34 | 3 | 9% |
| `KUpdateConf` | UPDATE | 28 | 1 | 4% |
| `KUserOsdSet` | CORE | 34 | 7 | 21% |
| `KVideoParam` | CORE | 42 | 16 | 38% |
| `KUserSet` | CORE | 27 | 3 | 11% |
| `KDicomInterface` | DICOM | 27 | 8 | 30% |
| `KSaveFile` | DB | 20 | 2 | 10% |
| `KProjectSet` | CORE | 31 | 14 | 45% |
| `KColdLightConfig` | CORE | 18 | 2 | 11% |
| `KSystemStatus` | CORE | 45 | 30 | 67% |
| `KThesaurusOpt` | REPORT | 15 | 2 | 13% |
| `KSysReportTempletCfg` | REPORT | 18 | 5 | 28% |
| `KPlControl` | CORE | 71 | 60 | 85% |
| `KManuPwdMng` | CORE | 16 | 6 | 38% |
| `KUIDesktop` | UI | 15 | 6 | 40% |

### 5.1 Оговорка о точности метрики

Счётчик «реализовано» = **совпадение имени метода** с референсом, т.е. это **нижняя
оценка**. Там, где мы сознательно переосмыслили API (свои имена вместо оригинальных),
метрика показывает 0 при фактически рабочем коде:

- **Свой API при наличии класса-референса (9):** `KDocumentGenerator`, `KEntityDicom`, `KEntityManage`, `KEntityReport`, `KEntityService`, `KRTDataSourceDemo`, `KRTDataSourceReal`, `KReportTemplateManager`, `KTempletBaseInfo`

- **Наши абстракции без класса-референса (14):** `Application`, `KDicomDatasetFormat`, `KDicomFieldMap`, `KEndoInfoServerConfig`, `KEntityQuickInput`, `KRemoteSwitchConfig`, `KReportDataSource`, `KSoftEndoParam`, `KStatisticConfig`, `KStyleConfig`, `KUpdateManifest`, `ReportItem`, `hal`, `theme`

Такие классы стоит либо привести к именам оригинала, либо исключить из метрики руками.

## 6. Device-bound (❌) — крупнейшие

Требуют прибора/живого экрана; off-device проверяются только косвенно.

| Класс | Домен | Методов | Причина |
|---|---|---:|---|
| `KLcdProxy` | HW | 100 | прибор: LCD/сенсор/камера/USB/принтер/MCU |
| `KViewSoftEndo` | UI | 88 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KEndoScope` | HW | 76 | прибор: LCD/сенсор/камера/USB/принтер/MCU |
| `KReportEditUi` | REPORT | 63 | редактор отчётов — Qt-виджеты |
| `KExamListViewUi` | UI | 59 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KViewHardEndo` | UI | 58 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KUiMsgProxy` | UI | 54 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KReportEditAddMarkView` | REPORT | 52 | редактор отчётов — Qt-виджеты |
| `KPatientListViewUi` | UI | 51 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KTempletTreeWidget` | REPORT | 51 | редактор отчётов — Qt-виджеты |
| `KImgList` | UI | 49 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KAlgParamAjustDlg` | UI | 48 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KCustomEdit` | UI | 47 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KSystemSetDlg` | UI | 38 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KCalendarWidget` | UI | 38 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KImgTableItem` | UI | 37 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KCamera` | HW | 37 | прибор: LCD/сенсор/камера/USB/принтер/MCU |
| `KScopeInfoEdit` | UI | 36 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KImgTableModel` | UI | 35 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KVideoCal` | UI | 34 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KVideoPlayerOSD` | UI | 33 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KExamDetailInfoUi` | UI | 33 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KPinyinWidget` | UI | 32 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KUserSrvSet` | UI | 31 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KOptionListButton` | UI | 31 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KPatientManagmentUi` | UI | 31 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KTableView` | UI | 27 | Qt5::Widgets — нужен UX-реверс живого экрана |
| `KThsaurusManageMentUi` | REPORT | 27 | редактор отчётов — Qt-виджеты |
| `KImageEditor` | REPORT | 26 | редактор отчётов — Qt-виджеты |
| `KProgressDlg` | UI | 25 | Qt5::Widgets — нужен UX-реверс живого экрана |

## 7. Как перегенерировать

```sh
python3 tools/coverage.py > docs/COVERAGE.md
```

Правила доменов/верифицируемости — `DOMAIN_RULES` и `verifiability()` в `tools/coverage.py`.
