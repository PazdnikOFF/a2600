cmake_minimum_required(VERSION 3.13)
project(EndoStation VERSION 0.1.0 LANGUAGES CXX)

# --- Реконструкция ПО эндоскопической стойки (см. ../docs/ARCHITECTURE.md) ---
# Цель: Zynq UltraScale+ EV, PetaLinux, aarch64. На десктопе собирается для отладки
# (видео от videotestsrc, если /dev/video0 недоступен).

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ---- Qt5 ----
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network Sql Xml)

# ---- GStreamer (через pkg-config) ----
# Необязателен на этапе конфигурации: без него собирается только Qt-only ui_preview
# (основное приложение endostation требует GStreamer и включается ниже по GST_FOUND).
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST IMPORTED_TARGET
    gstreamer-1.0
    gstreamer-app-1.0
    gstreamer-video-1.0)
if(NOT GST_FOUND)
    message(WARNING "GStreamer не найден — цель endostation отключена, доступен только ui_preview")
endif()

# ---- OpenCV (опционально: обработка изображения, M5) ----
find_package(OpenCV QUIET COMPONENTS core imgproc imgcodecs)
if(OpenCV_FOUND)
    message(STATUS "OpenCV found: ${OpenCV_VERSION}")
else()
    message(STATUS "OpenCV not found — модуль обработки будет отключён")
endif()

# ---- libhal (опционально: линкуется на устройстве) ----
find_library(HAL_LIB NAMES hal)
if(HAL_LIB)
    message(STATUS "libhal found: ${HAL_LIB}")
else()
    message(STATUS "libhal not found — HAL-фасад в режиме заглушки")
endif()

