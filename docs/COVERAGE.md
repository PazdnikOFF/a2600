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
| Классов затронуто у нас | **105** (22%) |
| Методов реализовано (совпало по имени) | **1238** (19.3%) |

## 2. По доменам

| Домен | Классов | Методов | Реализовано методов | % | Классов затронуто |
|---|---:|---:|---:|---:|---:|
| **UI** | 167 | 2410 | 99 | 4.1% | 10 |
| **CORE** | 60 | 1216 | 475 | 39.1% | 28 |
| **REPORT** | 72 | 752 | 144 | 19.1% | 17 |
| **HW** | 40 | 598 | 184 | 30.8% | 6 |
| **MISC** | 27 | 543 | 137 | 25.2% | 13 |
| **DICOM** | 60 | 423 | 56 | 13.2% | 10 |
| **DB** | 47 | 352 | 129 | 36.6% | 16 |
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
| `KUpdateAction` | UPDATE | 16 | — | не начат |
| `KDataOprEventDeal` | DB | 16 | — | не начат |
| `KEntityManage` | DB | 15 | — | есть заголовок, но свой API — сверить имена |
| `KEntityBase` | DB | 14 | — | не начат |
| `KRTTeCreatorContext` | REPORT | 14 | — | не начат |
| `KDataFileOpr` | DB | 12 | — | не начат |
| `KUpdatePrepare` | UPDATE | 12 | — | не начат |
| `KEntityService` | DB | 11 | — | есть заголовок, но свой API — сверить имена |
| `KRTAbsDataSource` | REPORT | 11 | — | не начат |
| `KRTDataSourceReal` | REPORT | 11 | — | есть заголовок, но свой API — сверить имена |
| `KRTCreatorContext` | REPORT | 10 | — | не начат |
| `KImportRules` | DB | 9 | — | не начат |
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
| `KHalUpdateClass` | UPDATE | 7 | — | не начат |
| `KThreadMessageQueue` | CORE | 7 | — | не начат |
| `KReportTemplateConfig` | REPORT | 7 | `ReportTemplateConfig.xml` | не начат |
| `KPicMng` | CORE | 7 | — | не начат |
| `KExamData` | DB | 7 | — | не начат |
| `KUpdateMng` | UPDATE | 6 | — | не начат |
| `KVideoPlay` | CORE | 6 | — | не начат |
| `KRTTableItemCreator` | REPORT | 6 | — | не начат |
| `KReportEditUIConfig` | REPORT | 6 | — | не начат |
| `KPdf2Pics` | REPORT | 6 | — | не начат |
| `KEntityDEC` | DB | 5 | — | не начат |

## 5. Частично реализованные (есть методы, но не все)

| Класс | Домен | Методов | Наших | Покрытие |
|---|---|---:|---:|---:|
| `KMainCtrlThread` | CORE | 112 | 12 | 11% |
| `KSystemSet` | CORE | 101 | 15 | 15% |
| `KViewSoftEndo` | UI | 88 | 4 | 5% |
| `KVideoProxy` | CORE | 115 | 50 | 43% |
| `AlgParaManager` | MISC | 62 | 10 | 16% |
| `KUiMsgProxy` | UI | 54 | 8 | 15% |
| `KImgList` | UI | 49 | 8 | 16% |
| `KVideoSet` | CORE | 58 | 18 | 31% |
| `KDccuParam` | CORE | 93 | 54 | 58% |
| `KEndoScope` | HW | 76 | 40 | 53% |
| `KSystem` | CORE | 51 | 17 | 33% |
| `KVideoCal` | UI | 34 | 3 | 9% |
| `KUpdateConf` | UPDATE | 28 | 1 | 4% |
| `KUserOsdSet` | CORE | 34 | 8 | 24% |
| `KVideoParam` | CORE | 42 | 16 | 38% |
| `KUserSet` | CORE | 27 | 3 | 11% |
| `KUsbDevice` | HW | 24 | 2 | 8% |
| `KEncStyle` | CORE | 38 | 18 | 47% |
| `KProjectSet` | CORE | 31 | 14 | 45% |
| `KSaveFile` | DB | 20 | 3 | 15% |
| `KDicomInterface` | DICOM | 27 | 11 | 41% |
| `KThesaurusOpt` | REPORT | 15 | 2 | 13% |
| `KColdLightConfig` | CORE | 18 | 6 | 33% |
| `KPlControl` | CORE | 71 | 60 | 85% |
| `KSystemStatus` | CORE | 45 | 34 | 76% |

### 5.1 Оговорка о точности метрики

Счётчик «реализовано» = **совпадение имени метода** с референсом, т.е. это **нижняя
оценка**. Там, где мы сознательно переосмыслили API (свои имена вместо оригинальных),
метрика показывает 0 при фактически рабочем коде:

- **Свой API при наличии класса-референса (8):** `KDocumentGenerator`, `KEntityDicom`, `KEntityManage`, `KEntityReport`, `KEntityService`, `KExamEntry`, `KRTDataSourceDemo`, `KRTDataSourceReal`

- **Наши абстракции без класса-референса (21):** `Application`, `K3ADimming`, `KAutoTestScript`, `KDicomDatasetFormat`, `KDicomFieldMap`, `KEndoInfoServerConfig`, `KEntityQuickInput`, `KRemoteSwitchConfig`, `KReportDataSource`, `KScopeClass`, `KSoftEndoParam`, `KStatisticConfig`, `KStyleConfig`, `KSystemLog`, `KUpdateManifest`, `ReportItem`, `_LcdActItem`, `hal`, `keyname`, `theme`, `yxyDES2`

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
