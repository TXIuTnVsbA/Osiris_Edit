#pragma once

#include "VirtualMethod.h"

struct ConVar;
struct c_console_color
{
	unsigned char r, g, b, a;
};
class CColor {
public:
	unsigned char RGBA[4];
	CColor()
	{
		RGBA[0] = 255;
		RGBA[1] = 255;
		RGBA[2] = 255;
		RGBA[3] = 255;
	}
	CColor(int r, int g, int b, int a = 255)
	{
		RGBA[0] = r;
		RGBA[1] = g;
		RGBA[2] = b;
		RGBA[3] = a;
	}
	inline int r() const
	{
		return RGBA[0];
	}
	inline int g() const
	{
		return RGBA[1];
	}
	inline int b() const
	{
		return RGBA[2];
	}
	inline int a() const
	{
		return RGBA[3];
	}
	bool operator!=(CColor color)
	{
		return RGBA[0] != color.RGBA[0] || RGBA[1] != color.RGBA[1] || RGBA[2] != color.RGBA[2] || RGBA[3] != color.RGBA[3];
	}
	bool operator==(CColor color)
	{
		return RGBA[0] == color.RGBA[0] && RGBA[1] == color.RGBA[1] && RGBA[2] == color.RGBA[2] && RGBA[3] == color.RGBA[3];
	}
	static float Base(const unsigned char col)
	{
		return col / 255.f;
	}
};

class Cvar {
protected:
	~Cvar() = default;
public:
	VIRTUAL_METHOD(ConVar*, findVar, 15, (const char* name), (this, name));
	template <typename... args>
	void consoleColorPrintf(const CColor& color, const char* format, args... arg) {
		const c_console_color cl = { static_cast<unsigned char>(color.r()), static_cast<unsigned char>(color.g()),
			static_cast<unsigned char>(color.b()), static_cast<unsigned char>(color.a()) };

		reinterpret_cast<void(*)(Cvar*, const c_console_color&, const char*, ...)>(
			(*reinterpret_cast<uint32_t**>(this))[25])(this, cl, format, arg...);
	}
};