# Реконструкция использует имена классов оригинального ПО (X-2600):
# KUIDesktop, KViewSoftEndo, KVideoProxy, KMainCtrlThread.
set(SOURCES
    src/main.cpp
    src/app/Application.cpp
    src/ctrl/KMainCtrlThread.cpp
    src/ctrl/KPlControl.cpp
    src/ctrl/KDccuParam.cpp
    src/ctrl/KColdLightConfig.cpp
    src/ctrl/K3ADimming.cpp
    src/endo/KEndoScope.cpp
    src/endo/KCamera.cpp
    src/ui/KUiMsgProxy.cpp
    src/ui/KLcdProxy.cpp
    src/ui/KScreenMng.cpp
    src/ui/KDialog.cpp
    src/ui/KUpdatePrepare.cpp
    src/ui/KImportRules.cpp
    src/ui/KUpdateAction.cpp
    src/ui/KFunTest.cpp
    src/ui/KMessageBox.cpp
    src/ui/KProgressBar.cpp
    src/ui/KWaitProgressBar.cpp
    src/ui/KVersion.cpp
    src/ui/KSysDicom.cpp
    src/ui/KLogin.cpp
    src/ui/KDeviceInfo.cpp
    src/ui/KGeneralSetDlg.cpp
    src/ui/KControlInfo.cpp
    src/ui/KAuthMachineDlg.cpp
    src/ui/KSystemSetDlg.cpp
    src/ui/KSysPrinter.cpp
    src/video/KVideoProxy.cpp
    src/video/KViewSoftEndo.cpp
    src/video/KVideoParam.cpp
    src/video/KVideoSet.cpp
    src/video/KVideoCal.cpp
    src/video/KSaveVideoFile.cpp
    src/ui/KUIDesktop.cpp
    src/ui/KDisplayOption.cpp
    src/ui/KImgList.cpp
    src/ui/KStopWatch.cpp
    src/ui/Theme.cpp
    src/sys/KSystem.cpp
    src/sys/KEnvConfig.cpp
    src/sys/KUserSet.cpp
    src/sys/KUserOsdSet.cpp
    src/sys/KAccount.cpp
    src/sys/KManuPwdMng.cpp
    src/sys/KSessionInfo.cpp
    src/sys/KSystemSet.cpp
    src/sys/KUpdateConf.cpp
    src/sys/KUpdateManifest.cpp
    src/sys/KEndoInfoServerConfig.cpp
    src/sys/KRemoteSwitchConfig.cpp
    src/sys/KPatientTimeOperation.cpp
    src/sys/KVersionConfig.cpp
    src/sys/KProjectSet.cpp
    src/sys/KStyleConfig.cpp
    src/sys/KStatisticConfig.cpp
    src/sys/languageConfig.cpp
    src/sys/KLoadUnicodeText.cpp
    src/sys/KEncStyle.cpp
    src/sys/KFactoryOptions.cpp
    src/sys/KSysPrintData.cpp
    src/sys/KThirdPartyLegalNotices.cpp
    src/autotest/KAutoTestScript.cpp
    src/sys/KEncSettings.cpp
    src/db/KExamListRecordFileUpdate.cpp
    src/db/KExamNoGenerate.cpp
    src/db/KDbFileOperation.cpp
    src/sys/KSystemStatus.cpp
    src/endo/KSoftEndoParam.cpp
    src/db/KEntityManage.cpp
    src/db/KDbStrHandler.cpp
    src/db/KDbSqlite.cpp
    src/db/KEntityQuickInput.cpp
    src/db/KExamListConfigHandler.cpp
    src/db/KPatientListConfigSetupHandler.cpp
    src/db/KWorklistConfigSetupHandler.cpp
    src/db/KEntityExam.cpp
    src/db/KExamListDBTableHandler.cpp
    src/db/KExportRecord.cpp
    src/alg/KImageProcess.cpp
    src/report/KReportEditDataSource.cpp
    src/video/KVideoPlayerMng.cpp
    src/sys/KSmallLangTranslate.cpp
    src/sys/KKey2Name.cpp
    src/report/KReportEntity.cpp
    src/db/KExamBussinessHandler.cpp
    src/db/KExamDataFileNameGenerator.cpp
    src/db/KPatientMngExamStatus.cpp
    src/sys/KTimeInfo.cpp
    src/hal/KUsbDevice.cpp
    src/kernel/KSystemLog.cpp
    src/kernel/KThreadPoolMsg.cpp
    src/db/KEntityPatient.cpp
    src/db/KEntityDoctor.cpp
    src/db/KFileBackup.cpp
    src/db/KSaveFile.cpp
    src/db/KPatientExamData.cpp
    src/db/KEntityService.cpp
    src/kernel/KConfig.cpp
    src/kernel/yxyDES2.cpp
    src/kernel/KControlProc.cpp
    src/kernel/KMessage.cpp
    src/kernel/KObject.cpp
    src/kernel/KPublishManager.cpp
    src/kernel/KControlINI.cpp
    src/alg/AlgParaManager.cpp
    src/dicom/KDicomFieldMap.cpp
    src/dicom/KDicomDatasetFormat.cpp
    src/dicom/KEntityDicom.cpp
    src/dicom/KDicomInterface.cpp
    src/dicom/KPatientStringOperation.cpp
    src/dicom/KDbStringOperation.cpp
    src/dicom/KSysDICOMData.cpp
    src/report/KMeaXMLBase.cpp
    src/report/XmlParser.cpp
    src/report/KMeaStringUtil.cpp
    src/report/KTemplateCfg.cpp
    src/report/KTemplateLibCfg.cpp
    src/report/KTemplateParamCfg.cpp
    src/report/KReportTemplateCommonDef.cpp
    src/report/KTextBlock.cpp
    src/report/KImageBlock.cpp
    src/report/KTableBlock.cpp
    src/report/KTitleTableBlock.cpp
    src/report/KReportTemplate.cpp
    src/report/KSysReportTempletCfg.cpp
    src/report/KSysReportTempletModel.cpp
    src/report/KSysReportTempletControl.cpp
    src/report/KReportDataSource.cpp
    src/report/KReportDisplayParam.cpp
    src/monitor/X2000Monitor.cpp
    src/video/KMessageManager.cpp
    src/report/KRTCreatorContext.cpp
    src/report/KDocumentGenerator.cpp
    src/report/KReportHtmlGenerator.cpp
    src/report/KEntityReport.cpp
    src/report/KThesaurusOpt.cpp
    src/report/KRTDataSourceReal.cpp
    src/report/KRTDataSourceDemo.cpp
    src/hal/Hal.cpp
    src/hal/KMemDevice.cpp
)

