
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

		for (auto& pendingResult : m_pendingResults)
		{
			pendingResult.id = INVALID_ID_U16;
		}

		m_logger.Info("Main thread id is: {}", Platform::GetThreadId());
		m_logger.Info("Spawning {} job threads.", m_threadCount);

		m_running = true;

		for (u8 i = 0; i < m_threadCount; i++)
		{
			m_jobThreads[i].index = i;
			m_jobThreads[i].typeMask = config.typeMasks[i];
			m_jobThreads[i].thread = std::thread([this, i] { Runner(i); });

			m_jobThreads[i].SetInfo({});
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
		for (auto& pendingResult : m_pendingResults)
		{
			// Lock, take a copy of our result and unlock
			JobResultEntry entry;
			{
				std::lock_guard resultLock(m_resultMutex);
				entry = pendingResult;
			}

			// Check if we have a result
			if (entry.id != INVALID_ID_U16)
			{
				// Execute the callback
				entry.callback(entry.result);

				// Lock our pending results and clear 
				std::lock_guard resultLock(m_resultMutex);
				pendingResult.Clear();
			}
		}
	}

	void JobSystem::Submit(JobInfo info)
	{
		// If the job priority is high, we try to start it immediately
		if (info.priority == JobPriority::High)
		{
			for (auto& thread : m_jobThreads)
			{
				if (thread.typeMask & info.type)
				{
					std::lock_guard threadLock(thread.mutex);
					if (thread.IsFree())
					{
						m_logger.Trace("Submit() - Job immediately submitted on thread {} since it has HIGH priority.", thread.index);
						thread.SetInfo(info);
						return;
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
			case JobPriority::None:
				m_logger.Error("Submit() - Failed to submit job since it has priority type NONE {}.");
				break;
		}

		m_logger.Trace("Submit() - Job has been queued.");
	}

	void JobSystem::StoreResult(const std::function<void(void*)>& callback, const u64 dataSize, const void* data)
	{
		JobResultEntry entry {};
		entry.id = INVALID_ID_U16;
		entry.resultSize = dataSize;
		entry.callback = callback;
		if (entry.resultSize > 0)
		{
			u16 alignment = 0;
			Memory.GetAlignment(data, &alignment);

			entry.result = Memory.AllocateAligned(entry.resultSize, alignment, MemoryType::Job);
			Memory.Copy(entry.result, data, dataSize);
		}
		else
		{
			entry.result = nullptr;
		}

		// Lock our m_pendingResults
		std::lock_guard lock(m_resultMutex);

		for (u16 i = 0; i < MAX_JOB_RESULTS; i++)
		{
			if (m_pendingResults[i].id == INVALID_ID_U16)
			{
				m_pendingResults[i] = entry;
				m_pendingResults[i].id = i;
				break;
			}
		}
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
			JobInfo info;
			{
				std::lock_guard threadLock(currentThread.mutex);
				info = currentThread.GetInfo();
			}

			if (info.entryPoint)
			{
				// Call our entry point and do the work and store the result of the work
				// if the user has provided a onSuccess callback (in the case of success)
				// or if the user has provided a onFailure callback (in the case of a failure)
				m_logger.Trace("Executing job on thread #{}.", index);

				if (info.entryPoint(info.inputData, info.resultData))
				{
					if (info.onSuccess) StoreResult(info.onSuccess, info.resultDataSize, info.resultData);
				}
				else
				{
					if (info.onFailure) StoreResult(info.onFailure, info.resultDataSize, info.resultData);
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

		m_logger.Trace("Stopping job thread #{} (id={}, type={}).", index, threadId, currentThread.typeMask);
	}

	void JobSystem::ProcessQueue(RingQueue<JobInfo>& queue, std::mutex& queueMutex)
	{
		while (!queue.Empty())
		{
			const auto& jobInfo = queue.Peek();

			// Find a thread that matches the type of job and that is not currently doing any work.
			bool threadFound = false;
			for (auto& thread : m_jobThreads)
			{
				// Skip threads that don't match the type of job
				if ((thread.typeMask & jobInfo.type) == 0) continue;

				{
					// Lock our thread so we can access it
					std::lock_guard threadLock(thread.mutex);

					// If the thread is free (not doing any work we queue this job
					if (thread.IsFree())
					{
						JobInfo nextJobInfo;
						// Lock the queue
						{
							std::lock_guard queueLock(queueMutex);
							auto kaas = queue.Dequeue();
							nextJobInfo = kaas;
						}

						thread.SetInfo(nextJobInfo);
						m_logger.Trace("Assigning job to thread: '{}'", thread.index);

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
