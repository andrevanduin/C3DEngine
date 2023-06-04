
#pragma once
#include <thread>
#include <mutex>

#include "memory/global_memory_system.h"
#include "systems/system_manager.h"

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

	class BaseJobResultEntry
	{
	public:
		BaseJobResultEntry() : id(INVALID_ID_U16) {}

		BaseJobResultEntry(const BaseJobResultEntry&) = default;
		BaseJobResultEntry(BaseJobResultEntry&&) = default;

		BaseJobResultEntry& operator=(const BaseJobResultEntry&) = default;
		BaseJobResultEntry& operator=(BaseJobResultEntry&&) = default;

		virtual ~BaseJobResultEntry() = default;

		virtual void Callback() = 0;

		virtual BaseJobResultEntry* MakeCopy() = 0;

		/* @brief The id the job. */
		u16 id;
		/* @brief The callback that we need to call (onSuccess or onFailure depending on the result) */
		std::function<void(void*)> callback = nullptr;
	};

	template <typename ResultType>
	class JobResultEntry final : public BaseJobResultEntry
	{
	public:
		JobResultEntry() : BaseJobResultEntry() {}

		void Callback() override
		{
			callback(&result);
		}

		BaseJobResultEntry* MakeCopy() override
		{
			return Memory.New<JobResultEntry<ResultType>>(MemoryType::Job, *this);
		}

		/* @brief The result of the work that was done during this job. */
		ResultType result;
	};

	class BaseJobInfo
	{
	public:
		BaseJobInfo(JobType type, JobPriority priority);

		BaseJobInfo(const BaseJobInfo&) = default;
		BaseJobInfo(BaseJobInfo&&) = default;

		BaseJobInfo& operator=(const BaseJobInfo&) = default;
		BaseJobInfo& operator=(BaseJobInfo&&) = default;

		virtual ~BaseJobInfo() = default;

		virtual bool CallEntry() = 0;
		virtual BaseJobInfo* MakeCopy() = 0;
		virtual BaseJobResultEntry* MakeResultEntry(bool wasSuccess) = 0;

		JobType type;
		JobPriority priority;

		/* @brief The entry point of the job. Gets called when the job starts. */
		std::function<bool(void*, void*)> entryPoint;
		/* @brief An optional callback for when the job finishes successfully. */
		std::function<void(void*)> onSuccess;
		/* @brief An optional callback for when the job finishes unsuccessfully. */
		std::function<void(void*)> onFailure;
	};

	template <typename InputType, typename OutputType>
	class JobInfo final : public BaseJobInfo
	{
	public:
		JobInfo() : BaseJobInfo(JobTypeGeneral, JobPriority::Normal) {}

		JobInfo(const JobType type, const JobPriority priority) : BaseJobInfo(type, priority) {}

		void SetData(const InputType& data)
		{
			input = data;
		}

		bool CallEntry() override
		{
			return entryPoint(&input, &output);
		}

		BaseJobInfo* MakeCopy() override
		{
			return Memory.New<JobInfo<InputType, OutputType>>(MemoryType::Job, *this);
		}

		BaseJobResultEntry* MakeResultEntry(const bool wasSuccess) override
		{
			auto entry = Memory.New<JobResultEntry<OutputType>>(MemoryType::Job);
			entry->id = INVALID_ID_U16;
			entry->callback = wasSuccess ? onSuccess : onFailure;
			entry->result = output;
			return entry;
		}

		InputType input;
		OutputType output;
	};

	class JobThread
	{
	public:
		JobThread() : index(0), typeMask(0), m_info(nullptr) {}

		[[nodiscard]] BaseJobInfo* CopyInfo() const
		{
			if (!m_info) return nullptr;
			return m_info->MakeCopy();
		}

		/* @brief Sets the thread's info. Thread should be locked before calling this! */
		void SetInfo(BaseJobInfo* info)
		{
			m_info = info;
		}

		/* @brief Clears the thread's info. Thread should be locked before calling this! */
		void ClearInfo()
		{
			if (m_info)
			{
				// Free our JobInfo memory that was allocated when we submitted this job
				Memory.Delete(MemoryType::Job, m_info);
				// Set our m_info to nullptr so it's explicitly empty
				m_info = nullptr;
			}
		}
			 
		/* @brief Checks if the thread currently has any work assigned. Thread should be locked before calling this! */
		[[nodiscard]] bool IsFree() const
		{
			return m_info == nullptr;
		}

		u8 index;
		std::thread thread;
		std::mutex mutex;

		/* @brief The types of jobs this thread can handle. */
		u32 typeMask;

	private:
		BaseJobInfo* m_info;
	};
}
