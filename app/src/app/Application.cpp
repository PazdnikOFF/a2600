#include "app/Application.h"
#include "ui/KUIDesktop.h"
#include "ui/Theme.h"
#include "ctrl/KMainCtrlThread.h"
#include "hal/Hal.h"
#include <memory>   // std::shared_ptr/unique_ptr (libstdc++ не тянет транзитивно)

#include <QByteArray>
#include <QTimer>
#include <cstdlib>

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

    // Отладочный шов (НЕ для прибора): ENDO_SHOT=<png> — снять главный экран через
    // задержку и выйти. Позволяет offscreen-верифицировать реальный десктоп так же,
    // как ui_preview снимает отдельные экраны. На устройстве переменная не задаётся.
    if (const char *shot = std::getenv("ENDO_SHOT")) {
        const QString path = QString::fromLocal8Bit(shot);
        int delayMs = 1200;
        if (const char *d = std::getenv("ENDO_SHOT_DELAY")) delayMs = QByteArray(d).toInt();
        QTimer::singleShot(delayMs, this, [this, path] {
            if (desktop_)
                desktop_->grab().save(path);
            quit();
        });
    }

    return exec();
}
