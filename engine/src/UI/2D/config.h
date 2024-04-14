
#pragma once
#include "containers/string.h"
#include "core/colors.h"
#include "core/defines.h"
#include "math/math_types.h"
#include "resources/font.h"

namespace C3D::UI_2D
{
    struct Config
    {
        vec2 position;
        u16vec2 size;
        u16vec2 cornerSize;

        FontHandle font;
        String text;

        vec4 color;
        vec4 backgroundColor;
        vec4 highlightColor;

        static Config DefaultButton()
        {
            Config config;
            config.position   = vec2(0, 0);
            config.size       = u16vec2(100, 40);
            config.cornerSize = u16vec2(8, 8);
            return config;
        }

        static Config DefaultPanel()
        {
            Config config;
            config.position        = vec2(0, 0);
            config.size            = u16vec2(100, 100);
            config.cornerSize      = u16vec2(16, 16);
            config.backgroundColor = vec4(0.05, 0.05, 0.05, 1.0);
            return config;
        }

        static Config DefaultLabel()
        {
            Config config;
            config.position = vec2(0, 0);
            config.size     = u16vec2(0, 0);
            config.color    = WHITE;
            return config;
        }

        static Config DefaultTextbox()
        {
            Config config;
            config.position        = vec2(0, 0);
            config.size            = u16vec2(80, 30);
            config.cornerSize      = u16vec2(1, 1);
            config.backgroundColor = vec4(0.02, 0.02, 0.02, 1.0);
            config.color           = WHITE;
            config.highlightColor  = vec4(0, 170.f / 255, 255.f / 255, 1);
            return config;
        }
    };

}  // namespace C3D::UI_2D