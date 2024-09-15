
#pragma once
#include "logger/logger.h"
#include "parsers/cson_types.h"

namespace C3D
{
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
        virtual bool OnInit()     = 0;
        virtual void OnShutdown() = 0;

        virtual bool OnUpdate(const FrameData& frameData) override { return true; }
        virtual bool OnPrepareRender(FrameData& frameData) { return true; }
    };

    template <typename T>
    class SystemWithConfig : public ISystem
    {
    public:
        virtual bool OnInit(const CSONObject& config) = 0;
        virtual void OnShutdown() override            = 0;

        virtual bool OnUpdate(const FrameData& frameData) override { return true; }
        virtual bool OnPrepareRender(FrameData& frameData) override { return true; }

    protected:
        T m_config;
    };
}  // namespace C3D
