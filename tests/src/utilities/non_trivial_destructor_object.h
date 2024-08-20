
#pragma once
#include <core/defines.h>
#include <core/random.h>
#include <memory/global_memory_system.h>

namespace C3D
{
    namespace Tests
    {
        class NonTrivialDestructorObject
        {
        public:
            NonTrivialDestructorObject()
            {
                m_size = 100;
                m_data = Memory.Allocate<u32>(MemoryType::Test, m_size);
                for (u32 i = 0; i < m_size; ++i)
                {
                    m_data[i] = Random.Generate(0, 100);
                }
            }

            NonTrivialDestructorObject(const NonTrivialDestructorObject& other)
            {
                Destroy();

                if (other.m_data && other.m_size > 0)
                {
                    m_data = Memory.Allocate<u32>(MemoryType::Test, other.m_size);
                    m_size = other.m_size;
                    std::memcpy(m_data, other.m_data, m_size);
                }
            }

            NonTrivialDestructorObject& operator=(const NonTrivialDestructorObject& other)
            {
                Destroy();

                if (other.m_data && other.m_size > 0)
                {
                    m_data = Memory.Allocate<u32>(MemoryType::Test, other.m_size);
                    m_size = other.m_size;
                    std::memcpy(m_data, other.m_data, m_size);
                }
                return *this;
            }

            NonTrivialDestructorObject(NonTrivialDestructorObject&& other)
            {
                std::swap(m_data, other.m_data);
                std::swap(m_size, other.m_size);
            }

            NonTrivialDestructorObject& operator=(NonTrivialDestructorObject&& other)
            {
                std::swap(m_data, other.m_data);
                std::swap(m_size, other.m_size);
                return *this;
            }

            ~NonTrivialDestructorObject() { Destroy(); }

        private:
            void Destroy()
            {
                if (m_data)
                {
                    Memory.Free(m_data);
                    m_data = nullptr;
                    m_size = 0;
                }
            }

            u32* m_data = nullptr;
            u32 m_size  = 0;
        };
    }  // namespace Tests
}  // namespace C3D