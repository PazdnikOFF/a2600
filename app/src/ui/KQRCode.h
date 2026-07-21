#pragma once

#include <QFrame>

// Панель отображения QR-кода (реф. KQRCode : QFrame — НЕ диалог!, ctor @0x745df0,
// Ui_KQRCode::setupUi @0x745f48). UI-порт. Встраиваемый QFrame (реф. 307×300, minSize
// 300×300, StyledPanel/Raised). Порт как самостоятельный QFrame (базовый класс совпал).
// Чистая display-панель: центрированное QR-изображение (label_QRCodePic, фикс 260×260,
// scaledContents) + центрированная подпись (label_QRCodeInfo, font-size:18px). Ни одного
// connect, кастомов нет. Обе метки пусты — заполняются device (SetQRCodePic/SetQRCodeInfo).
class KQRCode : public QFrame
{
    Q_OBJECT
public:
    explicit KQRCode(QWidget *parent = nullptr);

private:
    void setupUi();
};
