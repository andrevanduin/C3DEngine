
#pragma once
#include "core/logger.h"

namespace C3D
{
    class SystemManager;
    struct FrameData;

    class ISystem
    {
    public:
        ISystem() = default;

        ISystem(const ISystem&) = delete;
        ISystem(ISystem&&)      = delete;

        ISystem& operator=(const ISystem&) = delete;
        ISystem& operator=(ISystem&&)      = delete;

        virtual ~ISystem() = default;

        virtual void OnShutdown()                          = 0;
        virtual bool OnUpdate(const FrameData& frameData)  = 0;
        virtual bool OnPrepareRender(FrameData& frameData) = 0;

    protected:
        bool m_initialized = false;
    };

    class BaseSystem : public ISystem
    {
    public:
        virtual bool OnInit()
        {
            INFO_LOG("SYSTEM", "Initializing.");
            m_initialized = true;
            return true;
        }

        void OnShutdown() override
        {
            INFO_LOG("SYSTEM", "Shutting down.");
            m_initialized = false;
        }

        bool OnUpdate(const FrameData& frameData) override { return true; }
        bool OnPrepareRender(FrameData& frameData) { return true; }
    };

    template <typename T>
    class SystemWithConfig : public ISystem
    {
    public:
        virtual bool OnInit(const T& config = {})
        {
            INFO_LOG("SYSTEM", "Initializing.");
            m_initialized = true;
            m_config      = config;
            return true;
        }

        void OnShutdown() override
        {
            INFO_LOG("SYSTEM", "Shutting down.");
            m_initialized = false;
        }

        bool OnUpdate(const FrameData& frameData) override { return true; }
        bool OnPrepareRender(FrameData& frameData) override { return true; }

    protected:
        T m_config;
    };
}  // namespace C3D
