#pragma once

#include <QObject>

class KUIDesktop;
class KVideoProxy;
class KPlControl;
class KSaveVideoFile;

// Управляющий поток приложения (реф. класс KMainCtrlThread, X-2600).
// Состав и порядок Init() воспроизводят дизассемблер оригинала:
//   GetKHalClass → InitSignalException → StartRealTimeSrvThread →
//   StartNormalTimeSrvThread → StartPrintThread → ProductCheck → ModelInit →
//   InitLocalNet → StartUIFPGACheck → IsProductCnExists/GenerateProductCn
class KMainCtrlThread : public QObject
{
    Q_OBJECT
public:
    explicit KMainCtrlThread(KUIDesktop *desktop, QObject *parent = nullptr);
    ~KMainCtrlThread() override;

    void Init();                 // главная инициализация (см. порядок выше)
    bool IsEndoReady() const;

    // --- подметоды из оригинального KMainCtrlThread (вызываются из Init) ---
    void ProductCheck();
    void ModelInit();
    void StartUIFPGACheck();
    bool IsProductCnExists() const;
    void GenerateProductCn();

public slots:
    void HandleMsg(int msgId);
    void CameraButtonAct(int button);   // кнопки головки камеры (Freeze/Snap)
    void AddImageBrightness(int delta);
    void EndoStatusChangeAct();         // подключение/смена эндоскопа

private:
    void ConfigMIPI();                  // конфигурация PL-видеотракта

    QString CurrentExamDir() const;    // каталог осмотра для снимков/видео

    KUIDesktop     *desktop_  = nullptr;
    KVideoProxy    *video_    = nullptr;
    KPlControl     *pl_       = nullptr;   // доступ к регистрам FPGA
    KSaveVideoFile *recorder_ = nullptr;   // запись видео
    int             snapIndex_ = 0;        // счётчик снимков в осмотре
};
