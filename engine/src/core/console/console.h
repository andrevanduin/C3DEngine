
#pragma once
#include "console_command.h"
#include "core/defines.h"
#include "renderer/render_view.h"
#include "resources/ui_text.h"
#include "containers/circular_buffer.h"
#include "containers/hash_map.h"

namespace C3D
{
	constexpr auto MAX_LINES = 512;
	constexpr auto MAX_HISTORY = 64;
	constexpr auto SHOWN_LINES = 10;

#define Console C3D::UIConsole::GetBaseConsole()

	class C3D_API UIConsole
	{
	public:
		UIConsole();

		void OnInit();
		void OnShutDown();

		void OnUpdate();
		void OnRender(UIPacketData& packet);

		void RegisterCommand(const CString<128>& name, pStaticCommandFunc function);

		template <class T>
		void RegisterCommand(const CString<128>& name, T* classPtr, pCommandFunc<T> function)
		{
			const auto command = Memory.New<InstanceCommand<T>>(MemoryType::DebugConsole, name, classPtr, function);

			m_commands.Set(name, command);
			m_logger.Info("Registered command: \'{}\'", name);
		}

		void UnRegisterCommand(const CString<128>& name);

		void WriteLine(const char* line);

		[[nodiscard]] bool IsInitialized() const;
		[[nodiscard]] bool IsOpen() const;

		static UIConsole* GetBaseConsole()
		{
			static UIConsole console;
			return &console;
		}

	private:
		void WriteLineInternal(const CString<256>& line);

		bool OnKeyDownEvent(u16 code, void* sender, const EventContext& context);
		bool OnMouseScrollEvent(u16 code, void* sender, const EventContext& context);

		bool OnParseCommand();

		template <typename... Args>
		void PrintCommandMessage(const char* format, Args&&... args)
		{
			CString<256> warning;
			warning.FromFormat(format, args...);

			WriteLineInternal(warning);
			m_current = "";
			m_isEntryDirty = true;
		}

		bool m_isOpen, m_initialized;
		bool m_isTextDirty, m_isEntryDirty;

		u8 m_cursorCounter, m_scrollCounter;
		u32 m_startIndex, m_endIndex, m_nextLine;

		CircularBuffer<CString<256>, MAX_LINES> m_lines;
		CircularBuffer<CString<256>, MAX_HISTORY> m_history;

		UIText m_text, m_entry, m_cursor;
		CString<256> m_current;

		LoggerInstance<16> m_logger;

		HashMap<CString<128>, ICommand*> m_commands;
	};
}
