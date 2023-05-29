
#pragma once
#include "defines.h"
#include "renderer/render_view.h"
#include "resources/ui_text.h"

namespace C3D
{
	constexpr auto MAX_LINES = 2048;

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

		CString<256>& GetCurrentLine();

		std::mutex m_mutex;

		bool m_isOpen, m_isDirty, m_initialized;

		u32 m_startIndex, m_endIndex, m_nextLine, m_usedLines;
		Array<CString<256>, MAX_LINES> m_lines;
		UIText m_text, m_entry;

		LoggerInstance<16> m_logger;
	};
}
