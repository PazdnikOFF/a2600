#include "KToProgressDlgMsgDispatcher.h"

#include <QMetaType>

KToProgressDlgMsgDispatcher::KToProgressDlgMsgDispatcher(QObject *parent)
    : QObject(parent)
{
    // Реф. ctor @0x4511b0: только регистрация метатипа (сообщения ходят между потоками).
    qRegisterMetaType<KMessage>("KMessage");
}

void KToProgressDlgMsgDispatcher::EmitUpdateTitleTotalProgressSig(const QString &title, int p)
{ emit SigUpdateTitleTotalProgress(title, p); }          // реф. @0x450ea8
void KToProgressDlgMsgDispatcher::EmitUpdateHideSig()
{ emit SigUpdateHide(); }                                // реф. @0x450f50 (tail-jump)
void KToProgressDlgMsgDispatcher::EmitShowResultMsgBox(const QString &msg)
{ emit SigShowResultMsgBox(msg); }                       // реф. @0x450f58
void KToProgressDlgMsgDispatcher::EmitUpdateTotalProgress(int p)
{ emit SigUpdateTotalProgress(p); }                      // реф. @0x451000 (tail-jump)
void KToProgressDlgMsgDispatcher::EmitUpdateSubProgress(int p)
{ emit SigUpdateSubProgress(p); }                        // реф. @0x451008 (tail-jump)
void KToProgressDlgMsgDispatcher::EmitUpdateTotalLabel(const QString &t)
{ emit SigUpdateTotalLabel(t); }                         // реф. @0x451010
void KToProgressDlgMsgDispatcher::EmitUpdateSubLabel(const QString &t)
{ emit SigUpdateSubLabel(t); }                           // реф. @0x4510b8
void KToProgressDlgMsgDispatcher::EmitOneExamRecordUpdateFinish(const KMessage &msg)
{ emit SigOneExamRecordUpdateFinish(msg); }              // реф. @0x451160 (копия на стеке)

KToProgressDlgMsgDispatcher *ToDlgMsgDispatcher()
{
    // Реф. @0x4511f8: Meyers-статик (объект @0xa61b08, guard @0xa61b00).
    static KToProgressDlgMsgDispatcher inst;
    return &inst;
}
