
#pragma once
#include "containers/dynamic_array.h"
#include "containers/string.h"
#include "core/defines.h"
#include "flags.h"
#include "math/math_types.h"

namespace C3D
{
    struct FrameData;

    namespace UI_2D
    {
        class UI2DSystem;

        class Control
        {
        public:
            bool Create();
            void Destroy();

            bool Load();
            bool Unload();

            bool Update(const FrameData& frameData);
            bool Render(const FrameData& frameData);

            bool HasFlag(FlagBit flag) const { return m_flags & flag; }
            void Activate() { m_flags |= FlagBit::FlagActive; }

            u32 GetId() const { return m_id; }
            const String& GetName() const { return m_name; }

        private:
            u32 m_id = INVALID_ID;
            vec2 m_position;
            String m_name;
            Flags m_flags = FlagNone;

            DynamicArray<Control> m_children;
        };

    }  // namespace UI_2D
}  // namespace C3D