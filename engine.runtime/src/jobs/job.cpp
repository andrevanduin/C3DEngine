
#include "job.h"

namespace C3D
{
    void JobThread::SetInfo(JobInfo&& info)
    {
        m_info = std::move(info);
        m_info.inUse = true;
    }

    void JobThread::ClearInfo() { m_info.inUse = false; }

    bool JobThread::IsFree() const { return !m_info.inUse; }
}  // namespace C3D