set(HEADERS
    src/app/Application.h
    src/ctrl/KMainCtrlThread.h
    src/ctrl/KPlControl.h
    src/video/KVideoProxy.h
    src/video/KViewSoftEndo.h
    src/ui/KUIDesktop.h
    src/ui/KDisplayOption.h
    src/ui/Theme.h
    src/sys/KSystem.h
    src/hal/Hal.h
)

if(GST_FOUND)
    add_executable(endostation ${SOURCES} ${HEADERS})
    target_include_directories(endostation PRIVATE src)
    target_link_libraries(endostation PRIVATE
        Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network Qt5::Sql Qt5::Xml
        PkgConfig::GST)
    target_compile_definitions(endostation PRIVATE HAVE_GST=1)  # видео-тракт KVideoProxy

    if(OpenCV_FOUND)
        target_link_libraries(endostation PRIVATE ${OpenCV_LIBS})
        target_compile_definitions(endostation PRIVATE HAVE_OPENCV=1)
    endif()

    if(HAL_LIB)
        target_link_libraries(endostation PRIVATE ${HAL_LIB})
        target_compile_definitions(endostation PRIVATE HAVE_HAL=1)
    endif()
endif()

# ---- Qt-only превью UI (без GStreamer): рендер экранов и сверка с оригиналом ----
add_executable(ui_preview
    src/ui/preview_main.cpp
    src/ui/Theme.cpp
    src/ui/KUIDesktop.cpp
    src/ui/KDisplayOption.cpp
    src/ui/KImgList.cpp
    src/ui/KStopWatch.cpp
    src/sys/KSystem.cpp
    src/sys/KEnvConfig.cpp
    src/sys/KUserSet.cpp
    src/sys/KUserOsdSet.cpp
    src/sys/KAccount.cpp
    src/sys/KManuPwdMng.cpp
    src/sys/KSessionInfo.cpp
    src/sys/KSystemSet.cpp
    src/sys/KUpdateConf.cpp
    src/sys/KUpdateManifest.cpp
    src/sys/KEndoInfoServerConfig.cpp
    src/sys/KRemoteSwitchConfig.cpp
    src/sys/KPatientTimeOperation.cpp
    src/sys/KVersionConfig.cpp
    src/sys/KProjectSet.cpp
    src/sys/KStyleConfig.cpp
    src/sys/KStatisticConfig.cpp
    src/sys/languageConfig.cpp
    src/sys/KLoadUnicodeText.cpp
    src/sys/KEncStyle.cpp
    src/sys/KFactoryOptions.cpp
    src/sys/KSysPrintData.cpp
    src/sys/KThirdPartyLegalNotices.cpp
    src/autotest/KAutoTestScript.cpp
    src/sys/KEncSettings.cpp
    src/db/KExamListRecordFileUpdate.cpp
    src/db/KExamNoGenerate.cpp
    src/db/KDbFileOperation.cpp
    src/sys/KSystemStatus.cpp
    src/endo/KSoftEndoParam.cpp
    src/db/KEntityManage.cpp
    src/db/KDbStrHandler.cpp
    src/db/KDbSqlite.cpp
    src/db/KEntityQuickInput.cpp
    src/db/KExamListConfigHandler.cpp
    src/db/KPatientListConfigSetupHandler.cpp
    src/db/KWorklistConfigSetupHandler.cpp
    src/db/KEntityExam.cpp
    src/db/KExamListDBTableHandler.cpp
    src/db/KExportRecord.cpp
    src/alg/KImageProcess.cpp
    src/report/KReportEditDataSource.cpp
    src/video/KVideoPlayerMng.cpp
    src/sys/KSmallLangTranslate.cpp
    src/sys/KKey2Name.cpp
    src/report/KReportEntity.cpp
    src/db/KExamBussinessHandler.cpp
    src/db/KExamDataFileNameGenerator.cpp
    src/db/KPatientMngExamStatus.cpp
    src/sys/KTimeInfo.cpp
    src/hal/KUsbDevice.cpp
    src/kernel/KSystemLog.cpp
    src/kernel/KThreadPoolMsg.cpp
    src/db/KEntityPatient.cpp
    src/db/KEntityDoctor.cpp
    src/db/KFileBackup.cpp
    src/db/KSaveFile.cpp
    src/db/KPatientExamData.cpp
    src/db/KEntityService.cpp
    src/kernel/KConfig.cpp
    src/kernel/yxyDES2.cpp
    src/kernel/KControlProc.cpp
    src/kernel/KMessage.cpp
    src/kernel/KObject.cpp
    src/kernel/KPublishManager.cpp
    src/kernel/KControlINI.cpp
    src/alg/AlgParaManager.cpp
    src/ctrl/KPlControl.cpp
    src/ctrl/KDccuParam.cpp
    src/ctrl/KColdLightConfig.cpp
    src/ctrl/K3ADimming.cpp
    src/endo/KEndoScope.cpp
    src/endo/KCamera.cpp
    src/ui/KUiMsgProxy.cpp
    src/ui/KLcdProxy.cpp
    src/ui/KScreenMng.cpp
    src/ui/KDialog.cpp
    src/ui/KUpdatePrepare.cpp
    src/ui/KImportRules.cpp
    src/ui/KUpdateAction.cpp
    src/ui/KFunTest.cpp
    src/ui/KMessageBox.cpp
    src/ui/KProgressBar.cpp
    src/ui/KWaitProgressBar.cpp
    src/ui/KVersion.cpp
    src/ui/KSysDicom.cpp
    src/ui/KLogin.cpp
    src/ui/KDeviceInfo.cpp
    src/ui/KGeneralSetDlg.cpp
    src/ui/KControlInfo.cpp
    src/ui/KAuthMachineDlg.cpp
    src/ui/KSystemSetDlg.cpp
    src/ui/KSysPrinter.cpp
    src/hal/KMemDevice.cpp
    src/dicom/KDicomFieldMap.cpp
    src/dicom/KDicomDatasetFormat.cpp
    src/dicom/KEntityDicom.cpp
    src/dicom/KDicomInterface.cpp
    src/dicom/KPatientStringOperation.cpp
    src/dicom/KDbStringOperation.cpp
    src/dicom/KSysDICOMData.cpp
    src/report/KMeaXMLBase.cpp
    src/report/XmlParser.cpp
    src/report/KMeaStringUtil.cpp
    src/report/KTemplateCfg.cpp
    src/report/KTemplateLibCfg.cpp
    src/report/KTemplateParamCfg.cpp
    src/report/KReportTemplateCommonDef.cpp
    src/report/KTextBlock.cpp
    src/report/KImageBlock.cpp
    src/report/KTableBlock.cpp
    src/report/KTitleTableBlock.cpp
    src/report/KReportTemplate.cpp
    src/report/KSysReportTempletCfg.cpp
    src/report/KSysReportTempletModel.cpp
    src/report/KSysReportTempletControl.cpp
    src/report/KReportDataSource.cpp
    src/report/KReportDisplayParam.cpp
    src/monitor/X2000Monitor.cpp
    src/video/KMessageManager.cpp
    src/report/KRTCreatorContext.cpp
    src/report/KDocumentGenerator.cpp
    src/report/KReportHtmlGenerator.cpp
    src/report/KEntityReport.cpp
    src/report/KThesaurusOpt.cpp
    src/report/KRTDataSourceReal.cpp
    src/report/KRTDataSourceDemo.cpp
    src/video/KViewSoftEndo.cpp
    src/video/KVideoParam.cpp
    src/video/KVideoSet.cpp
    src/video/KVideoCal.cpp
    src/video/KVideoProxy.cpp   # off-device-логика (gst-тракт спрятан за HAVE_GST)
)
target_include_directories(ui_preview PRIVATE src)
target_link_libraries(ui_preview PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Sql Qt5::Xml Qt5::Network sqlite3)
# iconv: на macOS отдельная libiconv, на Xilinx-target — в glibc (линкуется через libc).
find_library(ICONV_LIB NAMES iconv)
if(ICONV_LIB)
    target_link_libraries(ui_preview PRIVATE ${ICONV_LIB})
endif()
