#pragma once

#include <tuple>

#include "Utils.h"

class Surface {
public:
    static constexpr unsigned font{ 0x1d }; // builtin font from vgui_spew_fonts

    constexpr void setDrawColor(int r, int g, int b, int a = 255) noexcept
    {
        callVirtualMethod<void, int, int, int, int>(this, 15, r, g, b, a);
    }

    constexpr void setDrawColor(const float color[3], int a = 255) noexcept
    {
        callVirtualMethod<void, int, int, int, int>(this, 15, static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255), a);
    }

    constexpr void setDrawColor(std::tuple<float, float, float> color, int a = 255) noexcept
    {
        callVirtualMethod<void, int, int, int, int>(this, 15, static_cast<int>(std::get<0>(color) * 255), static_cast<int>(std::get<1>(color) * 255), static_cast<int>(std::get<2>(color) * 255), a);
    }

    template <typename T>
    constexpr void drawFilledRect(T x0, T y0, T x1, T y1) noexcept
    {
        callVirtualMethod<void, int, int, int, int>(this, 16, static_cast<int>(x0), static_cast<int>(y0), static_cast<int>(x1), static_cast<int>(y1));
    }

    template <typename T>
    constexpr void drawOutlinedRect(T x0, T y0, T x1, T y1) noexcept
    {
        callVirtualMethod<void, int, int, int, int>(this, 18, static_cast<int>(x0), static_cast<int>(y0), static_cast<int>(x1), static_cast<int>(y1));
    }

    template <typename T>
    constexpr void drawLine(T x0, T y0, T x1, T y1) noexcept
    {
        callVirtualMethod<void, int, int, int, int>(this, 19, static_cast<int>(x0), static_cast<int>(y0), static_cast<int>(x1), static_cast<int>(y1));
    }

    constexpr void drawPolyLine(int* xs, int* ys, int pointCount) noexcept
    {
        callVirtualMethod<void, int*, int*, int>(this, 20, xs, ys, pointCount);
    }

    constexpr void setTextFont(unsigned font) noexcept
    {
        callVirtualMethod<void, unsigned>(this, 23, font);
    }

    constexpr void setTextColor(int r, int g, int b, int a = 255) noexcept
    {
        callVirtualMethod<void, int, int, int, int>(this, 25, r, g, b, a);
    }

    constexpr void setTextColor(const float color[3], int a = 255) noexcept
    {
        callVirtualMethod<void, int, int, int, int>(this, 25, static_cast<int>(color[0] * 255), static_cast<int>(color[1] * 255), static_cast<int>(color[2] * 255), a);
    }

    constexpr void setTextColor(std::tuple<float, float, float> color, int a = 255) noexcept
    {
        callVirtualMethod<void, int, int, int, int>(this, 25, static_cast<int>(std::get<0>(color) * 255), static_cast<int>(std::get<1>(color) * 255), static_cast<int>(std::get<2>(color) * 255), a);
    }

    template <typename T>
    constexpr void setTextPosition(T x, T y) noexcept
    {
        callVirtualMethod<void, int, int>(this, 26, static_cast<int>(x), static_cast<int>(y));
    }

    constexpr void printText(const std::wstring_view text, int drawType = 0) noexcept
    {
        callVirtualMethod<void, const wchar_t*, int, int>(this, 28, text.data(), text.length(), drawType);
    }
    constexpr void drawSetTextureRGBA(int id, const unsigned char* rgba, int wide, int tall) noexcept {
        callVirtualMethod<void, int, const unsigned char*, int, int, int, bool>(this, 37, id, rgba, wide, tall, 0, false);
    }
    constexpr void drawSetTexture(int id) noexcept {
        callVirtualMethod<void, int>(this, 38, id);
    }
    constexpr void drawTexturedRect(int x0, int y0, int x1, int y1) noexcept {
        callVirtualMethod<void, int, int, int, int>(this, 40, x0, y0, x1, y1);
    }
    constexpr auto createNewTextureID(bool procedural = false) noexcept {
        return callVirtualMethod<int, bool>(this, 43, procedural);
    }
    constexpr auto getScreenSize() noexcept
    {
        int width{ }, height{ };
        callVirtualMethod<void, int&, int&>(this, 44, width, height);
        return std::make_pair(width, height);
    }

    constexpr void unlockCursor() noexcept
    {
        callVirtualMethod<void>(this, 66);
    }

    constexpr unsigned createFont() noexcept
    {
        return callVirtualMethod<unsigned>(this, 71);
    }

    constexpr bool setFontGlyphSet(unsigned font, const char* fontName, int tall, int weight, int blur, int scanlines, int flags, int rangeMin = 0, int rangeMax = 0) noexcept
    {
        return callVirtualMethod<bool, unsigned, const char*, int, int, int, int, int, int, int>(this, 72, font, fontName, tall, weight, blur, scanlines, flags, rangeMin, rangeMax);
    }

    constexpr auto getTextSize(unsigned font, const wchar_t* text) noexcept
    {
        int width{ }, height{ };
        callVirtualMethod<void, unsigned, const wchar_t*, int&, int&>(this, 79, font, text, width, height);
        return std::make_pair(width, height);
    }
    constexpr void playSound(const char* fileName) noexcept
    {
        callVirtualMethod<void, const char*>(this, 82, fileName);
    }
    template <typename T>
    constexpr void drawOutlinedCircle(T x, T y, int r, int seg) noexcept
    {
        callVirtualMethod<void, int, int, int, int>(this, 103, static_cast<int>(x), static_cast<int>(y), r, seg);
    }

    template <typename T>
    void drawCircle(T x, T y, int startRadius, int radius) noexcept
    {
        int xs[12];
        int ys[12];

        for (int i = startRadius; i <= radius; ++i) {
            for (int j = 0; j < 12; ++j) {
                xs[j] = static_cast<int>(std::cos(degreesToRadians(static_cast<float>(j * 30))) * i + x);
                ys[j] = static_cast<int>(std::sin(degreesToRadians(static_cast<float>(j * 30))) * i + y);
            }
            g_interfaces.surface->drawPolyLine(xs, ys, 12);
        }
    }
    constexpr void drawFilledRectFade(int x0, int y0, int x1, int y1, unsigned int alpha0, unsigned int alpha1, bool horizontal) noexcept {
        callVirtualMethod<void, int, int, int, int, unsigned int, unsigned int, bool>(this, 123, x0, y0, x1, y1, alpha0, alpha1, horizontal);
    }
};
