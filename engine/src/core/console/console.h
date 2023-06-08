
#pragma once
#include "core/defines.h"
#include "renderer/render_view.h"
#include "resources/ui_text.h"
#include "containers/circular_buffer.h"
#include "containers/hash_map.h"
#include "core/callable/callable.h"

namespace C3D
{
	constexpr auto MAX_LINES = 512;
	constexpr i8 MAX_HISTORY = 64;
	constexpr auto SHOWN_LINES = 10;

#define Console m_engine->GetBaseConsole()

	REGISTER_CALLABLE(Command, const DynamicArray<CString<128>>&, String&)

	class C3D_API UIConsole
	{
		enum class LogType
		{
			Info,
			Error,
		};

	public:
		UIConsole();

		void OnInit(Engine* engine);
		void OnShutDown();

		void OnUpdate();
		void OnRender(UIPacketData& packet);

		void RegisterCommand(const CString<128>& name, CommandCallable* func);

		void UnRegisterCommand(const CString<128>& name);

		void WriteLine(const char* line);

		[[nodiscard]] bool IsInitialized() const;
		[[nodiscard]] bool IsOpen() const;

		static bool StaticShutdownCommand(int& jan, const float& bert);
		bool ShutdownCommand(const DynamicArray<CString<128>>& args, String& output) const;

	private:
		void WriteLineInternal(const CString<256>& line);

		bool OnKeyDownEvent(u16 code, void* sender, const EventContext& context);
		bool OnMouseScrollEvent(u16 code, void* sender, const EventContext& context);

		bool OnParseCommand();

		template <typename... Args>
		void PrintCommandMessage(const LogType type, const char* format, Args&&... args)
		{
			if (type == LogType::Info) m_logger.Info(format, args...);
			else m_logger.Error(format, args...);

			m_current = "";
			m_isEntryDirty = true;
		}

		bool m_isOpen, m_initialized;
		bool m_isTextDirty, m_isEntryDirty;

		u8 m_cursorCounter, m_scrollCounter;
		u32 m_startIndex, m_endIndex, m_nextLine;

		// History
		i8 m_currentHistory;
		i8 m_endHistory, m_nextHistory;

		CircularBuffer<CString<256>, MAX_LINES> m_lines;
		CircularBuffer<CString<256>, MAX_HISTORY> m_history;

		UIText m_text, m_entry, m_cursor;
		CString<256> m_current;

		LoggerInstance<16> m_logger;

		HashMap<CString<128>, CommandCallable*> m_commands;

		Engine* m_engine;
	};
}
