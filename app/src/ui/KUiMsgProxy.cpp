#include "ui/KUiMsgProxy.h"

#include <QStringList>

namespace {
QVector<KUiMsgProxy::Sent> g_sent;
QStringList g_displayed;
}

KUiMsgProxy *GetKUiMsgProxy()
{
    static KUiMsgProxy inst;
    return &inst;
}

QVector<KUiMsgProxy::Sent> KUiMsgProxy::TakeSent() { return g_sent; }
void KUiMsgProxy::ClearSent() { g_sent.clear(); g_displayed.clear(); }
QStringList KUiMsgProxy::TakeDisplayed() { return g_displayed; }

void KUiMsgProxy::SendToMainCtrl(int a) { g_sent.push_back({1, a, 0, 0}); }
void KUiMsgProxy::SendToMainCtrl(int a, int b) { g_sent.push_back({2, a, b, 0}); }
void KUiMsgProxy::SendToMainCtrl(int a, int b, int c) { g_sent.push_back({3, a, b, c}); }
void KUiMsgProxy::FileView(int v) { g_sent.push_back({1, -1, v, 0}); }
void KUiMsgProxy::PanelKeyVersion() { g_sent.push_back({0, -2, 0, 0}); }
void KUiMsgProxy::PanelKeyEndoInfo() { g_sent.push_back({0, -3, 0, 0}); }
void KUiMsgProxy::UpdateRigidEndoBtnGuide() { g_sent.push_back({0, -4, 0, 0}); }
void KUiMsgProxy::UpdateFlexEndoBtnGuide() { g_sent.push_back({0, -5, 0, 0}); }
void KUiMsgProxy::DisplayMsg(const QString &msg) { g_displayed << msg; }
void KUiMsgProxy::SigLightAdjustDisabel() { g_sent.push_back({0, -6, 0, 0}); }
