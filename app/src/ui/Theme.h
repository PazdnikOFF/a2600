#pragma once

#include <QString>

// Работа с готовой структурой прошивки (как на устройстве /home/root):
//   <root>/qss/black/style.qss   — стиль (url() ссылается на qss/black/...)
//   <root>/system/style/<model>/<brand>/qss  — бренд-набор (лого, фон)
//   <root>/system/...            — конфиги, пресеты, гамма и т.д.
//
// На устройстве <root> = /home/root; при отладке задаётся через ENDO_ROOT
// (указывает на update/root). Ничего не копируем — используем файлы прошивки.
namespace theme {

QString root();                          // корень (/home/root или ENDO_ROOT)
QString loadStyleSheet();                // qss/black/style.qss с абсолютными url()
QString asset(const QString &relToQss);  // <root>/qss/<relToQss>
QString brand(const QString &file);      // <root>/system/style/X-2600/PyCkeun/qss/<file>

} // namespace theme
