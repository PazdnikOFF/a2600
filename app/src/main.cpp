#include "app/Application.h"

// Точка входа. На приборе Qt-платформа — eglfs (Mali):
//   QT_QPA_PLATFORM=eglfs ./endostation
// На десктопе — обычный оконный бэкенд, видео от videotestsrc.
int main(int argc, char **argv)
{
    Application app(argc, argv);
    return app.run();
}
