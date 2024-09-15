
#include "job_system.h"

#include "cson/cson_types.h"
#include "formatters.h"
#include "frame_data.h"
#include "platform/platform.h"
#include "renderer/renderer_frontend.h"

namespace C3D
{
    bool JobSystem::OnInit(const CSONObject& config)
    {
        INFO_LOG("Initializing.");

        // Parse the user provided config
        for (const auto& prop : config.properties)
        {
            if (prop.name.IEquals("threadCount"))
            {
                m_config.threadCount = prop.GetI64();
            }
        }

        if (m_config.threadCount == 0)
        {
            ERROR_LOG("maxJobThreads must be > 0.");
            return false;
        }

        if (m_config.threadCount > MAX_JOB_THREADS)
        {
            ERROR_LOG("maxJobThreads must be <= {}.", MAX_JOB_THREADS);
            return false;
        }

        m_threadCount = m_config.threadCount;

        m_pendingResults.Reserve(100);

        INFO_LOG("Main thread id is: {}.", Platform::GetThreadId());
        INFO_LOG("Spawning {} job threads.", m_threadCount);

        // Prepare the job thread types
        u32 jobThreadTypes[15];
        for (u32& jobThreadType : jobThreadTypes) jobThreadType = JobTypeGeneral;

        if (m_config.threadCount == 1 || !Renderer.IsMultiThreaded())
        {
            jobThreadTypes[0] |= (JobTypeGpuResource | JobTypeResourceLoad);
        }
        else if (m_config.threadCount == 2)
        {
            jobThreadTypes[0] |= JobTypeGpuResource;
            jobThreadTypes[1] |= JobTypeResourceLoad;
        }
        else
        {
            jobThreadTypes[0] = JobTypeGpuResource;
            jobThreadTypes[1] = JobTypeResourceLoad;
        }

        // Set the system to running
        m_running = true;

        // Spawn and start running all threads
        for (u8 i = 0; i < m_threadCount; i++)
        {
            m_jobThreads[i].index    = i;
            m_jobThreads[i].typeMask = jobThreadTypes[i];
            m_jobThreads[i].thread   = std::thread([this, i] { Runner(i); });
            m_jobThreads[i].ClearInfo();
        }

        return true;
    }

    void JobSystem::OnShutdown()
    {
        INFO_LOG("Joining all job threads.");

        m_running = false;

        for (auto& jobThread : m_jobThreads)
        {
            if (jobThread.thread.joinable()) jobThread.thread.join();
        }

        // Destroy our queues
        m_lowPriorityQueue.Clear();
        m_normalPriorityQueue.Clear();
        m_highPriorityQueue.Clear();
    }

    bool JobSystem::OnUpdate(const FrameData& frameData)
    {
        // Process all our queues
        ProcessQueue(m_highPriorityQueue, m_highPriorityMutex);
        ProcessQueue(m_normalPriorityQueue, m_normalPriorityMutex);
        ProcessQueue(m_lowPriorityQueue, m_lowPriorityMutex);

        // Process all pending results
        for (i64 i = m_pendingResults.SSize() - 1; i >= 0; --i)
        {
            JobResultEntry entry;
            {
                // Lock our result so we can take a copy of our pointer and remove it from our array
                std::lock_guard resultLock(m_resultMutex);
                entry = m_pendingResults[i];
            }

            // Execute our callback
            entry.callback();

            {
                // Lock our results array again so we can clear this pending result
                std::lock_guard resultLock(m_resultMutex);
                m_pendingResults.Erase(i);
            }
        }

        return true;
    }

