
#pragma once
#include "core/defines.h"
#include "systems/system.h"

#include "job.h"
#include "containers/dynamic_array.h"
#include "containers/ring_queue.h"

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

	class JobSystem final : public System<JobSystemConfig>
	{
	public:
		JobSystem();

		bool Init(const JobSystemConfig& config) override;
		void Shutdown() override;

		void Update();

		template <typename InputType, typename OutputType>
		C3D_API void Submit(const JobInfo<InputType, OutputType>& info)
		{
			// First we need to make a copy and allocate memory to keep track of this JobInfo
			auto jobInfo = Memory.New<JobInfo<InputType, OutputType>>(MemoryType::Job, info);

			// If the job priority is high, we try to start it immediately
			if (jobInfo->priority == JobPriority::High)
			{
				for (auto& thread : m_jobThreads)
				{
					if (thread.typeMask & jobInfo->type)
					{
						std::lock_guard threadLock(thread.mutex);
						if (thread.IsFree())
						{
							m_logger.Trace("Submit() - Job immediately submitted on thread {} since it has HIGH priority.", thread.index);
							thread.SetInfo(jobInfo);
							return;
						}
					}
				}
			}

			// We need to lock our queue in case the job is submitted from another job/thread
			switch (jobInfo->priority)
			{
				case JobPriority::High:
				{
					std::lock_guard queueLock(m_highPriorityMutex);
					m_highPriorityQueue.Enqueue(jobInfo);
					break;
				}
				case JobPriority::Normal:
				{
					std::lock_guard queueLock(m_normalPriorityMutex);
					m_normalPriorityQueue.Enqueue(jobInfo);
					break;
				}
				case JobPriority::Low:
				{
					std::lock_guard queueLock(m_lowPriorityMutex);
					m_lowPriorityQueue.Enqueue(jobInfo);
					break;
				}
				default:
				case JobPriority::None:
					m_logger.Error("Submit() - Failed to submit job since it has priority type NONE {}.");
					break;
			}

			m_logger.Trace("Submit() - Job has been queued.");
		}

	private:
		void StoreResult(BaseJobInfo* info, bool wasSuccess);

		void Runner(u32 index);

		void ProcessQueue(RingQueue<BaseJobInfo*>& queue, std::mutex& queueMutex);

		bool m_running;
		u8 m_threadCount;

		JobThread m_jobThreads[MAX_JOB_THREADS];

		RingQueue<BaseJobInfo*> m_lowPriorityQueue;
		RingQueue<BaseJobInfo*> m_normalPriorityQueue;
		RingQueue<BaseJobInfo*> m_highPriorityQueue;

		std::mutex m_lowPriorityMutex;
		std::mutex m_normalPriorityMutex;
		std::mutex m_highPriorityMutex;

		DynamicArray<BaseJobResultEntry*> m_pendingResults;
		std::mutex m_resultMutex;
	};
}
