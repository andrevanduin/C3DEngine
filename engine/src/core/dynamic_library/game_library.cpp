
#include "game_library.h"

namespace C3D
{
	GameLibrary::GameLibrary() : DynamicLibrary("GAME_LIB") {}

	ApplicationState* GameLibrary::CreateState()
	{
		const auto createStateFunc = LoadFunction<ApplicationState * (*)()>("CreateApplicationState");
		if (!createStateFunc)
		{
			m_logger.Error("Create() - Failed to load CreateState function for: '{}'", m_name);
			return nullptr;
		}

		const auto state = createStateFunc();
		if (!state)
		{
			m_logger.Error("Create() - Failed to create state for: '{}'", m_name);
			return nullptr;
		}

		return state;
	}


	Application* GameLibrary::Create(ApplicationState* state)
	{
		const auto createApplicationFunc = LoadFunction<Application* (*)(ApplicationState*)>("CreateApplication");
		if (!createApplicationFunc) {
			m_logger.Error("Create() - Failed to load CreateApplication function for: '{}'", m_name);
			return nullptr;
		}
		
		return createApplicationFunc(state);
	}
}
