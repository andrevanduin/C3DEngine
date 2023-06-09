
#pragma once
#include "core/defines.h"
#include "renderer/render_view.h"
#include "resources/ui_text.h"
#include "containers/circular_buffer.h"
#include "containers/hash_map.h"
#include "core/function/function.h"

namespace C3D
{
	constexpr auto MAX_LINES = 512;
	constexpr i8 MAX_HISTORY = 64;
	constexpr auto SHOWN_LINES = 10;

#define Console m_engine->GetBaseConsole()

	using CommandName = CString<128>;
	using ArgName = CString<128>;
	using CommandCallback = StackFunction<bool(const DynamicArray<ArgName>&, String&), 16>;

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

		void RegisterCommand(const CommandName& name, const CommandCallback& func);

		void UnRegisterCommand(const CommandName& name);

		void WriteLine(const char* line);

		[[nodiscard]] bool IsInitialized() const;
		[[nodiscard]] bool IsOpen() const;
	private:
		void RegisterDefaultCommands() const;

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

		HashMap<CommandName, CommandCallback> m_commands;

		Engine* m_engine;
	};
}
