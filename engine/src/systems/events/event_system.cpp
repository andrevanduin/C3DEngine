
#include "event_system.h"

#include <algorithm>

#include "core/logger.h"

namespace C3D
{
    EventSystem::EventSystem(const SystemManager* pSystemsManager) : BaseSystem(pSystemsManager, "EVENT"), m_registered{} {}

    bool EventSystem::Init()
    {
        m_initialized = true;
        return true;
    }

    void EventSystem::Shutdown()
    {
        m_logger.Info("Shutting Down");
        for (auto& [events] : m_registered)
        {
            if (!events.Empty()) events.Clear();
        }
    }

    RegisteredEventCallback EventSystem::Register(const u16 code, const EventCallbackFunc& callback)
    {
        static EventCallbackId UNIQUE_ID = 0;

        auto& events = m_registered[code].events;
        events.EmplaceBack(UNIQUE_ID, callback);

        RegisteredEventCallback cb = { code, UNIQUE_ID++ };
        return cb;
    }

    bool EventSystem::Unregister(const u16 code, EventCallbackId& callbackId)
    {
        if (code > MAX_MESSAGE_CODES)
        {
            m_logger.Warn("Unregister() - Tried to Unregister Event for invalid code: '{}'", code);
            return false;
        }

        auto& events = m_registered[code].events;
        if (events.Empty())
        {
            m_logger.Warn("Unregister() - Tried to Unregister Event for code: '{}' that has no events", code);
            return false;
        }

        const auto it = std::ranges::find_if(events, [&](const EventCallback& e) { return e.id == callbackId; });
        if (it != events.end())
        {
            events.Erase(it);
            callbackId = INVALID_ID_U16;
            return true;
        }

        m_logger.Warn("Unregister() - Tried to Unregister Event that did not exist");
        return false;
    }

    bool EventSystem::Unregister(RegisteredEventCallback callback) { return Unregister(callback.code, callback.id); }

    bool EventSystem::UnregisterAll(u16 code)
    {
        auto& events = m_registered[code].events;
        if (events.Empty())
        {
            m_logger.Warn("UnRegisterAll() - Tried to UnRegister all Events for code: '{}' that has no events", code);
            return false;
        }

        events.Clear();
        return true;
    }

    bool EventSystem::Fire(const u16 code, void* sender, const EventContext& data) const
    {
        if (!m_initialized) return true;

        const auto& events = m_registered[code].events;
        if (events.Empty())
        {
            return false;
        }

        return std::ranges::any_of(events, [&](const EventCallback& e) { return e.func(code, sender, data); });
    }
}  // namespace C3D
