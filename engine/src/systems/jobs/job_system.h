
#pragma once
#include "core/defines.h"
#include "systems/system.h"

#include "job.h"
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

		C3D_API void Submit(JobInfo info);

	private:
		void StoreResult(const std::function<void(void*)>& callback, u64 dataSize, const void* data);

		void Runner(u32 index);

		void ProcessQueue(RingQueue<JobInfo>& queue, std::mutex& queueMutex);

		bool m_running;
		u8 m_threadCount;

		JobThread m_jobThreads[MAX_JOB_THREADS];

		RingQueue<JobInfo> m_lowPriorityQueue;
		RingQueue<JobInfo> m_normalPriorityQueue;
		RingQueue<JobInfo> m_highPriorityQueue;

		std::mutex m_lowPriorityMutex;
		std::mutex m_normalPriorityMutex;
		std::mutex m_highPriorityMutex;

		JobResultEntry m_pendingResults[MAX_JOB_RESULTS];
		std::mutex m_resultMutex;
	};
}
