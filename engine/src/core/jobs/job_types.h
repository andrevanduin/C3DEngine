
#pragma once
#include "core/defines.h"
#include "core/function/function.h"

namespace C3D
{
    /** @brief The maximum number of dependencies a single job can have. */
    constexpr auto MAX_JOB_DEPENDENCIES = 16;

    using JobHandle = u16;

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
        /** @brief The handle for this job. */
        JobHandle handle = INVALID_ID_U16;
        /** @brief The callback that we need to call (onSuccess or onFailure depending on the result). */
        StackFunction<void(), 24> callback;

        JobResultEntry() = default;

        JobResultEntry(u16 id, const StackFunction<void(), 24>& callback) : handle(id), callback(callback) {}
    };
}  // namespace C3D