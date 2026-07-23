#pragma once

#include <QObject>
#include <QString>

#include "kernel/KMessage.h"

// Мост «фоновая работа → диалог прогресса» (реф. KToProgressDlgMsgDispatcher : QObject,
// ctor @0x4511b0). UI-порт.
//
// Синглтон, но БЕЗ Instance()/CreateInstance(): доступ через свободную функцию
// ToDlgMsgDispatcher() @0x4511f8 (Meyers-статик). ctor не делает ничего, кроме
// qRegisterMetaType<KMessage>() — сообщения ходят через очередь сигналов между потоками.
//
// Все восемь Emit*-методов — ЧИСТЫЕ эмиттеры без побочных эффектов (три из них в реф.
// вообще хвостовые переходы прямо на сигнал). Эмитят фоновые обработчики
// KDicomEventDeal::OnHandle{Download,Upload,Resend}Progress и
// KDataOprEventDeal::OnHandle{FuncCall,FileDelete,DirDelete}{Progress,Result} (device-seam);
// единственный потребитель — KProgressDlg::InitConnect @0x451320, подключающий все восемь.
class KToProgressDlgMsgDispatcher : public QObject
{
    Q_OBJECT
public:
    explicit KToProgressDlgMsgDispatcher(QObject *parent = nullptr);

    // Реф. имена и сигнатуры 1:1 (@0x450ea8..0x451160).
    void EmitUpdateTitleTotalProgressSig(const QString &title, int totalProgress);
    void EmitUpdateHideSig();
    void EmitShowResultMsgBox(const QString &msg);
    void EmitUpdateTotalProgress(int progress);
    void EmitUpdateSubProgress(int progress);
    void EmitUpdateTotalLabel(const QString &text);
    void EmitUpdateSubLabel(const QString &text);
    void EmitOneExamRecordUpdateFinish(const KMessage &msg);

signals:
    // Реф. moc @0x8a52b0 — ровно эти восемь, в этом порядке.
    void SigUpdateTitleTotalProgress(QString qstrTitle, int iTotalProgress);
    void SigUpdateHide();
    void SigShowResultMsgBox(QString qstrMsg);
    void SigUpdateTotalProgress(int iProgress);
    void SigUpdateSubProgress(int iProgress);
    void SigUpdateTotalLabel(QString text);
    void SigUpdateSubLabel(QString text);
    void SigOneExamRecordUpdateFinish(KMessage msg);
};

// Реф. свободная функция @0x4511f8 — единственная точка доступа.
KToProgressDlgMsgDispatcher *ToDlgMsgDispatcher();
