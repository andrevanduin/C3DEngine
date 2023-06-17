
#ifdef C3D_PLATFORM_WINDOWS
#include "platform_win32.h"
#include "containers/cstring.h"
#include "core/engine.h"
#include "core/events/event_context.h"
#include "systems/events/event_system.h"

namespace C3D
{
	Platform::Platform()
		: BaseSystem(nullptr, "PLATFORM"), m_clockFrequency(0), m_startTime(0)
	{}

	Platform::Platform(const Engine* engine)
		: BaseSystem(engine, "PLATFORM"), m_clockFrequency(0), m_startTime(0)
	{}

	bool Platform::Init()
	{
		m_logger.Info("Init()");

		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);

		m_clockFrequency = 1.0 / static_cast<f64>(frequency.QuadPart);

		LARGE_INTEGER startTime;
		QueryPerformanceCounter(&startTime);

		m_startTime = startTime.QuadPart;

		m_initialized = true;
		return true;
	}

	void Platform::Shutdown()
	{
		// Unwatch all files that we are currently watching
		for (const auto& watch : m_fileWatches)
		{
			if (watch.id != INVALID_ID)
			{
				UnwatchFile(watch.id);
			}
		}
		m_fileWatches.Destroy();
	}

	CopyFileStatus Platform::CopyFile(const char* source, const char* dest, const bool overwriteIfExists = true)
	{
		if (const auto result = CopyFileA(source, dest, !overwriteIfExists); !result)
		{
			const auto error = GetLastError();
			if (error == ERROR_FILE_NOT_FOUND)
			{
				return CopyFileStatus::NotFound;
			}

			if (error == ERROR_SHARING_VIOLATION)
			{
				return CopyFileStatus::Locked;
			}

			return CopyFileStatus::Unknown;
		}

		return CopyFileStatus::Success;
	}

	FileWatchId Platform::WatchFile(const char* filePath)
	{
		if (!filePath)
		{
			m_logger.Error("WatchFile() - Failed due to filePath being invalid");
			return INVALID_ID;
		}

		WIN32_FIND_DATAA data = {};
		const auto fileHandle = FindFirstFileA(filePath, &data);
		if (fileHandle == INVALID_HANDLE_VALUE)
		{
			m_logger.Error("WatchFile() - Could not find file at: '{}'", filePath);
			return INVALID_ID;
		}

		if (!FindClose(fileHandle))
		{
			m_logger.Error("WatchFile() - Could not close file at: '{}'", filePath);
			return INVALID_ID;
		}

		for (u32 i = 0; i <  m_fileWatches.Size(); i++)
		{
			auto& watch = m_fileWatches[i];

			if (watch.id == INVALID_ID)
			{
				// We have found an empty slot so we put the FileWatch in this slot and set the id to it's index
				watch.id = i;
				watch.filePath = filePath;
				watch.lastWriteTime = data.ftLastWriteTime;

				m_logger.Info("WatchFile() - Registered watch for: '{}'", filePath);
				return i;
			}
		}

		// We were unable to find an empty slot so let's add a new slot
		const auto nextIndex = static_cast<u32>(m_fileWatches.Size());

		const Win32FileWatch watch = { nextIndex, filePath, data.ftLastWriteTime };
		m_fileWatches.PushBack(watch);

		m_logger.Info("WatchFile() - Registered watch for: '{}'", filePath);
		return nextIndex;
	}

	bool Platform::UnwatchFile(const FileWatchId watchId)
	{
		if (watchId == INVALID_ID)
		{
			m_logger.Error("UnwatchFile() - Failed due to watchId being invalid");
			return false;
		}

		if (m_fileWatches.Empty())
		{
			m_logger.Error("UnwatchFile() - Failed since there are no files being watched currently");
			return false;
		}

		if (watchId >= m_fileWatches.Size())
		{
			m_logger.Error("UnwatchFile() - Failed since there is no watch for the provided id: {}", watchId);
			return false;
		}

		// Set the id to INVALID_ID to indicate that we no longer are interested in this watch
		// This makes the slot available to be filled by a different FileWatch in the future
		Win32FileWatch& watch = m_fileWatches[watchId];

		m_logger.Info("UnwatchFile() - Stopped watching: '{}'", watch.filePath);

		watch.id = INVALID_ID;
		watch.filePath.Clear();
		std::memset(&watch.lastWriteTime, 0, sizeof(FILETIME));

		return true;
	}

	void Platform::WatchFiles()
	{
		for (auto& watch : m_fileWatches)
		{
			if (watch.id != INVALID_ID)
			{
				WIN32_FIND_DATAA data;
				const HANDLE fileHandle = FindFirstFileA(watch.filePath.Data(), &data);
				if (fileHandle == INVALID_HANDLE_VALUE)
				{
					// File was not found
					EventContext context = {};
					context.data.u32[0] = watch.id;
					// Fire a watched file removed event
					Event.Fire(EventCodeWatchedFileRemoved, nullptr, context);
					// Unwatch the file since it no longer exists
					UnwatchFile(watch.id);
					continue;
				}

				if (!FindClose(fileHandle)) continue;

				// Check the time the file was last edited to see if it has been changed
				if (CompareFileTime(&watch.lastWriteTime, &data.ftLastWriteTime) != 0)
				{
					// File has been changed since last time
					watch.lastWriteTime = data.ftLastWriteTime;
					EventContext context = {};
					context.data.u32[0] = watch.id;
					// Fire the watched file changed event
					Event.Fire(EventCodeWatchedFileChanged, nullptr, context);
				}
			}
		}
	}

	f64 Platform::GetAbsoluteTime() const
	{
		LARGE_INTEGER nowTime;
		QueryPerformanceCounter(&nowTime);
		return static_cast<f64>(nowTime.QuadPart) * m_clockFrequency;
	}

	void Platform::SleepMs(const u64 ms)
	{
		Sleep(static_cast<u32>(ms));
	}

	i32 Platform::GetProcessorCount()
	{
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		return static_cast<i32>(sysInfo.dwNumberOfProcessors);
	}

	u64 Platform::GetThreadId()
	{
		return GetCurrentThreadId();
	}

	bool Platform::LoadDynamicLibrary(const char* name, void** libraryData, u64& size)
	{
		if (!name)
		{
			printf("[PLATFORM] - LoadDynamicLibrary() Failed - Please provide a valid name");
			return false;
		}

		CString<128> path(name);
		path += ".dll";

		HMODULE library = LoadLibraryA(path.Data());
		if (!library)
		{
			const auto errorMsg = GetLastErrorMsg();
			printf("[PLATFORM] - LoadDynamicLibrary() Failed - %s", errorMsg.data());
			return false;
		}

		*libraryData = library;
		size = sizeof(HMODULE);
		return true;
	}

	bool Platform::UnloadDynamicLibrary(void* libraryData)
	{
		const auto library = static_cast<HMODULE>(libraryData);
		if (!library)
		{
			printf("[PLATFORM] - UnloadDynamicLibrary() Failed - Please provide a valid library");
			return false;
		}

		if (FreeLibrary(library) == 0)
		{
			const auto errorMsg = GetLastErrorMsg();
			printf("[PLATFORM] - UnloadDynamicLibrary() Failed - %s", errorMsg.data());
			return false;
		}

		return true;
	}
	
	std::string Platform::GetLastErrorMsg()
	{
		const auto errorCode = GetLastError();
		if (errorCode == 0)
		{
			// No Error has been reported
			return "NONE";
		}

		// Pointer to buffer holding our message
		LPSTR messageBuffer = nullptr;

		// Let the win32 API allocate and populate our message buffer
		const auto size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
			errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&messageBuffer), 0, nullptr);

		// Copy the message into a String
		std::string msg(messageBuffer, size);

		// Free our allocated buffer so we don't leak
		LocalFree(messageBuffer);

		return msg;
	}
}
#endif