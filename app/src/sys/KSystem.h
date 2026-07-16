#pragma once

#include <QString>
#include <QSize>

// Пути и системные параметры (реф. класс KSystem, X-2600).
// Методы повторяют оригинал: SystemPath, DataPath, AppPath, DisplayConfigPath,
// ProductDisplayConfigPath/File, GetSystemResolution и т.д.
// Базой служит корень прошивки (устройство: /home/root; отладка: ENDO_ROOT).
namespace KSystem {

// Цепочка путей сверена с дизасмом (реф. строит их конкатенацией от RootPath).
QString RootPath();              // /home/root                (реф. литерал; у нас ENDO_ROOT)
QString AppPath();               // /home/root/data/app       (реф. = DataPath + "app/")
QString SystemPath();            // /home/root/system
QString DataPath();              // /home/root/data
QString DisplayConfigPath();     // /home/root/system/display
QString SystemPresetPath();      // /home/root/system/presetdata
QString ProjectPresetPath();     // /home/root/system/presetdata/syspreset   (KEnvConfig RO-корень)
QString ProjectUserPresetPath(); // /home/root/system/presetdata/userpreset  (KEnvConfig user-корень)
QString UserPresetPath();        // синоним ProjectUserPresetPath (наше имя, прижилось у вызывающих)
QString SetDataPath();           // /home/root/data/setdata/
QString UserSetPath();           // /home/root/data/setdata/userset/
QString ProtectedPath();         // /home/root/data/protected/
QString VideoConfPath();         // /home/root/system/videoconf
QString ProductDisplayConfigPath(const QString &model);  // .../display/<model>/<model>
QString ProductDisplayConfigFile(const QString &model);  // .../<model>/product.ini

// Разрешение основного монитора (для выбора layout-файла). Мультимонитор — screens().
QSize GetSystemResolution();

// Текущая модель продукта (из display/.../project.ini). По умолчанию X-2600.
QString ProductModel();

} // namespace KSystem
