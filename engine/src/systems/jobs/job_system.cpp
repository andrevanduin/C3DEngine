
#include "core/formatters.h"
#include "job_system.h"

#include "platform/platform.h"

namespace C3D
{
	JobSystem::JobSystem()
		: System("JOB_SYSTEM"), m_running(false), m_threadCount(0), m_jobThreads{}, m_pendingResults{}
	{}

	bool JobSystem::Init(const JobSystemConfig& config)
	{
		if (config.threadCount == 0)
		{
			m_logger.Error("Init() - maxJobThreads must be > 0.");
			return false;
		}

		if (config.threadCount > MAX_JOB_THREADS)
		{
			m_logger.Error("Init() - maxJobThreads must be <= {}", MAX_JOB_THREADS);
			return false;
		}

		// Create our ring queues for our jobInfo's
		m_lowPriorityQueue.Create(512);
		m_normalPriorityQueue.Create(512);
		m_highPriorityQueue.Create(512);

		m_threadCount = config.threadCount;

		m_pendingResults.Reserve(100);

		m_logger.Info("Main thread id is: {}", Platform::GetThreadId());
		m_logger.Info("Spawning {} job threads.", m_threadCount);

		m_running = true;

		for (u8 i = 0; i < m_threadCount; i++) 
		{
			m_jobThreads[i].index = i;
			m_jobThreads[i].typeMask = config.typeMasks[i];
			m_jobThreads[i].thread = std::thread([this, i] { Runner(i); });
			m_jobThreads[i].SetInfo(nullptr);
		}
		return true;
	}

	void JobSystem::Shutdown()
	{
		m_running = false;

		for (auto& jobThread : m_jobThreads)
		{
			if (jobThread.thread.joinable()) jobThread.thread.join();
		}

		// Destroy our queues
		m_lowPriorityQueue.Destroy();
		m_normalPriorityQueue.Destroy();
		m_highPriorityQueue.Destroy();
	}

	void JobSystem::Update()
	{
		// Process all our queues
		ProcessQueue(m_highPriorityQueue, m_highPriorityMutex);
		ProcessQueue(m_normalPriorityQueue, m_normalPriorityMutex);
		ProcessQueue(m_lowPriorityQueue, m_lowPriorityMutex);

		// Process all pending results
		for (i64 i = m_pendingResults.SSize() - 1; i >= 0; --i)
		{
			BaseJobResultEntry* entry;
			{
				// Lock our result so we can take a copy of our pointer and remove it from our array
				std::lock_guard resultLock(m_resultMutex);
				entry = m_pendingResults[i]->MakeCopy();
			}

			// Execute our callback
			entry->Callback();

			// Free our memory
			Memory.Delete(MemoryType::Job, entry);

			// Lock our actual result array
			std::lock_guard resultLock(m_resultMutex);

			// Delete the memory associated with the result
			Memory.Delete(MemoryType::Job, m_pendingResults[i]);

			// Remove it from our results array
			m_pendingResults.Erase(i);
		}
	}

	void JobSystem::StoreResult(BaseJobInfo* info, const bool wasSuccess)
	{
		// Lock our m_pendingResults
		std::lock_guard lock(m_resultMutex);
		m_pendingResults.PushBack(info->MakeResultEntry(wasSuccess));
	}

	void JobSystem::Runner(const u32 index)
	{
		auto& currentThread = m_jobThreads[index];
		auto threadId = currentThread.thread.get_id();

		m_logger.Trace("Starting job thread #{} (id={}, type={}).", index, threadId, currentThread.typeMask);

		// Keep running, waiting for jobs
		while (true)
		{
			if (!m_running) break;

			// Grab a copy of our info
			BaseJobInfo* info;
			{
				std::lock_guard threadLock(currentThread.mutex);
				info = currentThread.CopyInfo();
			}

			if (info && info->entryPoint)
			{
				// Call our entry point and do the work and store the result of the work
				// if the user has provided a onSuccess callback (in the case of success)
				// or if the user has provided a onFailure callback (in the case of a failure)
				m_logger.Trace("Executing job on thread #{}.", index);

				if (info->CallEntry())
				{
					if (info->onSuccess) StoreResult(info, true);
				}
				else
				{
					if (info->onFailure) StoreResult(info, false);
				}

				// We delete our copy since we are done with it
				Memory.Delete(MemoryType::Job, info);

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

		m_logger.Trace("Stopping job thread #{} (id={}, type={}).", index, threadId, currentThread.typeMask);
	}

	void JobSystem::ProcessQueue(RingQueue<BaseJobInfo*>& queue, std::mutex& queueMutex)
	{
		while (!queue.Empty())
		{
			const auto jobInfo = queue.Peek();

			// Find a thread that matches the type of job and that is not currently doing any work.
			bool threadFound = false;
			for (auto& thread : m_jobThreads)
			{
				// Skip threads that don't match the type of job
				if ((thread.typeMask & jobInfo->type) == 0) continue;

				{
					// Lock our thread so we can access it
					std::lock_guard threadLock(thread.mutex);

					// If the thread is free (not doing any work) we queue this job
					if (thread.IsFree())
					{
						BaseJobInfo* nextJobInfo;
						// Lock the queue
						{
							std::lock_guard queueLock(queueMutex);
							nextJobInfo = queue.Dequeue();
						}

						thread.SetInfo(nextJobInfo);
						m_logger.Trace("Assigning job to thread: #{}", thread.index);

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
}
