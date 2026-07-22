#include "kernel/KThreadPoolMsg.h"

#include <mutex>
#include <memory>   // std::shared_ptr/unique_ptr (libstdc++ не тянет транзитивно)

namespace {
std::mutex g_mutex;
std::vector<KThreadPoolMsg::Msg> g_posted;
}

void KThreadPoolMsg::PostMsgToUI(int dest, int id, int p1, int p2,
                                 const std::shared_ptr<void> & /*payload*/)
{
    std::lock_guard<std::mutex> lk(g_mutex);
    g_posted.push_back(Msg{dest, id, p1, p2});
}

std::vector<KThreadPoolMsg::Msg> KThreadPoolMsg::TakePosted()
{
    std::lock_guard<std::mutex> lk(g_mutex);
    return g_posted;
}

void KThreadPoolMsg::ClearPosted()
{
    std::lock_guard<std::mutex> lk(g_mutex);
    g_posted.clear();
}
