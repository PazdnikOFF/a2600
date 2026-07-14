#include "app/Application.h"
#include "ui/KUIDesktop.h"
#include "ui/Theme.h"
#include "ctrl/KMainCtrlThread.h"
#include "hal/Hal.h"

Application::Application(int &argc, char **argv) : QApplication(argc, argv)
{
    setApplicationName("EndoStation");
    setOrganizationName("EndoStation");
}

Application::~Application() = default;

int Application::run()
{
    hal::init();
    setStyleSheet(theme::loadStyleSheet());

    desktop_  = std::make_unique<KUIDesktop>();
    mainCtrl_ = std::make_unique<KMainCtrlThread>(desktop_.get());

    desktop_->show();
    mainCtrl_->Init();

    return exec();
}
