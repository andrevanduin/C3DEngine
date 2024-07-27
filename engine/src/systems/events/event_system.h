
#pragma once
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "core/events/event_context.h"
#include "core/function/function.h"
#include "core/logger.h"
#include "systems/system.h"

namespace C3D
{
    constexpr auto MAX_MESSAGE_CODES = 4096;

    // TODO: Replace with fancy StackFunction<bool(u16, void*, const EventContext&), 16> after it's properly implemented
    using EventCallbackFunc = std::function<bool(u16, void*, const EventContext&)>;
    using EventCallbackId   = u16;

#define INVALID_CALLBACK std::numeric_limits<u16>::max()

    struct RegisteredEventCallback
    {
        u16 code           = INVALID_ID_U16;
        EventCallbackId id = INVALID_ID_U16;
    };

    class C3D_API EventSystem final : public BaseSystem
    {
        struct EventCallback
        {
            EventCallback(const EventCallbackId id, EventCallbackFunc func) : id(id), func(std::move(func)) {}

            EventCallbackId id;
            EventCallbackFunc func;
        };

        struct EventCodeEntry
        {
            DynamicArray<EventCallback> events;
        };

    public:
        void OnShutdown() override;

        RegisteredEventCallback Register(u16 code, const EventCallbackFunc& callback);
        bool Unregister(u16 code, EventCallbackId& callbackId);
        bool Unregister(RegisteredEventCallback callback);

        bool UnregisterAll(u16 code);

        bool Fire(u16 code, void* sender, const EventContext& data) const;

    private:
        EventCodeEntry m_registered[MAX_MESSAGE_CODES] = {};
    };
}  // namespace C3D
