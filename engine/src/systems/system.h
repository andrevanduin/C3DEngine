
#pragma once
#include "core/logger.h"

namespace C3D
{
    class SystemManager;
    struct FrameData;

    class ISystem
    {
    public:
        explicit ISystem(const SystemManager* pSystemsManager) : m_pSystemsManager(pSystemsManager) {}

        ISystem(const ISystem&) = delete;
        ISystem(ISystem&&)      = delete;

        ISystem& operator=(const ISystem&) = delete;
        ISystem& operator=(ISystem&&)      = delete;

        virtual ~ISystem() = default;

        virtual void OnShutdown()                         = 0;
        virtual void OnUpdate(const FrameData& frameData) = 0;

    protected:
        bool m_initialized = false;

        const SystemManager* m_pSystemsManager;
    };

    class BaseSystem : public ISystem
    {
    public:
        explicit BaseSystem(const SystemManager* pSystemsManager) : ISystem(pSystemsManager) {}

        virtual bool OnInit()
        {
            INSTANCE_INFO_LOG("SYSTEM", "Initializing.");
            m_initialized = true;
            return true;
        }

        void OnShutdown() override
        {
            INSTANCE_INFO_LOG("SYSTEM", "Shutting down.");
            m_initialized = false;
        }

        void OnUpdate(const FrameData& frameData) override {}
    };

    template <typename T>
    class SystemWithConfig : public ISystem
    {
    public:
        explicit SystemWithConfig(const SystemManager* pSystemsManager) : ISystem(pSystemsManager) {}

        virtual bool OnInit(const T& config = {})
        {
            INSTANCE_INFO_LOG("SYSTEM", "Initializing.");
            m_initialized = true;
            m_config      = config;
            return true;
        }

        void OnShutdown() override
        {
            INSTANCE_INFO_LOG("SYSTEM", "Shutting down.");
            m_initialized = false;
        }

        void OnUpdate(const FrameData& frameData) override {}

    protected:
        T m_config;
    };
}  // namespace C3D
