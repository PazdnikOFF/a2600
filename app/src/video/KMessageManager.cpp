#include "KMessageManager.h"

#include <cstdlib>
#include <cstring>

namespace x2000video {

EndoConnectPara ParseEndoConnect(const Para &p)
{
    // Реф. EndoConnectCmdHdl @0x9208: четыре int16 подряд с начала нагрузки.
    EndoConnectPara out;
    std::memcpy(&out.connect, p.raw + 0, 2);
    std::memcpy(&out.type,    p.raw + 2, 2);
    std::memcpy(&out.width,   p.raw + 4, 2);
    std::memcpy(&out.height,  p.raw + 6, 2);
    return out;
}

Para MakeEndoConnect(int16_t connect, int16_t type, int16_t w, int16_t h)
{
    Para p{};
    std::memcpy(p.raw + 0, &connect, 2);
    std::memcpy(p.raw + 2, &type, 2);
    std::memcpy(p.raw + 4, &w, 2);
    std::memcpy(p.raw + 6, &h, 2);
    return p;
}

Para MakeTwoInts(int32_t a, int32_t b)
{
    // Реф. MsgSend(type,int,int) @0x67d8: два int32 в начале, хвост нулями.
    Para p{};
    std::memcpy(p.raw + 0, &a, 4);
    std::memcpy(p.raw + 4, &b, 4);
    return p;
}

bool IsValidVideoSavePath(const std::string &path)
{
    // Реф. GetVideoSavePath @0x90b8: strcmp(p + strlen(p) - 4, ".mp4").
    // ⚠️ В реф. при длине < 4 это чтение ЗА пределами начала строки; у нас
    // короткая строка честно отбрасывается.
    if (path.size() < 4)
        return false;
    return path.compare(path.size() - 4, 4, ".mp4") == 0;
}

HandleResult HandleMessage(EncoderState &st, const KMsgBuf &msg)
{
    // Реф. @0x93d8 — switch по mtype. Коды сверены дизасмом:
    // mov x2,#0x1002 / #0x1003 / #0x2001, mov x1,#0x1001.
    HandleResult r;
    switch (static_cast<uint32_t>(msg.mtype)) {

    case MSG_IMAGESAVE_START:
        // Реф. ImageSaveStartCmdHdl @0x9130: если пайплайн не запущен —
        // поднять снимочный, затем SaveImage().
        r.known = true;
        if (st.current == KSNAP_KRECORD_NONE) {
            r.pipelineRequest = KSNAP_PIPELINE_RUN;
            r.pipelineTouched = true;
            st.current = KSNAP_PIPELINE_RUN;
        }
        break;

    case MSG_VIDEOSAVE_START:
        // Реф. VideoSaveStartCmdHdl @0x9310: если УЖЕ пишем — сразу отказ.
        r.known = true;
        if (st.current == KRECORD_PIPELINE_RUN) {
            r.respType = MSG_VIDEOSAVE_START_READY;
            r.respParam = RESP_FAIL;
        } else {
            st.recState = 1;
            r.pipelineRequest = KSNAP_KRECORD_NONE;   // реф. RunPipeline(-1)
            r.pipelineTouched = true;
        }
        break;

    case MSG_VIDEOSAVE_STOP:
        // Реф. VideoSaveStopCmdHdl @0x91a0: SetRecState(0), RunPipeline(-1).
        r.known = true;
        st.recState = 0;
        r.pipelineRequest = KSNAP_KRECORD_NONE;
        r.pipelineTouched = true;
        break;

    case MSG_ENDO_CONNECT: {
        // Реф. EndoConnectCmdHdl @0x9208: W/H в KCommData, SetEndoState(connect),
        // RunPipeline(connect ? 0 : -1).
        r.known = true;
        const EndoConnectPara ep = ParseEndoConnect(msg.para);
        st.endoState = ep.connect;
        r.pipelineRequest = ep.connect ? KSNAP_PIPELINE_RUN : KSNAP_KRECORD_NONE;
        r.pipelineTouched = true;
        st.current = r.pipelineRequest;
        break;
    }

    default:
        // Реф. логирует "[W] Message 0x%08x Not Defined!".
        r.known = false;
        break;
    }
    return r;
}

// --- KSharedBuffer ----------------------------------------------------------

KSharedBuffer::KSharedBuffer(size_t bufferSize, unsigned bufferNum)
    : m_bufferSize(bufferSize), m_bufferNum(bufferNum)
{
    // Реф. @0x8db0: malloc(bufferSize * bufferNum).
    if (bufferSize && bufferNum)
        m_buf = static_cast<unsigned char *>(std::malloc(bufferSize * bufferNum));
}

KSharedBuffer::~KSharedBuffer()
{
    std::free(m_buf);
}

void *KSharedBuffer::GetWriteBuffer()
{
    if (!m_buf || !m_bufferNum)
        return nullptr;
    return m_buf + (m_index % m_bufferNum) * m_bufferSize;
}

void *KSharedBuffer::GetReadBuffer()
{
    // Реф.: при index == 0 читать нечего — NULL.
    if (!m_buf || !m_bufferNum || m_index == 0)
        return nullptr;
    return m_buf + ((m_index - 1) % m_bufferNum) * m_bufferSize;
}

} // namespace x2000video
