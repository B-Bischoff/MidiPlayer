#pragma once

#include <imgui.h>

namespace UI_Colors {
	constexpr ImVec4 white              = ImVec4(1.00f, 1.00f, 1.00f, 1.0f); // #ffffff

	constexpr ImVec4 mid                = ImVec4(0.54f, 0.36f, 0.82f, 1.00f); // #895cd1
	constexpr ImVec4 base               = ImVec4(0.60f, 0.44f, 0.84f, 1.00f); // #996fd6
	constexpr ImVec4 light              = ImVec4(0.65f, 0.53f, 0.86f, 1.00f); // #a786db
	constexpr ImVec4 lighter            = ImVec4(0.71f, 0.61f, 0.88f, 1.00f); // #b59ce0
	constexpr ImVec4 highlight          = ImVec4(0.76f, 0.70f, 0.90f, 1.00f); // #c2b3e5

	constexpr ImVec4 background_dark    = ImVec4(0.10f, 0.08f, 0.14f, 1.00f); // #1a1424
	constexpr ImVec4 background_mid     = ImVec4(0.12f, 0.10f, 0.16f, 1.00f); // #1f1a29
	constexpr ImVec4 background_light   = ImVec4(0.16f, 0.13f, 0.20f, 1.00f); // #292133
	constexpr ImVec4 background_lighter = ImVec4(0.20f, 0.16f, 0.26f, 1.00f); // #332942
}
