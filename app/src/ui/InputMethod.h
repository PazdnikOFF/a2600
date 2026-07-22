#pragma once

#include <QString>
#include <functional>
#include <map>

class QKeyEvent;
class QWidget;

// Маршрутизатор ввода по языкам (реф. InputMethod, singleton getInstance @0x761568). UI-порт.
// На каждый keypress (реф. из KAppThread::notify) InputMutiLang выбирает обработчик по активному
// языку: 0=Chinese→InputChinese (кормит ПОРТИРОВАННЫЙ KPinyinWidget), 1=English→native (пропуск),
// 2/3/6=Latin/4=French/5=Russian/7=Polish→per-lang коммит. SEAM: активный язык реф. из
// languageConfig/KSystemSet::GetInputMethod (QSettings) → в порте SetActiveLanguage (дефолт Chinese).
class InputMethod
{
public:
    enum _KLanguageType {
        Lang_Chinese = 0, Lang_English = 1, Lang_Latin2 = 2, Lang_Latin3 = 3,
        Lang_French = 4, Lang_Russian = 5, Lang_Latin6 = 6, Lang_Polish = 7
    };

    static InputMethod &GetInstance();

    void SetActiveLanguage(int lang) { m_activeLang = lang; }   // seam (реф. languageConfig)
    int  ActiveLanguage() const { return m_activeLang; }

    bool InputMutiLang(QKeyEvent *e) const;   // реф. @0x7615f8: диспетч по языку

private:
    InputMethod();
    void SetFunctionMap();                     // реф. @0x761278

    bool InputChinese(QKeyEvent *e, QWidget *w) const;   // реф. @0x7608e8 → KPinyinWidget
    bool InputLatin(QKeyEvent *e, QWidget *w) const;     // реф. @0x760fa0
    bool InputRussian(QKeyEvent *e, QWidget *w) const;   // реф. @0x760c18
    bool InputFrench(QKeyEvent *e, QWidget *w) const;
    bool InputPolish(QKeyEvent *e, QWidget *w) const;
    bool CommitText(QKeyEvent *e, QWidget *w) const;     // общий: перевод-клавиши → commit (упрощ.)

    std::map<int, std::function<bool(QKeyEvent *, QWidget *)>> m_map;   // this+0x08
    int m_activeLang = Lang_Chinese;
};
