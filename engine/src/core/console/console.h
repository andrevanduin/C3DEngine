
#pragma once
#include "console_command.h"
#include "core/defines.h"
#include "renderer/render_view.h"
#include "resources/ui_text.h"
#include "containers/circular_buffer.h"
#include "containers/hash_map.h"

namespace C3D
{
	constexpr auto MAX_LINES = 1024;
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

		bool m_isOpen, m_initialized;
		bool m_isTextDirty, m_isEntryDirty;

		u8 m_cursorCounter, m_scrollCounter;
		u32 m_startIndex, m_endIndex, m_nextLine;

		CircularBuffer<CString<256>, MAX_LINES> m_lines;

		UIText m_text, m_entry, m_cursor;
		CString<256> m_current;

		LoggerInstance<16> m_logger;

		HashMap<CString<256>, ICommand*> m_commands;
	};
}
