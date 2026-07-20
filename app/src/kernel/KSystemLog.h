#pragma once

#include <string>
#include <vector>

// Журнал приложения (реф. KLogPrint.cpp — СВОБОДНЫЕ функции, класса-логгера нет).
//
// РЕВЕРС формата строки:
//   GetCurTime(buf,32): snprintf("[%02d-%02d %02d:%02d:%02d:%03d]",
//                                mon+1, mday, hour, min, sec, usec/1000)  — БЕЗ года;
//   префикс: snprintf(buf,255,"%s%s", timeStr, tag).
// Итог: `[07-01 12:06:32:789][APP][I]: текст`.
//
// `tag` — литерал вызывающего: "[APP][I]: ", "[APP][W]: ", "[APP][E]: ",
// "[APP][D]: ". УРОВЕНЬ НЕ ВЫЧИСЛЯЕТСЯ — он часть строки тега.
// Единственный рантайм-уровень — g_euLogPriority (SetLogPriority/GetLogPriority),
// и его смотрят ТОЛЬКО …Ex-варианты: печатать если (g_euLogPriority != 0 || bEnable).
//
// Файл: <RootPath>/data/app/logfile/APPlog<YYYY>-<MM>.txt (помесячная ротация ТОЛЬКО
// по имени: нет лимита размера, нет retention, нет нумерованных ротаций).
// Открытие/закрытие fopen(path,"a+")/fclose НА КАЖДЫЙ ВЫЗОВ, без мьютекса и буферизации.
// '\n' добавляется, ТОЛЬКО если fmt им не заканчивается. Если файл не открылся —
// вызов молча ничего не делает (в stdout/stderr реф. не пишет).
void LogPrintf(const char *tag, const char *fmt, ...);
void LogPrintfEx(bool bEnable, const char *tag, const char *fmt, ...);
void LogPrintfx(const char *tag, const char *file, int line, const char *func,
                const char *fmt, ...);

// Реф.: snprintf(buf,256,"%s%s%s\n", timeStr, tag, msg.toLocal8Bit().constData()) —
// перевод строки ДОБАВЛЯЕТСЯ ВСЕГДА (в отличие от LogPrintf).
void QStringLogPrintf(const char *tag, const class QString &msg);
void QStringLogPrintfEx(bool bEnable, const char *tag, const class QString &msg);

void SetLogPriority(int p);
int  GetLogPriority();

// Не из реф. — перехват для self-test (в реф. вывод только в файл).
namespace KSystemLog {
void EnableCapture(bool on);
void ClearCapture();
std::vector<std::string> Captured();   // строки БЕЗ отметки времени: <tag><текст>
std::string LogFileName(int year, int month);   // "APPlog%04d-%02d.txt"
}
