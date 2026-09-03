#include "pch.h"
#include ".\UI\SoulMeterTheme.h"

namespace {

const ImVec4 kAccent(0.12f, 0.66f, 0.80f, 1.00f);
const ImVec4 kAccentHover(0.18f, 0.76f, 0.90f, 1.00f);
const ImVec4 kAccentActive(0.08f, 0.52f, 0.66f, 1.00f);
const ImVec4 kLive(0.35f, 0.84f, 0.61f, 1.00f);
const ImVec4 kIdle(0.60f, 0.66f, 0.73f, 1.00f);

}

void SoulMeterTheme::Apply(bool preserveUserOverrides) {
	ImGuiStyle& style = ImGui::GetStyle();

	const ImVec4 savedText = style.Colors[ImGuiCol_Text];
	const ImVec4 savedWindowBg = style.Colors[ImGuiCol_WindowBg];
	const float savedWindowBorderSize = style.WindowBorderSize;
	const ImVec2 savedCellPadding = style.CellPadding;

	ImGui::StyleColorsDark(&style);

	style.Alpha = 1.0f;
	style.WindowPadding = ImVec2(10.0f, 8.0f);
	style.FramePadding = ImVec2(8.0f, 5.0f);
	style.CellPadding = ImVec2(6.0f, 4.0f);
	style.ItemSpacing = ImVec2(8.0f, 6.0f);
	style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
	style.IndentSpacing = 18.0f;
	style.ScrollbarSize = 10.0f;
	style.GrabMinSize = 10.0f;
	style.WindowBorderSize = 1.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupBorderSize = 1.0f;
	style.FrameBorderSize = 0.0f;
	style.TabBorderSize = 0.0f;
	style.WindowRounding = 4.0f;
	style.ChildRounding = 3.0f;
	style.FrameRounding = 3.0f;
	style.PopupRounding = 3.0f;
	style.ScrollbarRounding = 3.0f;
	style.GrabRounding = 3.0f;
	style.TabRounding = 3.0f;

	style.Colors[ImGuiCol_Text] = ImVec4(0.89f, 0.92f, 0.95f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.47f, 0.53f, 0.61f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.035f, 0.050f, 0.065f, 1.00f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.055f, 0.075f, 0.095f, 1.00f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.060f, 0.085f, 0.110f, 1.00f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.18f, 0.23f, 0.28f, 1.00f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.085f, 0.115f, 0.145f, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.105f, 0.180f, 0.220f, 1.00f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.100f, 0.270f, 0.330f, 1.00f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.030f, 0.045f, 0.060f, 1.00f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.055f, 0.100f, 0.130f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.030f, 0.045f, 0.060f, 0.85f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.055f, 0.075f, 0.095f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.035f, 0.050f, 0.065f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.22f, 0.28f, 0.33f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.39f, 0.45f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = kAccentActive;
	style.Colors[ImGuiCol_CheckMark] = kAccentHover;
	style.Colors[ImGuiCol_SliderGrab] = kAccent;
	style.Colors[ImGuiCol_SliderGrabActive] = kAccentHover;
	style.Colors[ImGuiCol_Button] = ImVec4(0.090f, 0.150f, 0.185f, 1.00f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.110f, 0.250f, 0.305f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive] = kAccentActive;
	style.Colors[ImGuiCol_Header] = ImVec4(0.090f, 0.160f, 0.200f, 1.00f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.120f, 0.285f, 0.345f, 1.00f);
	style.Colors[ImGuiCol_HeaderActive] = kAccentActive;
	style.Colors[ImGuiCol_Separator] = ImVec4(0.18f, 0.23f, 0.28f, 1.00f);
	style.Colors[ImGuiCol_SeparatorHovered] = kAccentHover;
	style.Colors[ImGuiCol_SeparatorActive] = kAccent;
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.15f, 0.26f, 0.31f, 0.65f);
	style.Colors[ImGuiCol_ResizeGripHovered] = kAccentHover;
	style.Colors[ImGuiCol_ResizeGripActive] = kAccent;
	style.Colors[ImGuiCol_Tab] = ImVec4(0.065f, 0.100f, 0.130f, 1.00f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.105f, 0.270f, 0.330f, 1.00f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.095f, 0.205f, 0.255f, 1.00f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.050f, 0.075f, 0.100f, 1.00f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.070f, 0.130f, 0.165f, 1.00f);
	style.Colors[ImGuiCol_DockingPreview] = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.70f);
	style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.035f, 0.050f, 0.065f, 1.00f);
	style.Colors[ImGuiCol_PlotLines] = kAccentHover;
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.76f, 0.32f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = kAccent;
	style.Colors[ImGuiCol_PlotHistogramHovered] = kAccentHover;
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.075f, 0.115f, 0.145f, 1.00f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.20f, 0.26f, 0.31f, 1.00f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.11f, 0.15f, 0.19f, 1.00f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.045f, 0.065f, 0.085f, 1.00f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.060f, 0.085f, 0.110f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.35f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 0.76f, 0.32f, 0.95f);
	style.Colors[ImGuiCol_NavHighlight] = kAccentHover;
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.30f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.48f);

	if (preserveUserOverrides) {
		style.Colors[ImGuiCol_Text] = savedText;
		style.Colors[ImGuiCol_WindowBg] = savedWindowBg;
		style.WindowBorderSize = savedWindowBorderSize;
		style.CellPadding = savedCellPadding;
	}

	ImPlot::StyleColorsAuto();
	ImPlotStyle& plotStyle = ImPlot::GetStyle();
	plotStyle.PlotBorderSize = 1.0f;
	plotStyle.PlotPadding = ImVec2(10.0f, 8.0f);
	plotStyle.LabelPadding = ImVec2(6.0f, 4.0f);
	plotStyle.LegendPadding = ImVec2(8.0f, 6.0f);
	plotStyle.LegendInnerPadding = ImVec2(5.0f, 4.0f);
	plotStyle.Colors[ImPlotCol_PlotBg] = ImVec4(0.045f, 0.065f, 0.085f, 1.00f);
	plotStyle.Colors[ImPlotCol_PlotBorder] = style.Colors[ImGuiCol_Border];
	plotStyle.Colors[ImPlotCol_LegendBg] = ImVec4(0.055f, 0.080f, 0.100f, 0.92f);
	plotStyle.Colors[ImPlotCol_LegendBorder] = style.Colors[ImGuiCol_Border];
	plotStyle.Colors[ImPlotCol_XAxisGrid] = ImVec4(0.32f, 0.42f, 0.50f, 0.20f);
	plotStyle.Colors[ImPlotCol_YAxisGrid] = ImVec4(0.32f, 0.42f, 0.50f, 0.20f);
}

const ImVec4& SoulMeterTheme::Accent() {
	return kAccent;
}

const ImVec4& SoulMeterTheme::Live() {
	return kLive;
}

const ImVec4& SoulMeterTheme::Idle() {
	return kIdle;
}