    JobHandle JobSystem::Submit(const StackFunction<bool(), 24>& entry, const StackFunction<void(), 24>& onSuccess,
                                const StackFunction<void(), 24>& onFailure, JobType type, JobPriority priority, u8* dependencies,
                                u8 numberOfDependencies)
    {
        JobInfo info;
        // Copy over the type
        info.type = type;
        // Copy over the priority
        info.priority = priority;
        // Copy over the entry, onSuccess and onFailure functions
        info.entryPoint = entry;
        info.onSuccess  = onSuccess;
        info.onFailure  = onFailure;
        // Copy over the number of dependencies
        info.numberOfDependencies = numberOfDependencies;
        // copy over the dependencies
        std::memcpy(info.dependencies, dependencies, numberOfDependencies);

        // Linearly keep track of the next handle we will hand out
        static u16 nextHandle = 0;
        // Keep track off and increment the handle
        u16 handle = nextHandle++;
        // Store the handle on the JobInfo
        info.handle = nextHandle++;

        // If the job priority is high (and has no dependencies), we try to start it immediately
        if (info.priority == JobPriority::High && info.numberOfDependencies == 0)
        {
            for (auto& thread : m_jobThreads)
            {
                if (thread.typeMask & info.type)
                {
                    std::lock_guard threadLock(thread.mutex);
                    if (thread.IsFree())
                    {
                        TRACE("Job: '{}' immediately submitted on thread '{}' since it has HIGH priority.", handle, thread.index);
                        thread.SetInfo(std::move(info));
                        return handle;
                    }
                }
            }
        }

        // We need to lock our queue in case the job is submitted from another job/thread
        switch (info.priority)
        {
            case JobPriority::High:
            {
                std::lock_guard queueLock(m_highPriorityMutex);
                m_highPriorityQueue.Enqueue(info);
                break;
            }
            case JobPriority::Normal:
            {
                std::lock_guard queueLock(m_normalPriorityMutex);
                m_normalPriorityQueue.Enqueue(info);
                break;
            }
            case JobPriority::Low:
            {
                std::lock_guard queueLock(m_lowPriorityMutex);
                m_lowPriorityQueue.Enqueue(info);
                break;
            }
            default:
            case JobPriority::None:
                ERROR_LOG("Failed to submit job since it has priority type NONE.");
                break;
        }

        TRACE("Job: '{}' has been queued.", handle);
        return handle;
    }

    void JobSystem::Runner(const u32 index)
    {
        auto& currentThread = m_jobThreads[index];
        auto threadId       = currentThread.thread.get_id();

        TRACE("Starting job thread #{} (id={}, type={}).", index, threadId, currentThread.typeMask);

        // Keep running, waiting for jobs
        while (true)
        {
            if (!m_running) break;

            // Grab a copy of our info
            JobInfo info;
            {
                std::lock_guard threadLock(currentThread.mutex);
                info = currentThread.GetInfo();
            }

            if (info.inUse)
            {
                // Call our entry point and do the work and store the result of the work
                // if the user has provided a onSuccess callback (in the case of success)
                // or if the user has provided a onFailure callback (in the case of a failure)
                TRACE("Executing job on thread #{}.", index);

                static u16 resultId = 0;

                if (info.entryPoint())
                {
                    if (info.onSuccess)
                    {
                        std::lock_guard resultLock(m_resultMutex);
                        m_pendingResults.EmplaceBack(resultId++, info.onSuccess);
                    }
                }
                else
                {
                    if (info.onFailure)
                    {
                        std::lock_guard resultLock(m_resultMutex);
                        m_pendingResults.EmplaceBack(resultId++, info.onFailure);
                    }
                }

                // Clear out our current thread's info
                {
                    std::lock_guard threadLock(currentThread.mutex);
                    currentThread.ClearInfo();
                }
            }

            if (m_running)
            {
                // TODO: This needs to be improved. Maybe sleeping until a request comes through for a new job?
                Platform::SleepMs(10);
            }
        }

        TRACE("Stopping job thread #{} (id={}, type={}).", index, threadId, currentThread.typeMask);
    }

    void JobSystem::ProcessQueue(RingQueue<JobInfo, 128>& queue, std::mutex& queueMutex)
    {
        while (!queue.Empty())
        {
            const auto jobInfo = queue.Peek();

            // Find a thread that matches the type of job and that is not currently doing any work.
            bool threadFound = false;
            for (auto& thread : m_jobThreads)
            {
                // Skip threads that don't match the type of job
                if ((thread.typeMask & jobInfo.type) == 0) continue;
                {
                    // Lock our thread so we can access it
                    std::lock_guard threadLock(thread.mutex);

                    // If the thread is free (not doing any work) we queue this job
                    if (thread.IsFree())
                    {
                        JobInfo nextJobInfo;
                        // Lock the queue
                        {
                            std::lock_guard queueLock(queueMutex);
                            nextJobInfo = queue.Dequeue();
                        }

                        thread.SetInfo(std::move(nextJobInfo));
                        TRACE("Assigning job to thread: #{}.", thread.index);

                        threadFound = true;
                    }
                }

                // If we have queued the job we can break out of loop
                if (threadFound) break;
            }

            // This means all the thread are currently busy handling jobs.
            // So wait until the next update and try again.
            if (!threadFound) break;
        }
    }
}  // namespace C3D
