
#include "job.h"

namespace C3D
{
	JobInfo::JobInfo()
		: type(JobTypeGeneral), priority(JobPriority::Normal), entryPoint(nullptr), onSuccess(nullptr), onFailure(nullptr),
		inputData(nullptr), inputDataSize(0), resultData(nullptr), resultDataSize(0)
	{}

	JobInfo::JobInfo(const JobType type, const JobPriority priority)
		: type(type), priority(priority), entryPoint(nullptr), onSuccess(nullptr), onFailure(nullptr),
		inputData(nullptr), inputDataSize(0), resultData(nullptr), resultDataSize(0)
	{}

	JobInfo::JobInfo(const JobInfo& other)
		: type(other.type), priority(other.priority), entryPoint(other.entryPoint), onSuccess(other.onSuccess),
	      onFailure(other.onFailure), inputData(nullptr), inputDataSize(other.inputDataSize), resultData(nullptr), resultDataSize(other.resultDataSize)
	{
		if (inputDataSize)
		{
			u16 alignment = 0;
			Memory.GetAlignment(other.inputData, &alignment);

			inputData = Memory.AllocateAligned(inputDataSize, alignment, MemoryType::Job);
			Memory.Copy(inputData, other.inputData, inputDataSize);
		}
		if (resultDataSize)
		{
			u16 alignment = 0;
			Memory.GetAlignment(other.resultData, &alignment);

			resultData = Memory.AllocateAligned(resultDataSize, alignment, MemoryType::Job);
			Memory.Copy(resultData, other.resultData, resultDataSize);
		}
	}

	JobInfo::JobInfo(JobInfo&& other) noexcept
		: type(other.type), priority(other.priority), entryPoint(std::move(other.entryPoint)),
		  onSuccess(std::move(other.onSuccess)), onFailure(std::move(other.onFailure))
	{
		// Take the inputData pointer from other
		inputData = other.inputData;
		// Set other inputData to nullptr so we don't double free
		other.inputData = nullptr;

		inputDataSize = other.inputDataSize;
		other.inputDataSize = 0;

		// Take the resultData pointer from other
		resultData = other.resultData;
		// Set other resultData to nullptr so we don't double free
		other.resultData = nullptr;

		resultDataSize = other.resultDataSize;
		other.resultDataSize = 0;
	}

	JobInfo& JobInfo::operator=(const JobInfo& other)
	{
		if (this == &other) return *this;

		type = other.type;
		priority = other.priority;
		entryPoint = other.entryPoint;
		onSuccess = other.onSuccess;
		onFailure = other.onFailure;

		// Ensure that if we had any memory allocated we free it first
		FreeMemory();

		// Assign the correct sizes
		inputDataSize = other.inputDataSize;
		resultDataSize = other.resultDataSize;

		// If other has any input data we allocate memory for it and copy
		if (inputDataSize)
		{
			u16 alignment = 0;
			Memory.GetAlignment(other.inputData, &alignment);

			inputData = Memory.AllocateAligned(inputDataSize, alignment, MemoryType::Job);
			Memory.Copy(inputData, other.inputData, inputDataSize);
		}
		// If other has any result data we allocate memory for it and copy
		if (resultDataSize)
		{
			u16 alignment = 0;
			Memory.GetAlignment(other.resultData, &alignment);

			resultData = Memory.AllocateAligned(resultDataSize, alignment, MemoryType::Job);
			Memory.Copy(resultData, other.resultData, resultDataSize);
		}
		return *this;
	}

	JobInfo& JobInfo::operator=(JobInfo&& other) noexcept
	{
		if (this == &other) return *this;

		// Swap all values
		std::swap(type, other.type);
		std::swap(priority, other.priority);
		std::swap(entryPoint, other.entryPoint);
		std::swap(onSuccess, other.onSuccess);
		std::swap(onFailure, other.onFailure);

		// Take the inputData pointer from other
		inputData = other.inputData;
		// Set other inputData to nullptr so we don't double free
		other.inputData = nullptr;

		inputDataSize = other.inputDataSize;
		other.inputDataSize = 0;

		// Take the resultData pointer from other
		resultData = other.resultData;
		// Set other resultData to nullptr so we don't double free
		other.resultData = nullptr;

		resultDataSize = other.resultDataSize;
		other.resultDataSize = 0;
		
		return *this;
	}

	JobInfo::~JobInfo()
	{
		Clear();
	}

	void JobInfo::Clear()
	{
		type = JobTypeNone;
		priority = JobPriority::None;
		entryPoint = nullptr;
		onSuccess = nullptr;
		onFailure = nullptr;
		FreeMemory();
	}

	void JobInfo::FreeMemory()
	{
		if (inputData)
		{
			Memory.Free(inputData, inputDataSize, MemoryType::Job);
			inputData = nullptr;
			inputDataSize = 0;
		}
		if (resultData)
		{
			Memory.Free(resultData, resultDataSize, MemoryType::Job);
			resultData = nullptr;
			resultDataSize = 0;
		}
	}

	JobResultEntry::JobResultEntry()
		: id(INVALID_ID_U16), result(nullptr), resultSize(0)
	{}

	JobResultEntry& JobResultEntry::operator=(const JobResultEntry& other)
	{
		// Prevent self-assignment bugs
		if (this == &other) return *this;

		id = other.id;
		callback = other.callback;

		// Ensure that if we have any allocations open we free them
		Free();

		// Copy the result from other if there is any
		resultSize = other.resultSize;
		if (other.result && other.resultSize)
		{
			u16 alignment = 0;
			Memory.GetAlignment(other.result, &alignment);

			result = Memory.AllocateAligned(resultSize, alignment, MemoryType::Job);
			Memory.Copy(result, other.result, resultSize);
		}

		return *this;
	}

	JobResultEntry::~JobResultEntry()
	{
		Clear();
	}

	void JobResultEntry::Clear()
	{
		id = INVALID_ID_U16;
		callback = nullptr;
		Free();
	}

	void JobResultEntry::Free()
	{
		if (result && resultSize)
		{
			Memory.Free(result, resultSize, MemoryType::Job);
			resultSize = 0;
		}
	}
}
