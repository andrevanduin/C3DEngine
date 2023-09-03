
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
        ISystem(ISystem&&) = delete;

        ISystem& operator=(const ISystem&) = delete;
        ISystem& operator=(ISystem&&) = delete;

        virtual ~ISystem() = default;

        virtual void Shutdown() = 0;
        virtual void Update(const FrameData& frameData) = 0;

    protected:
        bool m_initialized = false;

        const SystemManager* m_pSystemsManager;
    };

    class BaseSystem : public ISystem
    {
    public:
        explicit BaseSystem(const SystemManager* pSystemsManager, const CString<32>& name)
            : ISystem(pSystemsManager), m_logger(name)
        {}

        virtual bool Init()
        {
            m_logger.Info("Init()");
            m_initialized = true;
            return true;
        }

        void Shutdown() override
        {
            m_logger.Info("Shutdown()");
            m_initialized = false;
        }

        void Update(const FrameData& frameData) override {}

    protected:
        LoggerInstance<32> m_logger;
    };

    template <typename T>
    class SystemWithConfig : public ISystem
    {
    public:
        explicit SystemWithConfig(const SystemManager* pSystemsManager, const CString<32>& name)
            : ISystem(pSystemsManager), m_logger(name), m_config{}
        {}

        virtual bool Init(const T& config = {})
        {
            m_logger.Info("Init()");
            m_initialized = true;
            m_config = config;
            return true;
        }

        void Shutdown() override
        {
            m_logger.Info("Shutdown()");
            m_initialized = false;
        }

        void Update(const FrameData& frameData) override {}

    protected:
        LoggerInstance<32> m_logger;
        T m_config;
    };
}  // namespace C3D
