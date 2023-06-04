
#include "console_sink.h"

namespace C3D
{
	ConsoleSink::ConsoleSink(UIConsole* console) : m_pConsole(console) {}

	ConsoleSink::~ConsoleSink() = default;

	void ConsoleSink::sink_it_(const spdlog::details::log_msg& msg)
	{
		m_pConsole->WriteLine(msg.payload.begin());
	}

	// No need to do anything in these methods since it will just immediately print to in-game console
	void ConsoleSink::flush_() {}
}
