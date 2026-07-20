#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

// Протокол общения основного процесса X2000 с процессом видеотракта X2000Video
// (реф. ОТДЕЛЬНЫЙ бинарник update/root/X2000Video, 142 КБ; исходники main.cpp,
// public/msgquehandle.cpp, server/kmessagemanager.cpp, server/kencodermanager.cpp).
//
// Транспорт — System V message queue (msgget/msgsnd/msgrcv). Ни сокетов, ни
// POSIX mq, ни сигналов. Парная сторона в X2000 — KProcMessage/KProcMsgManager
// (@0x6ced30/@0x6cd280), там тот же msgget с теми же флагами 0x3B6.
//
// ЗДЕСЬ РЕАЛИЗОВАНО: раскладки, коды, разбор полей, конечный автомат пайплайнов,
// правила путей — всё off-device и проверяемо. Сами msgget/msgsnd/shmat и
// GStreamer не воспроизводятся.
namespace x2000video {

// Каналы (реф. enum _KProcMsg). ЗНАЧЕНИЯ enum'а — ЭТО И ЕСТЬ СЫРЫЕ КЛЮЧИ msgget.
// Подтверждено дизасмом KMessageManager::KMessageManager @0x9000:
//   mov w1, #-0x7f000000 → 0x81000000, объект сохранён в this+0x10 (RX)
//   mov w1, #-0x7e000000 → 0x82000000, объект сохранён в this+0x18 (TX)
// В X2000 ровно ЧЕТЫРЕ ключа (все StartProcMsgManager-сайты перечислены):
// пара видеотракта и пара автотеста. Очереди ЗЕРКАЛЬНЫ: X2000 шлёт на
// 0x81000000 (KVideoProxy::SendCmd2VideoApp @0x6d7790) и читает 0x82000000
// (KMainCtrlThread::HandleTask @0x6cbf10).
enum ProcMsgChannel : uint32_t {
    CH_VIDEO_CMD  = 0x81000000u,   // X2000 → X2000Video (команды)
    CH_VIDEO_RESP = 0x82000000u,   // X2000Video → X2000 (ответы)
    // Пара автотеста: владелец — KAutoTestThread @0x6fb514/@0x6fb52c,
    // peer — X2000Simulator (его main @0x28cc создаёт ровно эти два ключа).
    // KFunTest только ПИШЕТ в 0x85000000, владельцем не является.
    CH_AUTOTEST_CMD  = 0x85000000u,  // X2000 → X2000Simulator
    CH_AUTOTEST_RESP = 0x86000000u,  // X2000Simulator → X2000
};

// Флаги msgsnd/msgrcv (реф. литерал 0x800 = IPC_NOWAIT) — одинаковы на обеих
// сторонах и для обеих пар очередей.
enum : int { MSG_FLAGS_NOWAIT = 0x800 };

// Флаги msgget/shmget (реф. литерал 0x3B6 = IPC_CREAT | 0666).
enum : int { IPC_FLAGS = 0x3B6 };

// Коды сообщений. ⚠️ ВАЖНО ПРО ИМЕНОВАНИЕ: именованного enum'а кодов в X2000
// НЕТ — там это просто `long mtype`, а строк KPMSG_*/KPMST_* в X2000 ровно 0
// (они есть только в X2000Video, 13 штук). Мангл-тип `_KProcMsg` в X2000 — это
// перечисление КЛЮЧЕЙ ОЧЕРЕДЕЙ (см. ProcMsgChannel выше), а НЕ кодов сообщений.
// Имя ProcMsgType и константы ниже — наша сборка из лог-строк X2000Video.
// Входящие коды сверены МНОЙ дизасмом switch'а в HandleMessage @0x93d8:
// mov x2,#0x1002 / #0x1003 / #0x2001, mov x1,#0x1001.
// Сравнение mtype в X2000 (GetVideoAck @0x6d8b28) — 64-битное; 0x80001001
// собирается как mov x2,#0x1001 + movk x2,#0x8000,lsl#16, т.е. ПОЛОЖИТЕЛЬНЫЙ
// long 0x0000000080001001, без знакового расширения.
enum ProcMsgType : uint32_t {
    // X2000 → X2000Video
    MSG_IMAGESAVE_START = 0x1001,
    MSG_VIDEOSAVE_START = 0x1002,
    MSG_VIDEOSAVE_STOP  = 0x1003,
    MSG_ENDO_CONNECT    = 0x2001,
    // X2000Video → X2000
    MSG_THUMBIMAG_READY = 0x3001,  // param 0 = thumb; param -1 = volume-record
    // 0x3002: X2000Video шлёт его из main() сразу после старта (param 1), НО
    // X2000 трактует код как уведомление о смене статуса эндоскопа —
    // KVideoProxy @0x6d8a44 зовёт EndoStatusChangeAct(GetCamera()+0x10) либо
    // GetEndoScope()+0x10 при SoftEndo, и гасит RecordStatus, если тот не 0.
    MSG_ENDO_STATUS_NOTIFY = 0x3002,
    MSG_IMAGESAVE_READY       = 0x80001001u,
    MSG_VIDEOSAVE_START_READY = 0x80001002u,
    MSG_VIDEOSAVE_STOP_READY  = 0x80001003u,
};

// Параметр ответа (реф.: 0 — успех, -1 — ошибка, -2 — прерывание/стоп).
enum : int { RESP_OK = 0, RESP_FAIL = -1, RESP_ABORT = -2 };

// Полезная нагрузка сообщения — 72 байта (реф. _Para). Трактуется по-разному
// в зависимости от кода команды; сырых полей у неё нет.
struct Para {
    uint8_t raw[72];
};

// Сообщение очереди (реф. KMsgBuf, 80 байт). msgsnd/msgrcv ВСЕГДА вызываются
// с msgsz = 0x48 (только mtext, без mtype) — реф. литерал.
struct KMsgBuf {
    long mtype;   // +0x00, 8 байт
    Para para;    // +0x08, 72 байта
};
enum : size_t { MSG_TEXT_SIZE = 0x48 };   // = sizeof(Para) = 72

static_assert(sizeof(Para) == 72, "реф. _Para = 72 байта");
static_assert(sizeof(KMsgBuf) == 80, "реф. KMsgBuf = 80 байт");
static_assert(offsetof(KMsgBuf, para) == 8, "реф. para сразу за mtype");
static_assert(MSG_TEXT_SIZE == sizeof(Para), "msgsz реф. = 0x48");

// Разбор нагрузки MSG_ENDO_CONNECT (реф. EndoConnectCmdHdl @0x9208 читает
// ЧЕТЫРЕ int16 подряд).
struct EndoConnectPara {
    int16_t connect;   // +0x00 — 0/1
    int16_t type;      // +0x02
    int16_t width;     // +0x04
    int16_t height;    // +0x06
};
EndoConnectPara ParseEndoConnect(const Para &p);
Para MakeEndoConnect(int16_t connect, int16_t type, int16_t w, int16_t h);

// Разбор нагрузки для overload MsgSend(type, int, int) @0x67d8: два int32
// в начале, остальные 64 байта обнуляются.
Para MakeTwoInts(int32_t a, int32_t b);

// --- Разделяемая память кадра -----------------------------------------------

// Ключи shmget (реф. синглтоны ImageSaveBuf::getImgShMemory @0x6108 /
// getRecordShMemory @0x6198).
// ⚠️ АСИММЕТРИЯ: в X2000 используется ТОЛЬКО 0xA1000000 (KImgProcThread @0x6f8b10,
// mov w0,#-0x5f000000, размер 0x697A20 через mov x1,#0x7a20 + movk #0x69,lsl#16).
// Ключа 0xA2000000 в X2000 НЕТ вообще — ни литералом, ни через movk. Второй
// сегмент существует только внутри X2000Video (getRecordShMemory @0x6198).
// Мангл-тип ключей shm в X2000 — `_KProcShm`.
enum ShmKey : uint32_t {
    SHM_IMAGE  = 0xA1000000u,   // общий с X2000
    SHM_RECORD = 0xA2000000u,   // ТОЛЬКО внутри X2000Video
};

// Раскладка сегмента (реф. shmget(key, 0x697A20, 0x3B6)). Имя структуры НАШЕ —
// в реф. это «голый» буфер, адресуемый смещениями.
// Кадр: 0x697800 = 6 912 000 = 1920 * 1200 * 3 (RGB24) — арифметика сходится
// точно, что подтверждает и формат, и разрешение.
enum : size_t {
    SHM_FRAME_WIDTH  = 1920,
    SHM_FRAME_HEIGHT = 1200,
    SHM_FRAME_BPP    = 3,
    SHM_FRAME_SIZE   = 0x697800,   // 6912000
    SHM_TOTAL_SIZE   = 0x697A20,   // 6912544 = кадр + 544 байта метаданных
};

#pragma pack(push, 1)
struct ImageShmLayout {
    uint8_t  frame[SHM_FRAME_SIZE];  // 0x000000 — RGB24 1920x1200
    char     volumePath[256];        // 0x697800 — из KCommData+0x248
    char     thumbPath[256];         // 0x697900 — из KCommData+0x348
    int32_t  width;                  // 0x697A00
    int32_t  height;                 // 0x697A04
    int32_t  bpp;                    // 0x697A08 — всегда 3
    uint8_t  pad0[4];                // 0x697A0C
    int64_t  recordField;            // 0x697A10 — копируется в KCommData+0x40
    uint8_t  pendingThumb;           // 0x697A18 — флаг «thumb/volume ожидает»
    uint8_t  pad1[7];                // 0x697A19
};
#pragma pack(pop)

static_assert(SHM_FRAME_SIZE == SHM_FRAME_WIDTH * SHM_FRAME_HEIGHT * SHM_FRAME_BPP,
              "кадр 0x697800 обязан быть 1920*1200*3");
static_assert(offsetof(ImageShmLayout, volumePath)   == 0x697800, "");
static_assert(offsetof(ImageShmLayout, thumbPath)    == 0x697900, "");
static_assert(offsetof(ImageShmLayout, width)        == 0x697A00, "");
static_assert(offsetof(ImageShmLayout, height)       == 0x697A04, "");
static_assert(offsetof(ImageShmLayout, bpp)          == 0x697A08, "");
static_assert(offsetof(ImageShmLayout, recordField)  == 0x697A10, "");
static_assert(offsetof(ImageShmLayout, pendingThumb) == 0x697A18, "");
static_assert(sizeof(ImageShmLayout) == SHM_TOTAL_SIZE, "реф. shmget = 0x697A20");

// Реф. KMessageManager::GetVideoSavePath @0x90b8: берёт base+0x697800 и
// ТРЕБУЕТ суффикс ".mp4" — иначе возвращает NULL. Правило однострочное, но
// ломкое (см. self-test).
bool IsValidVideoSavePath(const std::string &path);

// --- Конечный автомат пайплайнов --------------------------------------------

// Реф. KEncodermanager, поле +0x18 (Get/SetCurrentPipline). Значения из
// лог-строк RunPipeline @0x7208.
enum PipelineType : int {
    KSNAP_KRECORD_NONE  = -1,
    KSNAP_PIPELINE_RUN  = 0,
    KRECORD_PIPELINE_RUN = 1,
};

// Состояние менеджера энкодера (реф. поля KEncodermanager).
struct EncoderState {
    int current = KSNAP_KRECORD_NONE;  // +0x18
    int endoState = 0;                 // +0x14
    int recState = 0;                  // +0x10
};

// Реф. HandleMessage @0x93d8 — диспетчер. Возвращает исходящий код ответа
// (0, если ответ не шлётся). Побочные эффекты — в state.
// Реализованы ветки, не требующие GStreamer.
struct HandleResult {
    uint32_t respType = 0;   // код ответа в очередь CH_VIDEO_RESP
    int respParam = 0;
    bool known = false;      // false → реф. логирует "Message 0x%08x Not Defined!"
    int pipelineRequest = KSNAP_KRECORD_NONE;   // что просят у RunPipeline
    bool pipelineTouched = false;
};
HandleResult HandleMessage(EncoderState &st, const KMsgBuf &msg);

// --- Локальный кольцевой буфер ----------------------------------------------

// Реф. KSharedBuffer @0x8db0. ⚠️ ВОПРЕКИ ИМЕНИ это НЕ разделяемая память:
// обычный malloc внутри процесса. Межпроцессной синхронизации доступа к shm
// в X2000Video нет вообще (KSemaphoreManger — это внутрипроцессный QSemaphore).
class KSharedBuffer
{
public:
    KSharedBuffer(size_t bufferSize, unsigned bufferNum);
    ~KSharedBuffer();

    // Реф.: buf + (index % bufferNum) * bufferSize.
    void *GetWriteBuffer();
    // Реф.: index==0 → NULL; иначе buf + ((index-1) % bufferNum) * bufferSize.
    void *GetReadBuffer();
    void  SetPrivateData(void *p) { m_priv = p; }
    void  Advance() { ++m_index; }   // наше (реф. индекс двигают потребители)

private:
    unsigned char *m_buf = nullptr;  // +0x00
    size_t   m_bufferSize = 0;       // +0x08
    unsigned m_bufferNum = 0;        // +0x0C
    unsigned m_index = 0;            // +0x10
    void    *m_priv = nullptr;       // +0x18
};

} // namespace x2000video
