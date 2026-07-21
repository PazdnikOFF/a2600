#pragma once

#include <QTextEdit>
#include <QString>

// Многострочный редактор с ограничением длины и счётчиком (реф. KCounterTextEdit @ctor 0x5a7d10,
// base QTextEdit — НЕ контейнер). UI-порт РЕАЛЬНОГО кастом-виджета — ранее подставлялся
// QTextEdit + отдельный QLabel-счётчик. Счётчик здесь НЕ встроен: виджет ЭМИТИТ строку
// «cur/max» сигналом ChangeCounterShowText, а метку рисует внешняя форма.
//
// Логика (реф.): default cap = 800; InitWidget(n) ставит cap и подключает textChanged→OnTextChanged
// (при n<0 — no-op, без ограничения). OnTextChanged: если length()>max — обрезать до left(max),
// курсор в End, эмит счётчика с max; иначе эмит счётчика с length(). Считает QChar (UTF-16),
// CJK-safe. Текстовый I/O — штатный QTextEdit (setPlainText/toPlainText/clear/setPlaceholderText).
// 100% PORT: чистый Qt.
class KCounterTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit KCounterTextEdit(QWidget *parent = nullptr);

    // Реф. @0x5a7d50 — фактический SetMaxLength: ставит cap и подключает счётчик (n<0 → no-op).
    void InitWidget(int nMaxLength);

    // Реф. @0x5a7df8 — «nCount/max».
    QString GetCounterShowText(int nCount) const;

signals:
    void ChangeCounterShowText(const QString &text);

private slots:
    void OnTextChanged();   // реф. @0x5a8048

private:
    int m_nMaxLength = 800;   // +0x30
};
