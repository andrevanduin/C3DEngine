
#pragma once
#include "core/callable.h"

#include <thread>
#include <mutex>

namespace C3D
{
	enum JobType
	{
		/* @brief A general job that does not have any specific thread requirements. */
		JobTypeGeneral = 0x2,
		/* @brief A resource loading job. Resources should be loaded from the same thread to avoid disk thrashing. */
		JobTypeResourceLoad = 0x4,
		/* @brief A job that uses GPU resources should be bound to a thread using this job type.
		 * MultiThreaded renderers will use a specific job thread and this type of job will
		 * run on this thread. For single-threaded renderers this will simply be the main thread.
		 */
		 JobTypeGpuResource = 0x8,
	};

	enum class JobPriority
	{
		/* @brief The lowest-priority job, used for things that can wait to be done (for example log flushing).*/
		Low,
		/* @brief The normal-priority job. Should be used for regular priority tasks such as loading assets. */
		Normal,
		/* @brief The highest-priority job. Should be used sparingly and only for time-critical operations. */
		High,
	};

	struct JobInfo
	{
		JobInfo()
			: type(), priority(), entryPoint(nullptr), onSuccess(nullptr), onFailure(nullptr), resultData(nullptr), resultDataSize(0)
		{}

		JobType type;
		JobPriority priority;

		/* @brief The entry point of the job. Gets called when the job starts. */
		ICallable* entryPoint;
		/* @brief An optional callback for when the job finishes successfully. */
		std::function<void(void*)> onSuccess;
		/* @brief An optional callback for when the job finishes unsuccessfully. */
		std::function<void(void*)> onFailure;
		/* @brief A pointer to optional result data. Which gets populated after the work is done. */
		void* resultData;
		/* @brief The size of the optional result data. */
		u64 resultDataSize;
	};

	class JobThread
	{
	public:
		JobThread() : index(0), typeMask(0) {}

		/* @brief Gets a copy of the info. Thread should be locked before calling this! */
		[[nodiscard]] JobInfo GetInfo() const
		{
			return m_info;
		}

		/* @brief Sets the thread's info. Thread should be locked before calling this! */
		void SetInfo(const JobInfo& info)
		{
			m_info = info;
		}
			 
		/* @brief Checks if the thread currently has any work assigned. Thread should be locked before calling this! */
		[[nodiscard]] bool IsFree() const
		{
			return m_info.entryPoint == nullptr;
		}

		u8 index;
		std::thread thread;
		std::mutex mutex;

		/* @brief The types of jobs this thread can handle. */
		u32 typeMask;

	private:
		JobInfo m_info;
	};

	struct JobResultEntry
	{
		JobResultEntry() : id(INVALID_ID_U16), result(nullptr), resultSize(0) {}

		/* @brief The id the job. */
		u16 id;
		/* @brief The result of the work that was done during this job. */
		void* result;
		/* @brief The size of the result of the work done during this job. */
		u64 resultSize;
		/* @brief The callback that we need to call (onSuccess or onFailure depending on the result) */
		std::function<void(void*)> callback;
	};
}