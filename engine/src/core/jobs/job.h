
#pragma once
#include <mutex>
#include <thread>

#include "core/function/function.h"
#include "memory/global_memory_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    enum JobType
    {
        JobTypeNone = 0x0,
        /** @brief A general job that does not have any specific thread requirements. */
        JobTypeGeneral = 0x2,
        /** @brief A resource loading job. Resources should be loaded from the same thread to avoid disk thrashing. */
        JobTypeResourceLoad = 0x4,
        /**
         * @brief A job that uses GPU resources should be bound to a thread using this job type.
         * MultiThreaded renderers will use a specific job thread and this type of job will
         * run on this thread. For single-threaded renderers this will simply be the main thread.
         */
        JobTypeGpuResource = 0x8,
    };

    enum class JobPriority
    {
        None,
        /** @brief The lowest-priority job, used for things that can wait to be done (for example log flushing).*/
        Low,
        /** @brief The normal-priority job. Should be used for regular priority tasks such as loading assets. */
        Normal,
        /** @brief The highest-priority job. Should be used sparingly and only for time-critical operations. */
        High,
    };

    struct JobResultEntry
    {
        /** @brief The id the job. */
        u16 id;
        /** @brief The callback that we need to call (onSuccess or onFailure depending on the result) */
        StackFunction<void(), 24> callback;

        JobResultEntry() = default;

        JobResultEntry(u16 id, const StackFunction<void(), 24>& callback) : id(id), callback(callback) {}
    };

    struct JobInfo
    {
        bool inUse = false;

        JobType type         = JobTypeGeneral;
        JobPriority priority = JobPriority::Normal;

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
