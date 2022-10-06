
#pragma once
#include <thread>
#include <mutex>

#include "core/memory.h"
#include "services/services.h"

namespace C3D
{
	enum JobType
	{
		JobTypeNone = 0x0,
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
		None,
		/* @brief The lowest-priority job, used for things that can wait to be done (for example log flushing).*/
		Low,
		/* @brief The normal-priority job. Should be used for regular priority tasks such as loading assets. */
		Normal,
		/* @brief The highest-priority job. Should be used sparingly and only for time-critical operations. */
		High,
	};

	class JobInfo
	{
	public:

		JobInfo();
		JobInfo(JobType type, JobPriority priority);

		JobInfo(const JobInfo& other);
		JobInfo(JobInfo&& other) noexcept;

		JobInfo& operator=(const JobInfo& other);
		JobInfo& operator=(JobInfo&& other) noexcept;

		~JobInfo();

		/* @brief Store the input data (Allocates space and makes a copy of your provided data internally).
		 * This version of the method should be used if you are not expecting result data.
		 */
		template <typename InputType>
		void SetData(InputType* data);

		/* @brief Store the input data and space for the output data (Allocates space and makes a copy of your provided data internally).
		 * This version of the method should be used if you are expecting result data.
		 */
		template <typename InputType, typename OutputType>
		void SetData(InputType* data);

		void Clear();

		JobType type;
		JobPriority priority;

		/* @brief The entry point of the job. Gets called when the job starts. */
		std::function<bool(void* , void*)> entryPoint;
		/* @brief An optional callback for when the job finishes successfully. */
		std::function<void(void*)> onSuccess;
		/* @brief An optional callback for when the job finishes unsuccessfully. */
		std::function<void(void*)> onFailure;

		/* @brief A pointer to input data. Which gets passed to the entry point.  */
		void* inputData;
		/* @brief The size of the optional input data. */
		u64 inputDataSize;

		/* @brief A pointer to optional result data. Which gets populated after the work is done. */
		void* resultData;
		/* @brief The size of the optional result data. */
		u64 resultDataSize;

	private:
		void FreeMemory();
		void Copy();
	};

	template <typename InputType>
	void JobInfo::SetData(InputType* data)
	{
		inputDataSize = sizeof(InputType);
		inputData = Memory.Allocate<InputType>(MemoryType::Job);
		Memory.Copy(inputData, data, inputDataSize);

		resultDataSize = 0;
	}

	template <typename InputType, typename OutputType>
	void JobInfo::SetData(InputType* data)
	{
		inputDataSize = sizeof(InputType);
		inputData = Memory.Allocate<InputType>(MemoryType::Job);
		Memory.Copy(inputData, data, inputDataSize);

		resultDataSize = sizeof(OutputType);
		resultData = Memory.Allocate<OutputType>(MemoryType::Job);
	}

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

		/* @brief Clears the thread's info. Thread should be locked before calling this! */
		void ClearInfo()
		{
			m_info.Clear();
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

	class JobResultEntry
	{
	public:
		JobResultEntry();
		JobResultEntry(const JobResultEntry& other) = delete;
		JobResultEntry(JobResultEntry&& other) = delete;

		JobResultEntry& operator=(const JobResultEntry& other);
		JobResultEntry& operator=(JobResultEntry&& other) = delete;

		~JobResultEntry();

		void Clear();

		/* @brief The id the job. */
		u16 id;
		/* @brief The result of the work that was done during this job. */
		void* result;
		/* @brief The size of the result of the work done during this job. */
		u64 resultSize;
		/* @brief The callback that we need to call (onSuccess or onFailure depending on the result) */
		std::function<void(void*)> callback;

	private:
		void Free();
	};
}
