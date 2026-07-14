#pragma once

#include <QWidget>

class QLabel;
class KViewSoftEndo;
class KImgList;

// Главный экран основного монитора (реф. класс KUIDesktop, X-2600).
// Холст 1920×1080, чёрный фон. Области по Rect из display/IMG*-UI19201080.ini.
// Методы повторяют оригинал: ShowRigidEndoBtnGuide, OpenWHB, RecLeftTime,
// SetStopWatchState, SingleKeyAct, closeDesktop.
class KUIDesktop : public QWidget
{
    Q_OBJECT
public:
    explicit KUIDesktop(QWidget *parent = nullptr);

    KViewSoftEndo *VideoView() const { return videoView_; }
    KImgList      *ImgList() const { return imgList_; }

    // --- методы из оригинального KUIDesktop ---
    void ShowRigidEndoBtnGuide(bool show);
    void OpenWHB();                 // баланс белого (White Balance)
    void RecLeftTime(int seconds);  // оставшееся время записи
    void SetStopWatchState(bool on);
    void closeDesktop();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override; // SingleKeyAct и т.п.

private:
    void InitUiLayout();                       // построение областей экрана
    QWidget *BuildPatientPanel();
    QWidget *BuildOsdPanel();
    QLabel  *MakeIconLabel(const QString &assetRel, int w, int h);
    QWidget *MakePatientField(const QString &iconRel, const QString &placeholder);

    KViewSoftEndo *videoView_ = nullptr;
    KImgList      *imgList_ = nullptr;
    QLabel        *recLeftTime_ = nullptr;
};
