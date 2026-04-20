module;

#include "imgui.h"

export module render.ui:util;
import weretype;

constexpr auto icon(const char8_t* s) -> const char* {
	return (const char*)s;
}
#define FA_ICON(name, literal) inline const auto name = icon(literal)

export namespace fa_icon {
	FA_ICON(wifi,        u8"\uF1EB");
	FA_ICON(satellite,   u8"\uF7C0");
	FA_ICON(gear,        u8"\uF013");
	FA_ICON(bookmark,    u8"\uF02E");
	FA_ICON(eye,         u8"\uF06E");
	FA_ICON(bars,        u8"\uF0C9");
	FA_ICON(bell,        u8"\uF0F3");
	FA_ICON(bell_slash,  u8"\uF1F6");
	FA_ICON(check,       u8"\uF00C");
	FA_ICON(file,        u8"\uF15B");
	FA_ICON(heart,       u8"\uF004");
	FA_ICON(play,        u8"\uF04B");
	FA_ICON(pause,       u8"\uF04C");
	FA_ICON(plus,        u8"\uF0FE");
	FA_ICON(wrench,      u8"\uF0AD");
	FA_ICON(display,     u8"\uE163");
	FA_ICON(network,     u8"\uF6FF");
	FA_ICON(server,      u8"\uF233");
	FA_ICON(terminal,    u8"\uF120");
	FA_ICON(file_lines,  u8"\uF15C");
	FA_ICON(lightbulb,   u8"\uF0EB");
	FA_ICON(faucet_drip, u8"\uE006");
	FA_ICON(arrow_up_19, u8"\uF887");
}

constexpr auto HexColor(u32 hex, f32 alpha = 1) -> ImVec4 {
	return ImVec4(
		((hex >> 16) & 0xFF) / 255.0f,
		((hex >> 8) & 0xFF) / 255.0f,
		(hex & 0xFF) / 255.0f,
		alpha
	);
}