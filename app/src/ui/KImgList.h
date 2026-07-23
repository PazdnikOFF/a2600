#pragma once

#include <QWidget>
#include <QString>

class QTableWidget;
class KImgListCell;

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
    // Реф. Ui_KImgList: под таблицей четыре ячейки-превью KImgListCell (KlList_img0..3).
    void SetImgInfo(int index, const QString &imgPath);   // реф. @0x67fe38
    void ClearImgInfo();                                  // реф. @0x67fef8
    KImgListCell *Cell(int index) const { return (index >= 0 && index < 4) ? m_cells[index] : nullptr; }
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
    KImgListCell *m_cells[4] = {nullptr, nullptr, nullptr, nullptr};
};
