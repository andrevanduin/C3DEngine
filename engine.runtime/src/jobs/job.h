
#pragma once
#include <mutex>
#include <thread>

#include "job_types.h"
#include "memory/global_memory_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    struct JobInfo
    {
        bool inUse = false;

        /** @brief The handle for this job. */
        JobHandle handle = INVALID_ID_U16;
        /** @brief The type of this job. */
        JobType type = JobTypeGeneral;
        /** @brief The priority for this job. */
        JobPriority priority = JobPriority::Normal;
        /** @brief An array of dependencies for this job. These should be finished before this job starts. */
        u16 dependencies[MAX_JOB_DEPENDENCIES];
        /** @brief The number of dependencies for this job. */
        u8 numberOfDependencies = 0;
        /** @brief The entry point of the job. Gets called when the job starts. */
        StackFunction<bool(), 24> entryPoint;
        /** @brief An optional callback for when the job finishes successfully. */
        StackFunction<void(), 24> onSuccess;
        /** @brief An optional callback for when the job finishes unsuccessfully. */
        StackFunction<void(), 24> onFailure;
    };

    class JobThread
    {
    public:
        JobThread() = default;

        /** @brief Sets the thread's info. Thread should be locked before calling this! */
        void SetInfo(JobInfo&& info);

        /** @brief Get the thread's info. Thread should be locked before calling this! */
        JobInfo GetInfo() const { return m_info; }

        /** @brief Clears the thread's info. Thread should be locked before calling this! */
        void ClearInfo();

        /** @brief Checks if the thread currently has any work assigned. Thread should be locked before calling this! */
        [[nodiscard]] bool IsFree() const;

        u8 index = 0;
        std::thread thread;
        std::mutex mutex;

        /** @brief The types of jobs this thread can handle. */
        u32 typeMask = 0;

    private:
        JobInfo m_info;
    };
}  // namespace C3D
