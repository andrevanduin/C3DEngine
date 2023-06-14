
#pragma once
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "systems/system.h"

#ifdef C3D_PLATFORM_WINDOWS
	#include "win32/platform_types_win32.h"
#endif

namespace C3D
{
	struct DynamicLibraryFunction;
	using DynamicLibraryExtension = CString<8>;
	using FileWatchId = u32;

	class C3D_API Platform final : public BaseSystem
	{
	public:
		Platform();
		explicit Platform(const Engine* engine);

		bool Init() override;

		void Shutdown() override;

		/* @brief Copies a file from source to dest.
		 *
		 *  @param Source - the path to the file we want to copy
		 *	@param Dest   - the path to where we want the file to be copied
		 *	@param OverwriteIfExists - Set to true if you want to overwrite the file in the dest if it exists
		 *	@return True if successfully copied file false otherwise
		 */
		bool CopyFile(const char* source, const char* dest, bool overwriteIfExists) const;

		/* @brief Starts watching the file at the provided filePath for changes.
		 *
		 * @param FilePath - the path the the file you want to watch
		 * @return The id for the watched file if successful, otherwise INVALID_ID
		 */
		FileWatchId WatchFile(const char* filePath);

		/* @brief Stops watching the file at with the provided FileWatchId.
		 *
		 * @param FileWatchId - The id for the file that you no longer want to watch
		 * @return True if successful, otherwise false
		 */
		bool UnwatchFile(FileWatchId watchId);

		/* @brief - Check all the watched files again for changes. */
		void WatchFiles();

		/* @brief Get the systems absolute time. */
		[[nodiscard]] f64 GetAbsoluteTime() const;

		/* @brief Sleeps the current thread for the provided amount of ms. */
		static void SleepMs(u64 ms);

		/* @brief Obtains the number of logical processor cores. */
		static i32 GetProcessorCount();

		/* @brief Obtains the current thread's id. */
		static u64 GetThreadId();

		/* @brief Loads a dynamic library by name into memory.
		 *
		 * @param Name - The name of the dynamic library you want to load
		 * @param LibraryData - A pointer to a void pointer that can accept the data
		 * @param Size - A reference to a u64 that can hold the size of the provided data
		 * @return True if successful, false otherwise
		 */
		static bool LoadDynamicLibrary(const char* name, void** libraryData, u64& size);

		/* @brief Unloads a dynamic library from memory.
		 *
		 * @param LibraryData - A void pointer to the library data
		 * @return True if successful, false otherwise
		 */
		static bool UnloadDynamicLibrary(void* libraryData);

		/* @brief Loads a function inside a loaded dynamic library.
		 *
		 * @param Name - The name of the function you want to load
		 * @param LibraryData - A void pointer to the loaded library data
		 * @return True if successful, false otherwise
		 */
		template <typename Func>
		static Func LoadDynamicLibraryFunction(const char* name, void* libraryData)
#ifdef C3D_PLATFORM_WINDOWS
		{
			if (!name || !libraryData)
			{
				printf("[PLATFORM] - LoadDynamicLibraryFunction() Failed - Please provide valid data");
				return nullptr;
			}

			const auto library = static_cast<HMODULE>(libraryData);
			const FARPROC funcAddress = GetProcAddress(library, name);
			if (!funcAddress)
			{
				const auto errorMsg = GetLastErrorMsg();
				printf("[PLATFORM] - LoadDynamicLibraryFunction() Failed - %s", errorMsg.data());
				return nullptr;
			}

			return reinterpret_cast<Func>(funcAddress);
		}
#endif

		/* @brief Get the extension that is used for dynamic libraries on the current platform. */
		constexpr static DynamicLibraryExtension GetDynamicLibraryExtension();

	private:
#ifdef C3D_PLATFORM_WINDOWS

		static std::string GetLastErrorMsg();


		f64 m_clockFrequency;
		u64 m_startTime;

		DynamicArray<Win32FileWatch> m_fileWatches;
#endif
	};
}
