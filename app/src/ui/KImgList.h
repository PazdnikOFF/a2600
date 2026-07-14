#pragma once

#include <QWidget>
#include <QString>

class QTableWidget;

// Список снимков осмотра (реф. класс KImgList, X-2600).
// Обёртка над QTableWidget (как в оригинале initUiConfig: setShowGrid,
// setSelectionMode, setSortingEnabled). Показывает миниатюры из папки осмотра.
// Реальные методы: SetExamFolder/GetExamFolder, AppendImg, DeleteImg,
// GetImgTotal, GetCurrentImgPath, PageNext/PagePre.
class KImgList : public QWidget
{
    Q_OBJECT
public:
    explicit KImgList(QWidget *parent = nullptr);

    void SetExamFolder(const QString &dir);
    QString GetExamFolder() const { return examFolder_; }

    void AppendImg();                 // перечитать миниатюры из папки
    int  GetImgTotal() const;
    QString GetCurrentImgPath() const;

public slots:
    void PageNext();
    void PagePre();

private:
    void initUiConfig();

    QTableWidget *table_ = nullptr;
    QString       examFolder_;
    QSize         cell_;
    QSize         icon_;
};
