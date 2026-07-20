#pragma once

#include <memory>
#include <vector>

// Мост «рабочий поток → UI» (реф. KThreadPoolMsg, X-2600).
// Off-device реализация — очередь-журнал: сообщения складываются, self-test их
// проверяет. Коды сообщений из реверса KExamBussinessHandler:
//   12041 (0x2f09) — осмотр создан в БД,
//   12044          — каталог данных осмотра переименован.
class KThreadPoolMsg
{
public:
    struct Msg { int dest; int id; int p1; int p2; };

    static void PostMsgToUI(int dest, int id, int p1, int p2,
                            const std::shared_ptr<void> &payload);

    // Не из реф. — доступ к журналу для self-test.
    static std::vector<Msg> TakePosted();
    static void ClearPosted();
};
