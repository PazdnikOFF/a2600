#pragma once

#include <QApplication>
#include <memory>

class KUIDesktop;
class KMainCtrlThread;

// Корневой объект приложения: создаёт главный экран KUIDesktop и управляющий
// поток KMainCtrlThread (реф. архитектура X-2600).
class Application : public QApplication
{
    Q_OBJECT
public:
    Application(int &argc, char **argv);
    ~Application() override;

    int run();

private:
    std::unique_ptr<KUIDesktop>      desktop_;
    std::unique_ptr<KMainCtrlThread> mainCtrl_;
};
