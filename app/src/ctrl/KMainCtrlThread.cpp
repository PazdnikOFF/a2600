#include "ctrl/KMainCtrlThread.h"
#include "ctrl/KPlControl.h"
#include "ui/KUIDesktop.h"
#include "video/KViewSoftEndo.h"
#include "video/KVideoProxy.h"
#include "video/KSaveVideoFile.h"
#include "sys/KSystem.h"
#include "hal/Hal.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QDebug>

namespace {
// Файл ProductCn (оригинал читает/пишет его в Init). Путь данных на устройстве.
QString productCnPath() { return "/home/root/data/protected/productcn.ini"; }
}

KMainCtrlThread::KMainCtrlThread(KUIDesktop *desktop, QObject *parent)
    : QObject(parent), desktop_(desktop)
{
    pl_       = new KPlControl(this);
    video_    = new KVideoProxy(this);
    recorder_ = new KSaveVideoFile(this);
    video_->AttachPl(pl_);
}

QString KMainCtrlThread::CurrentExamDir() const
{
    // <data>/exam/<timestamp-осмотра>. В оригинале — путь текущего осмотра пациента.
    return QDir(KSystem::DataPath()).absoluteFilePath("exam/current");
}

KMainCtrlThread::~KMainCtrlThread() = default;

// Порядок вызовов — как в дизассемблере KMainCtrlThread::Init оригинала.
void KMainCtrlThread::Init()
{
    hal::init();                 // GetKHalClass + KHalClass::InitSignalException

    // StartRealTimeSrvThread / StartNormalTimeSrvThread / StartPrintThread —
    // сервисные потоки (таймеры реального/обычного времени, печать). TODO.

    ProductCheck();              // KMainCtrlThread::ProductCheck
    ModelInit();                 // KMainCtrlThread::ModelInit
    // KNetWorkSet::InitLocalNet — инициализация локальной сети. TODO.
    StartUIFPGACheck();          // KMainCtrlThread::StartUIFPGACheck

    if (!IsProductCnExists())    // KMainCtrlThread::IsProductCnExists
        GenerateProductCn();     // KMainCtrlThread::GenerateProductCn
}

void KMainCtrlThread::ProductCheck()
{
    // Оригинал: проверка модели/лицензии/серийника продукта. TODO.
}

void KMainCtrlThread::ModelInit()
{
    // Оригинал: загрузка product.ini/display-раскладок и параметров модели.
    // Здесь связываем видеотракт с вьювером и инициализируем параметры вида.
    if (desktop_ && desktop_->VideoView()) {
        connect(video_, &KVideoProxy::VideoFrameReady,
                desktop_->VideoView(), &KViewSoftEndo::OnVideoFrameReady);
        desktop_->VideoView()->InitVideoParam();
        desktop_->VideoView()->InitStatus();
    }
    ConfigMIPI();
}

void KMainCtrlThread::StartUIFPGACheck()
{
    // Оригинал: проверка связи UI↔FPGA (регистры версии PL/камеры).
    // Реимплементация bring-up: запускаем захват (эндоскоп X-2600 = softendo).
    KVideoProxy::Config cfg;
    cfg.useTestSource = !QFileInfo::exists("/dev/video0");
    if (!video_->InitCamera(cfg))
        qWarning() << "StartUIFPGACheck: InitCamera failed";
}

bool KMainCtrlThread::IsProductCnExists() const
{
    // Оригинал читает файл ProductCn (см. QFile/QTextStream в Init).
    return QFile::exists(productCnPath());
}

void KMainCtrlThread::GenerateProductCn()
{
    // Оригинал: KMainCtrlThread::GenerateProductCn — формирует и пишет ProductCn.
    QFile f(productCnPath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream(&f) << "X-2600\n";
    }
}

void KMainCtrlThread::ConfigMIPI()
{
    // Оригинал: конфигурация PL-видеотракта. На устройстве — media-ctl/v4l2-ctl
    // (switchvideoformat.sh: a00a0000.v_tpg, NV12). Регистры PL — через KPlControl.
}

bool KMainCtrlThread::IsEndoReady() const
{
    return video_ && video_->isRunning();
}

void KMainCtrlThread::HandleMsg(int) {}

void KMainCtrlThread::CameraButtonAct(int button)
{
    switch (button) {
    case 0: // Freeze
        video_->FreezeVideoSwitch(true);
        if (desktop_ && desktop_->VideoView()) desktop_->VideoView()->FreezeAck();
        break;
    case 1: // Snap — сохранить текущий кадр в JPEG (<exam>/<N>.jpeg)
        if (video_->ImageSavePreset(CurrentExamDir(), ++snapIndex_)) {
            // Миниатюра для списка снимков
            KSaveVideoFile::SaveVideoThumbnail(
                video_->CurrentFrame(),
                QString("%1/thumb/%2.jpeg").arg(CurrentExamDir()).arg(snapIndex_),
                QSize(190, 135));
            if (desktop_ && desktop_->VideoView()) desktop_->VideoView()->ImageSaveAck();
        }
        break;
    case 2: // Record — старт/стоп записи MP4 (VCU)
        if (recorder_->IsRecording()) {
            recorder_->StopRecord();
        } else if (!KSaveVideoFile::CheckIsToLimitStatus(KSystem::DataPath(), 200LL * 1024 * 1024)) {
            const QString mp4 = QString("%1/%2.mp4")
                .arg(CurrentExamDir(), video_->GenerateVideoFileName());
            recorder_->StartRecord(mp4, video_->config().device,
                                   video_->config().width, video_->config().height);
        }
        break;
    default: break;
    }
}

void KMainCtrlThread::AddImageBrightness(int) {}

void KMainCtrlThread::EndoStatusChangeAct()
{
    // Оригинал: реакция на подключение/смену эндоскопа → переинициализация камеры.
    StartUIFPGACheck();
}
