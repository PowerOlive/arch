/*
 * SocketChannel.cpp
 *
 *  Created on: 2011-1-12
 *      Author: qiyingwang
 */
#include "channel/all_includes.hpp"
#include "util/time_helper.hpp"
#include "logging/logger_macros.hpp"

using namespace arch::channel;
using namespace arch::channel::timer;

using namespace arch::util;
using namespace arch::concurrent;

int TimerChannel::TimeoutCB(struct aeEventLoop *eventLoop, long long id,
        void *clientData)
{
    TimerChannel* channel = (TimerChannel*) clientData;
    int64 nextTime = channel->Routine();
    //int64 nextTime = channel->GetNearestTaskTriggerTime();
    if (nextTime > 0)
    {
        //uint64 now = getCurrentTimeMillis();
        //return nextTime - now;
        return nextTime;
    }
    //ASSERT(nextTime > 0);
    channel->m_timer_id = -1;
    return AE_NOMORE;
}

void TimerChannel::OnScheduled(TimerTask* task)
{
    //TimerTask* nearestTask = getNearestTimerTask();
    int64 nextTime = GetNearestTaskTriggerTime();
    TRACE_LOG(
            "Nearest task trigger time is %lld, task's trigger time is %lld in timer:%d ", nextTime, task->GetNextTriggerTime(), m_timer_id);
    if (nextTime < 0
            || static_cast<uint64>(nextTime) > task->GetNextTriggerTime())
    {
        if (-1 != m_timer_id)
        {
            if (nextTime > 0)
            {
                aeModifyTimeEvent(m_service.GetRawEventLoop(), m_timer_id,
                        task->GetDelay());
            }
            else
            {
                //just ignore, let timer handle it later
            }
        }
        else
        {
            if (-1 != m_timer_id)
            {
                aeDeleteTimeEvent(m_service.GetRawEventLoop(), m_timer_id);
                m_timer_id = -1;
            }TRACE_LOG(
                    "Create time event with timeout value:%d", task->GetDelay());
            m_timer_id = aeCreateTimeEvent(m_service.GetRawEventLoop(),
                    task->GetDelay(), TimeoutCB, this, NULL);
        }
    }
}

TimerChannel::~TimerChannel()
{
    if (-1 != m_timer_id)
    {
        aeDeleteTimeEvent(m_service.GetRawEventLoop(), m_timer_id);
    }
}
