#pragma once
#include "core/defines.h"
#include "math/c3d_math.h"

namespace C3D
{
    constexpr auto HSV_60  = 60.0f / 360.0f;
    constexpr auto HSV_120 = 120.0f / 360.0f;
    constexpr auto HSV_180 = 180.0f / 360.0f;
    constexpr auto HSV_240 = 240.0f / 360.0f;
    constexpr auto HSV_300 = 300.0f / 360.0f;

    struct RGB
    {
        f32 r;
        f32 g;
        f32 b;
    };

    struct RGBA
    {
        f32 r;
        f32 g;
        f32 b;
        f32 a;
    };

    struct HSV
    {
        HSV(u32 h, u32 s, u32 v) : h(h / 360.0f), s(s / 100.0f), v(v / 100.0f) {}
        HSV(f32 h, f32 s, f32 v) : h(h), s(s), v(v) {}

        f32 h = 0.0f;
        f32 s = 0.0f;
        f32 v = 0.0f;
    };

    C3D_API C3D_INLINE u32 RgbToU32(const u32 r, const u32 g, const u32 b)
    {
        return (r & 0x0FF) << 16 | (g & 0x0FF) << 8 | (b & 0x0FF);
    }

    C3D_API C3D_INLINE void U32ToRgb(const u32 rgb, u32* outR, u32* outG, u32* outB)
    {
        *outR = rgb >> 16 & 0x0FF;
        *outG = rgb >> 8 & 0x0FF;
        *outB = rgb & 0x0FF;
    }

    C3D_API C3D_INLINE vec3 RgbToVec3(const u32 r, const u32 g, const u32 b)
    {
        return { static_cast<f32>(r) / 255.0f, static_cast<f32>(g) / 255.0f, static_cast<f32>(b) / 255.0f };
    }

    C3D_API C3D_INLINE void Vec3ToRgb(const vec3& v, u32* outR, u32* outG, u32* outB)
    {
        *outR = static_cast<u32>(v.r) * 255;
        *outG = static_cast<u32>(v.g) * 255;
        *outB = static_cast<u32>(v.b) * 255;
    }

    C3D_API C3D_INLINE RGBA HsvToRgba(const HSV& hsv)
    {
        RGBA rgba(0, 0, 0, 1);

        f32 c = hsv.s * hsv.v;
        f32 x = c * (1 - Abs(Mod(hsv.h * 6.0f, 2.0f) - 1));
        f32 m = hsv.v - c;

        if (hsv.h >= 0.0f && hsv.h < HSV_60)
        {
            rgba.r = c;
            rgba.g = x;
            rgba.b = 0;
        }
        else if (hsv.h >= HSV_60 && hsv.h < HSV_120)
        {
            rgba.r = x;
            rgba.g = c;
            rgba.b = 0;
        }
        else if (hsv.h >= HSV_120 && hsv.h < HSV_180)
        {
            rgba.r = 0;
            rgba.g = c;
            rgba.b = x;
        }
        else if (hsv.h >= HSV_180 && hsv.h < HSV_240)
        {
            rgba.r = 0;
            rgba.g = x;
            rgba.b = c;
        }
        else if (hsv.h >= HSV_240 && hsv.h < HSV_300)
        {
            rgba.r = x;
            rgba.g = 0;
            rgba.b = c;
        }
        else
        {
            rgba.r = c;
            rgba.g = 0;
            rgba.b = x;
        }

        return rgba;
    }
}  // namespace C3D