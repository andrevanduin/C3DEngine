
#pragma once
#include "containers/dynamic_array.h"
#include "containers/ring_queue.h"
#include "core/defines.h"
#include "core/jobs/job.h"
#include "systems/system.h"

namespace C3D
{
    /* @brief The maximum amount of job threads that can be used by the system.
     * This is the upper-limit regardless of what the user provides in the config. */
    constexpr auto MAX_JOB_THREADS = 32;
    /* @brief The maximum amount of job results that can be stored at once (per frame). */
    constexpr auto MAX_JOB_RESULTS = 512;

    struct JobSystemConfig
    {
        /* @brief The amount of threads that the job system may use. */
        u8 threadCount;
        /* @brief A collection of type masks for each job thread. The amount of elements must match maxJobThreads. */
        u32* typeMasks;
    };

    class JobSystem final : public SystemWithConfig<JobSystemConfig>
    {
    public:
        explicit JobSystem(const SystemManager* pSystemsManager);

        bool OnInit(const JobSystemConfig& config) override;
        void OnShutdown() override;

        void OnUpdate(const FrameData& frameData) override;

        C3D_API void Submit(JobInfo&& jobInfo);

    private:
        void Runner(u32 index);

        void ProcessQueue(RingQueue<JobInfo>& queue, std::mutex& queueMutex);

        bool m_running   = false;
        u8 m_threadCount = 0;

        JobThread m_jobThreads[MAX_JOB_THREADS] = {};

        RingQueue<JobInfo> m_lowPriorityQueue;
        RingQueue<JobInfo> m_normalPriorityQueue;
        RingQueue<JobInfo> m_highPriorityQueue;

        std::mutex m_lowPriorityMutex;
        std::mutex m_normalPriorityMutex;
        std::mutex m_highPriorityMutex;

        DynamicArray<JobResultEntry> m_pendingResults;
        std::mutex m_resultMutex;
    };
}  // namespace C3D
