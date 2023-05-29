
#pragma once
#include <spdlog/sinks/base_sink.h>

#include "console.h"

namespace C3D
{
	class ConsoleSink final : public spdlog::sinks::base_sink<std::mutex>
	{
	public:
		explicit ConsoleSink(UIConsole* console);
		~ConsoleSink() override;

	protected:
		void sink_it_(const spdlog::details::log_msg& msg) override;
		void flush_() override;

	private:
		UIConsole* m_pConsole;
	};
}
