#include "pch.h"
#include <map>
#include ".\UI\PlayerTable.h"
#include ".\Damage Meter\Damage Meter.h"
#include ".\Damage Meter\History.h"
#include ".\Damage Meter\MySQLite.h"
#include ".\UI\Option.h"
#include ".\UI\UiWindow.h"
#include ".\UI\UtillWindow.h"
#include ".\UI\PlotWindow.h"
#include ".\UI\SoulMeterTheme.h"
#include ".\Soulworker Packet\PacketInfo.h"
#include ".\Soulworker Packet\SWPacketMaker.h"
#include ".\Soulworker Packet\PipeReceiver.h"
#include "SWConfig.h"
#include ".\UI\DX11.h"

namespace {
	constexpr float STACKED_METER_WIDTH = 320.0f;
	constexpr float STACKED_METER_LABEL_WIDTH = 118.0f;
	// Keep the stacked view close to the original compact meter even when a
	// previously saved normal-table width was very large.
	constexpr float STACKED_METER_MAX_WIDTH = 520.0f;

	enum StackedRowID {
		StackedRowName,
		StackedRowDamagePercent,
		StackedRowDps,
		StackedRowDamage,
		StackedRowHit,
		StackedRowCrit,
		StackedRowHitPerSecond,
		StackedRowCritHitPerSecond,
		StackedRowSkillPerSecond,
		StackedRowSkillPicker,
		StackedRowSkillHit,
		StackedRowSkillCrit,
		StackedRowSkillHitPerSecond,
		StackedRowSkillHitPerCast,
		StackedRowMaxCombo,
		StackedRowAttackCritDamage,
		StackedRowSoulGauge,
		StackedRowAttackSpeed,
		StackedRowArmorBreak,
		StackedRowBossDamage,
		StackedRowStamina,
		StackedRowSoulVapor,
		StackedRowStoneDamageRate,
		StackedRowStoneProc,
		StackedRowStoneDamage,
		StackedRowAvgArmorBreak,
		StackedRowAvgArmorBreakUncapped,
		StackedRowAvgBossDamage,
		StackedRowMiss,
		StackedRowMissPercent,
		StackedRowMissDamage,
		StackedRowGetHitAll,
		StackedRowGetHit,
		StackedRowGetHitBS,
		StackedRowEvadeRateA,
		StackedRowEvadeRateB,
		StackedRowGigaEnlighten,
		StackedRowTeraEnlighten,
		StackedRowTeraFever,
		StackedRowTeraFury,
		StackedRowTeraBackstep,
		StackedRowTeraTechnic,
		StackedRowLosedHp,
		StackedRowDodge,
		StackedRowDeath,
		StackedRowFullArmorBreakTime,
		StackedRowFullArmorBreakPercent,
		StackedRowGigaEnlightenSkillPercent,
		StackedRowTeraEnlightenSkillPercent,
		StackedRowAggroPercent,
		StackedRowFullAttackSpeedTime,
		StackedRowFullAttackSpeedPercent,
		StackedRowAvgAttackSpeedPercent,
		StackedRowCount
	};

	bool g_stackedRowsVisible[StackedRowCount] = { false };
	bool g_stackedRowsInitialized = false;

	const char* g_stackedRowLabels[StackedRowCount] = {
		"NAME",
		"D%",
		"DPS",
		"DAMAGE",
		"HIT",
		"CRIT%",
		"HIT/s",
		"C.HIT/s",
		"Skill/s",
		"Skill",
		"S.Hit",
		"S.Crit",
		"S.Hit/s",
		"S.Hit/Cast",
		"MAXC",
		"ATK+C.DMG",
		"SG",
		"AS",
		"AB",
		"BD",
		"Stam",
		"SV",
		"Stone DMG Rate",
		"Stone Proc",
		"Stone DMG",
		"Avg.AB",
		"Avg.AB(U)",
		"Avg.BD",
		"Miss",
		"Miss%",
		"Miss DMG",
		"Get Hit(+0 DMG)",
		"Get Hit",
		"Get Hit(BS)",
		"Evade Rate A",
		"Evade Rate B",
		"Giga.Enli",
		"Tera.Enli",
		"Tera.Fever",
		"Tera.Fury",
		"Tera.Backstep",
		"Tera.Technic",
		"Losed HP",
		"Dodge",
		"Death",
		"Full.AB(s)",
		"Full.AB(%)",
		"G.Enli/Skill(%)",
		"T.Enli/Skill(%)",
		"Aggro%",
		"Full.AS(s)",
		"Full.AS(%)",
		"Avg.AS(%)"
	};

	bool IsStackedRowVisible(StackedRowID rowID) {
		return rowID >= 0 && rowID < StackedRowCount && g_stackedRowsVisible[rowID];
	}

	ImGuiTableColumnFlags StackedSyncedColumnFlags(StackedRowID rowID, ImGuiTableColumnFlags flags) {
		return IsStackedRowVisible(rowID) ? flags : (flags | ImGuiTableColumnFlags_DefaultHide);
	}

	void EnsureStackedRowsInitialized() {
		if (g_stackedRowsInitialized)
			return;

		// Fresh installs start with the compact, useful set of rows. The long
		// list of optional combat metrics remains available through Rows..., but
		// it should never make the first launch fill the screen.
		for (int i = 0; i < StackedRowCount; i++)
			g_stackedRowsVisible[i] = (i <= StackedRowSkillHitPerCast);
		g_stackedRowsInitialized = true;
	}

	// Keep the normal table's column visibility and the stacked view's row
	// visibility in sync when the user switches layouts.
	bool SyncStackedRowsFromNormalTable() {
		const int columnCount = ImGui::TableGetColumnCount();
		if (columnCount != StackedRowCount)
			return false;

		bool changed = false;
		for (int i = 0; i < StackedRowCount; i++) {
			const bool visible = (ImGui::TableGetColumnFlags(i) & ImGuiTableColumnFlags_IsEnabled) != 0;
			if (g_stackedRowsVisible[i] != visible) {
				g_stackedRowsVisible[i] = visible;
				changed = true;
			}
		}
		return changed;
	}

	void AppendCurrentUnit(char* dest, size_t destLen) {
		if (UIOPTION.is1K())
			strcat_s(dest, destLen, LANGMANAGER.GetText("STR_DISPLAY_UNIT_1K").data());
		else if (UIOPTION.is1M())
			strcat_s(dest, destLen, LANGMANAGER.GetText("STR_DISPLAY_UNIT_1M").data());
		else if (UIOPTION.is10K())
			strcat_s(dest, destLen, LANGMANAGER.GetText("STR_DISPLAY_UNIT_10K").data());
	}

	void FormatScaledUnsigned(uint64_t value, char* dest, size_t destLen) {
		if (UIOPTION.is1K())
			value /= 1000;
		else if (UIOPTION.is1M())
			value /= 1000000;
		else if (UIOPTION.is10K())
			value /= 10000;

		char raw[128] = { 0 };
		sprintf_s(raw, "%llu", value);
		TextCommma(raw, dest);
		AppendCurrentUnit(dest, destLen);
	}

	void FormatCommaUnsigned(uint64_t value, char* dest, size_t destLen) {
		char raw[128] = { 0 };
		(void)destLen;
		sprintf_s(raw, "%llu", value);
		TextCommma(raw, dest);
	}

		void FormatDpsValue(uint64_t damage, float tableTime, char* dest, size_t destLen) {
		if (tableTime < 1.0f) {
			strcpy_s(dest, destLen, "-");
			return;
		}

		double dps = ((double)damage) / tableTime;
		if (UIOPTION.is1K())
			dps /= 1000;
		else if (UIOPTION.is1M())
			dps /= 1000000;
		else if (UIOPTION.is10K())
			dps /= 10000;

		if (UIOPTION.is1M()) {
			TextCommmaIncludeDecimal(dps, destLen, dest);
		}
		else {
			char raw[128] = { 0 };
			sprintf_s(raw, "%.0lf", dps);
			TextCommma(raw, dest);
		}
			AppendCurrentUnit(dest, destLen);
		}

	void AccumulateTrackedSkill(std::map<uint32_t, TRACKED_SKILL_SUMMARY>& skillTotals, SWDamageSkill* skill) {
		if (skill == nullptr)
			return;

		const uint32_t skillID = skill->GetID();
		TRACKED_SKILL_SUMMARY& summary = skillTotals[skillID];
		if (!summary._found) {
			summary._found = true;
			summary._skillID = skillID;
			strcpy_s(summary._name, sizeof(summary._name), skill->GetName());
		}

		summary._damage += skill->GetDamage();
		summary._hitCount += skill->GetHitCount();
		summary._critHitCount += skill->GetCritHitCount();
	}

	bool SkillNamesMatch(const char* left, const char* right) {
		if (left == nullptr || right == nullptr || left[0] == '\0' || right[0] == '\0')
			return false;

		if (_stricmp(left, right) == 0)
			return true;

		char leftBase[64] = { 0 };
		char rightBase[64] = { 0 };
		strcpy_s(leftBase, sizeof(leftBase), left);
		strcpy_s(rightBase, sizeof(rightBase), right);

		// Damage packets sometimes identify a form/variant while the cast packet
		// carries the base skill ID (for example, "Skill - Alter Mode").
		char* leftSuffix = strstr(leftBase, " - ");
		if (leftSuffix != nullptr)
			*leftSuffix = '\0';
		char* rightSuffix = strstr(rightBase, " - ");
		if (rightSuffix != nullptr)
			*rightSuffix = '\0';

		return _stricmp(leftBase, rightBase) == 0;
	}

	void BuildTrackedSkillTotals(SWDamagePlayer* player, std::map<uint32_t, TRACKED_SKILL_SUMMARY>& skillTotals) {
		if (player == nullptr)
			return;

		for (auto monster = player->begin(); monster != player->end(); monster++) {
			for (auto skill = (*monster)->begin(); skill != (*monster)->end(); skill++) {
				AccumulateTrackedSkill(skillTotals, *skill);
			}
		}

		for (auto skillCount = player->skillCounts.begin(); skillCount != player->skillCounts.end(); skillCount++) {
			if (skillCount->second == nullptr)
				continue;

			const uint32_t castCount = skillCount->second->_count;
			bool matchedDamage = false;
			auto exact = skillTotals.find(skillCount->first);
			if (exact != skillTotals.end() && exact->second._hitCount > 0) {
				exact->second._casts += castCount;
				matchedDamage = true;
			}
			else {
				char castName[64] = { 0 };
				SWDB.GetSkillName(skillCount->first, castName, sizeof(castName));
				for (auto candidate = skillTotals.begin(); candidate != skillTotals.end(); candidate++) {
					if (candidate->second._hitCount == 0 || !SkillNamesMatch(candidate->second._name, castName))
						continue;

					// A base cast can produce several damage-form IDs. Keep the
					// lifetime cast count associated with every matching form for
					// selector metadata.
					candidate->second._casts += castCount;
					matchedDamage = true;
				}
			}

			if (matchedDamage) {
				continue;
			}

			// Preserve cast-only skills in the selector even when no damage packet
			// has arrived for them yet.
			TRACKED_SKILL_SUMMARY& skillSummary = skillTotals[skillCount->first];
			if (!skillSummary._found) {
				skillSummary._found = true;
				skillSummary._skillID = skillCount->first;
				SWDB.GetSkillName(skillCount->first, skillSummary._name, sizeof(skillSummary._name));
			}
			skillSummary._casts += castCount;
		}

		for (auto skillTotal = skillTotals.begin(); skillTotal != skillTotals.end(); skillTotal++) {
			uint32_t castHits = 0;
			uint32_t castCritHits = 0;
			if (player->GetSkillCastHits(skillTotal->second._skillID, castHits, castCritHits)) {
				skillTotal->second._hasCast = true;
				skillTotal->second._castHitCount = castHits;
				skillTotal->second._castCritHitCount = castCritHits;
			}
		}
	}
}

std::string PlayerTable::GetStackedRowsVisibility() {
	EnsureStackedRowsInitialized();
	std::string visibility;
	visibility.reserve(StackedRowCount);
	for (int i = 0; i < StackedRowCount; i++)
		visibility.push_back(g_stackedRowsVisible[i] ? '1' : '0');
	return visibility;
}

void PlayerTable::SetStackedRowsVisibility(const char* visibility) {
	if (visibility == nullptr || visibility[0] == '\0')
		return;

	for (int i = 0; i < StackedRowCount; i++)
		g_stackedRowsVisible[i] = false;
	for (int i = 0; i < StackedRowCount; i++) {
		const char value = visibility[i];
		if (value == '\0')
			break;
		g_stackedRowsVisible[i] = (value == '1' || value == 'y' || value == 'Y' || value == 't' || value == 'T');
	}

	// NAME is the recovery/display anchor for both layouts.
	g_stackedRowsVisible[StackedRowName] = true;
	g_stackedRowsInitialized = true;
}

void PlayerTable::ResetStackedRowsVisibility() {
	for (int i = 0; i < StackedRowCount; i++)
		g_stackedRowsVisible[i] = (i <= StackedRowSkillHitPerCast);
	g_stackedRowsInitialized = true;
}

PlayerTable::PlayerTable() : _tableResize(0), _globalFontScale(0), _columnFontScale(0), _tableFontScale(0), _curWindowSize(0), _tableTime(0), _accumulatedTime(0), _windowSizeInitialized(false), _wasStackedMeterMode(false)
{

}

PlayerTable::~PlayerTable() {
	ClearTable();
}

	void PlayerTable::ClearTable() {

	for (auto itr = _selectInfo.begin(); itr != _selectInfo.end(); itr++) {
		delete (*itr)->_specificInfo;
	}

	for (auto itr = _selectInfo.begin(); itr != _selectInfo.end(); itr++) {
		delete (*itr);
	}

		_selectInfo.clear();
		_trackedSkillByPlayer.clear();
		_curWindowSize = 0;
		_windowSizeInitialized = false;
	}

	void PlayerTable::TrackPlayerSkill(uint32_t playerID, uint32_t skillID, size_t slotIndex) {
		std::vector<uint32_t>& slots = _trackedSkillByPlayer[playerID];
		if (slots.size() <= slotIndex)
			slots.resize(slotIndex + 1, 0);
		slots[slotIndex] = skillID;
	}

	uint32_t PlayerTable::GetTrackedSkill(uint32_t playerID, size_t slotIndex) const {
		auto itr = _trackedSkillByPlayer.find(playerID);
		if (itr == _trackedSkillByPlayer.end())
			return 0;
		if (itr->second.size() <= slotIndex)
			return 0;

		return itr->second[slotIndex];
	}

	void PlayerTable::ClearTrackedSkill(uint32_t playerID, size_t slotIndex) {
		auto itr = _trackedSkillByPlayer.find(playerID);
		if (itr == _trackedSkillByPlayer.end())
			return;
		if (itr->second.size() <= slotIndex)
			return;

		itr->second[slotIndex] = 0;
	}

	void PlayerTable::AddTrackedSkillSlot(uint32_t playerID) {
		std::vector<uint32_t>& slots = _trackedSkillByPlayer[playerID];
		slots.push_back(0);
	}

	void PlayerTable::RemoveTrackedSkillSlot(uint32_t playerID, size_t slotIndex) {
		auto itr = _trackedSkillByPlayer.find(playerID);
		if (itr == _trackedSkillByPlayer.end())
			return;
		if (itr->second.size() <= slotIndex)
			return;

		itr->second.erase(itr->second.begin() + slotIndex);
		if (itr->second.empty()) {
			_trackedSkillByPlayer.erase(itr);
		}
	}

	size_t PlayerTable::GetTrackedSkillSlotCount(uint32_t playerID) const {
		auto itr = _trackedSkillByPlayer.find(playerID);
		if (itr == _trackedSkillByPlayer.end() || itr->second.empty())
			return 1;

		return itr->second.size();
	}

	bool PlayerTable::BuildTrackedSkillSummary(SWDamagePlayer* player, TRACKED_SKILL_SUMMARY& summary, size_t slotIndex) const {
		summary = TRACKED_SKILL_SUMMARY();
		if (player == nullptr)
			return false;

		std::map<uint32_t, TRACKED_SKILL_SUMMARY> skillTotals;
		BuildTrackedSkillTotals(player, skillTotals);

		const uint32_t trackedSkillID = GetTrackedSkill(player->GetID(), slotIndex);
		if (trackedSkillID != 0) {
			auto tracked = skillTotals.find(trackedSkillID);
			if (tracked != skillTotals.end()) {
				summary = tracked->second;
			}
			else {
				summary._found = true;
				summary._skillID = trackedSkillID;
				SWDB.GetSkillName(trackedSkillID, summary._name, sizeof(summary._name));
			}
			summary._pinned = true;
			return true;
		}

		for (auto itr = skillTotals.begin(); itr != skillTotals.end(); itr++) {
			if (!itr->second._found)
				continue;

			if (!summary._found ||
				itr->second._hitCount > summary._hitCount ||
				(itr->second._hitCount == summary._hitCount && itr->second._damage > summary._damage)) {
				summary = itr->second;
			}
		}

		return summary._found;
	}

	void PlayerTable::DrawTrackedSkillCombo(SWDamagePlayer* player, const TRACKED_SKILL_SUMMARY& trackedSkill, size_t slotIndex) {
		if (player == nullptr)
			return;

		std::map<uint32_t, TRACKED_SKILL_SUMMARY> skillTotals;
		BuildTrackedSkillTotals(player, skillTotals);

		char comboPreview[128] = { 0 };
		sprintf_s(comboPreview, "%s%s", trackedSkill._pinned ? "" : "Auto: ", trackedSkill._name);

		char comboID[64] = { 0 };
		sprintf_s(comboID, "##TrackedSkill%u_%llu", player->GetID(), (unsigned long long)slotIndex);

		const bool compactSkillCombo = !UIOPTION.isStackedMeterMode();
		if (compactSkillCombo) {
			ImGuiStyle& style = ImGui::GetStyle();
			ImVec4 comboFrame = style.Colors[ImGuiCol_FrameBg];
			ImVec4 comboFrameHovered = style.Colors[ImGuiCol_FrameBgHovered];
			ImVec4 comboFrameActive = style.Colors[ImGuiCol_FrameBgActive];
			comboFrame.w *= 0.30f;
			comboFrameHovered.w *= 0.55f;
			comboFrameActive.w *= 0.75f;
			ImGui::PushStyleColor(ImGuiCol_FrameBg, comboFrame);
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, comboFrameHovered);
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, comboFrameActive);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));
		}

		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::BeginCombo(comboID, comboPreview, ImGuiComboFlags_HeightLarge)) {
			char autoLabel[160] = { 0 };
			sprintf_s(autoLabel, "Auto: top-hit skill###AutoSkill%u_%llu", player->GetID(), (unsigned long long)slotIndex);
			if (ImGui::Selectable(autoLabel, GetTrackedSkill(player->GetID(), slotIndex) == 0)) {
				ClearTrackedSkill(player->GetID(), slotIndex);
			}
			ImGui::Separator();

			for (auto itr = skillTotals.begin(); itr != skillTotals.end(); itr++) {
				if (!itr->second._found)
					continue;

				char skillLabel[256] = { 0 };
				sprintf_s(skillLabel, "%s  (%u hits)###Skill%u_%llu", itr->second._name, itr->second._hitCount, itr->first, (unsigned long long)slotIndex);
				if (ImGui::Selectable(skillLabel, GetTrackedSkill(player->GetID(), slotIndex) == itr->first)) {
					TrackPlayerSkill(player->GetID(), itr->first, slotIndex);
				}
			}

			ImGui::EndCombo();
		}
		if (compactSkillCombo) {
			ImGui::PopStyleVar();
			ImGui::PopStyleColor(3);
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s #%u", trackedSkill._pinned ? "Selected" : "Auto", trackedSkill._skillID);
		}
	}

	void PlayerTable::DrawTrackedSkillColumns(SWDamagePlayer* player) {
		TRACKED_SKILL_SUMMARY trackedSkill;
		char label[128] = { 0 };
		char comma[128] = { 0 };

		if (!BuildTrackedSkillSummary(player, trackedSkill)) {
			for (int i = 0; i < 5; i++) {
				ImGui::Text("-");
				ImGui::TableNextColumn();
			}
			return;
		}

		DrawTrackedSkillCombo(player, trackedSkill);
		ImGui::TableNextColumn();

		sprintf_s(label, sizeof(label), "%u", trackedSkill._hitCount);
		TextCommma(label, comma);
		ImGui::Text(comma);
		ImGui::TableNextColumn();

		sprintf_s(label, sizeof(label), "%u", trackedSkill._critHitCount);
		TextCommma(label, comma);
		ImGui::Text(comma);
		ImGui::TableNextColumn();

		if (_tableTime == 0.0f)
			sprintf_s(label, sizeof(label), "-");
		else
			sprintf_s(label, sizeof(label), "%.2lf", (double)trackedSkill._hitCount / _tableTime);
		ImGui::Text(label);
		ImGui::TableNextColumn();

		if (!trackedSkill._hasCast)
			sprintf_s(label, sizeof(label), "-");
		else
			sprintf_s(label, sizeof(label), "%u", trackedSkill._castHitCount);
		ImGui::Text(label);
		ImGui::TableNextColumn();
	}

	void PlayerTable::DrawStackedSkillBlock(SWDamagePlayer* player, const TRACKED_SKILL_SUMMARY& trackedSkill, size_t slotIndex) {
		char value[128] = { 0 };

		if (IsStackedRowVisible(StackedRowSkillPicker)) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextDisabled("%s", trackedSkill._pinned ? "PIN SKILL" : "TOP SKILL");
			ImGui::TableNextColumn();
			if (slotIndex > 0) {
				char removeLabel[64] = { 0 };
				sprintf_s(removeLabel, "x###RemoveSkill%u_%llu", player->GetID(), (unsigned long long)slotIndex);
				if (ImGui::SmallButton(removeLabel)) {
					RemoveTrackedSkillSlot(player->GetID(), slotIndex);
					return;
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Remove this skill row");
				}
				ImGui::SameLine();
			}
			DrawTrackedSkillCombo(player, trackedSkill, slotIndex);
		}

		FormatCommaUnsigned(trackedSkill._hitCount, value, sizeof(value));
		if (IsStackedRowVisible(StackedRowSkillHit))
			DrawStackedMetric("S.HIT", value);

		FormatCommaUnsigned(trackedSkill._critHitCount, value, sizeof(value));
		if (IsStackedRowVisible(StackedRowSkillCrit))
			DrawStackedMetric("S.CRIT", value);

		if (_tableTime == 0.0f)
			strcpy_s(value, "-");
		else
			sprintf_s(value, "%.2lf", (double)trackedSkill._hitCount / _tableTime);
		if (IsStackedRowVisible(StackedRowSkillHitPerSecond))
			DrawStackedMetric("S.HIT/s", value);

		if (!trackedSkill._hasCast)
			strcpy_s(value, "-");
		else
			sprintf_s(value, "%u", trackedSkill._castHitCount);
		if (IsStackedRowVisible(StackedRowSkillHitPerCast))
			DrawStackedMetric("S.HIT/CAST", value);
	}

	void PlayerTable::DrawStackedSkillAddRow(SWDamagePlayer* player) {
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextDisabled("+");
		ImGui::TableNextColumn();

		char label[64] = { 0 };
		sprintf_s(label, "+ Add skill###AddSkill%u", player->GetID());
		if (ImGui::Button(label, ImVec2(-FLT_MIN, 0.0f))) {
			AddTrackedSkillSlot(player->GetID());
		}
	}

	void PlayerTable::DrawStackedRowsMenu() {
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextDisabled("OPTIONS");
		ImGui::TableNextColumn();

		if (ImGui::Button("Rows...###StackedRowsButton", ImVec2(-FLT_MIN, 0.0f))) {
			ImGui::OpenPopup("###StackedRowsPopup");
		}

		if (ImGui::BeginPopup("###StackedRowsPopup")) {
			if (ImGui::Selectable("Show all", false, ImGuiSelectableFlags_DontClosePopups)) {
				for (int i = 0; i < StackedRowCount; i++)
					g_stackedRowsVisible[i] = true;
				UIOPTION.SaveOption(TRUE);
			}
			if (ImGui::Selectable("Deselect all", false, ImGuiSelectableFlags_DontClosePopups)) {
				for (int i = 0; i < StackedRowCount; i++)
					g_stackedRowsVisible[i] = false;
				g_stackedRowsVisible[StackedRowName] = true;
				UIOPTION.SaveOption(TRUE);
			}
			ImGui::Separator();
			for (int i = 0; i < StackedRowCount; i++) {
				if (ImGui::Selectable(g_stackedRowLabels[i], g_stackedRowsVisible[i], ImGuiSelectableFlags_DontClosePopups)) {
					g_stackedRowsVisible[i] = !g_stackedRowsVisible[i];
					if (i == StackedRowName)
						g_stackedRowsVisible[i] = true;
					UIOPTION.SaveOption(TRUE);
				}
			}
			ImGui::EndPopup();
		}
	}

	void PlayerTable::DrawStackedOpacitySlider() {
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextDisabled("TRANSPARENCY");
		ImGui::TableNextColumn();

		bool transparencyEnabled = UIOPTION.isMeterTransparencyEnabled();
		if (ImGui::Checkbox("Enabled###StackedTransparency", &transparencyEnabled)) {
			UIOPTION.SetMeterTransparencyEnabled(transparencyEnabled);
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Enable or disable the transparent meter surface");
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::TextDisabled("OPACITY");
		ImGui::TableNextColumn();

		float opacity = UIOPTION.GetMeterOpacity();
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::SliderFloat("###StackedOpacity", &opacity, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
			UIOPTION.SetMeterOpacity(opacity);
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Drag left for more transparent, right for more solid");
		}
	}

void PlayerTable::SetupFontScale() {

	ImFont* font = ImGui::GetFont();

	_globalFontScale = font->Scale;
	_columnFontScale = _globalFontScale * UIOPTION.GetColumnFontScale();
	_tableFontScale = _globalFontScale * UIOPTION.GetTableFontScale();
}

void PlayerTable::ResizeTalbe() {
	_tableResize = TRUE;
}

void PlayerTable::Update() {

	DAMAGEMETER.GetLock();
	{
		ImGuiStyle& style = ImGui::GetStyle();

		ImVec4 prevInActiveColor = style.Colors[10];
		ImVec4 prevActiveColor = style.Colors[11];

		if (DAMAGEMETER.isRun()) {
			style.Colors[10] = UIOPTION.GetActiveColor();
			style.Colors[11] = UIOPTION.GetActiveColor();
		}
		else {
			style.Colors[10] = UIOPTION.GetInActiveColor();
			style.Colors[11] = UIOPTION.GetInActiveColor();
		}

		_accumulatedTime += UIWINDOW.GetDeltaTime();

		if (_accumulatedTime > UIOPTION.GetRefreshTime()) {
			_tableTime = static_cast<float>(((double)DAMAGEMETER.GetTime()) / 1000);
			_accumulatedTime = 0;
		}

		SetupFontScale();

		const bool tableOverlayMode = UIOPTION.isTableOverlayMode();
		const bool stackedMeterMode = UIOPTION.isStackedMeterMode();
		const float meterOpacity = UIOPTION.GetMeterOpacity();
		const float effectiveOpacity = UIOPTION.isMeterTransparencyEnabled() ? meterOpacity : 1.0f;
		const ImVec4 transparent = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		const ImVec4 overlayHeader = ImVec4(0.02f, 0.06f, 0.08f, 0.12f);
		const ImVec4 overlayBorder = ImVec4(0.32f, 0.55f, 0.68f, 0.22f);
		ImVec4 meterWindowBg = UIOPTION.GetWindowBGColor();
		meterWindowBg.w = effectiveOpacity;
		ImVec4 meterChildBg = style.Colors[ImGuiCol_ChildBg];
		meterChildBg.w = effectiveOpacity;
		ImVec4 meterPopupBg = style.Colors[ImGuiCol_PopupBg];
		meterPopupBg.w = effectiveOpacity;
		ImVec4 meterMenuBg = style.Colors[ImGuiCol_MenuBarBg];
		meterMenuBg.w = effectiveOpacity;
		ImVec4 meterTitleBg = style.Colors[ImGuiCol_TitleBg];
		meterTitleBg.w = effectiveOpacity;
		ImVec4 meterTitleBgActive = style.Colors[ImGuiCol_TitleBgActive];
		meterTitleBgActive.w = effectiveOpacity;
		ImVec4 meterTitleBgCollapsed = style.Colors[ImGuiCol_TitleBgCollapsed];
		meterTitleBgCollapsed.w = effectiveOpacity;
		ImVec4 meterFrameBg = style.Colors[ImGuiCol_FrameBg];
		meterFrameBg.w = effectiveOpacity;
		ImVec4 meterFrameBgHovered = style.Colors[ImGuiCol_FrameBgHovered];
		meterFrameBgHovered.w = effectiveOpacity;
		ImVec4 meterFrameBgActive = style.Colors[ImGuiCol_FrameBgActive];
		meterFrameBgActive.w = effectiveOpacity;
		ImVec4 meterButton = style.Colors[ImGuiCol_Button];
		meterButton.w = effectiveOpacity;
		ImVec4 meterButtonHovered = style.Colors[ImGuiCol_ButtonHovered];
		meterButtonHovered.w = effectiveOpacity;
		ImVec4 meterButtonActive = style.Colors[ImGuiCol_ButtonActive];
		meterButtonActive.w = effectiveOpacity;
		ImVec4 meterHeader = style.Colors[ImGuiCol_Header];
		meterHeader.w = effectiveOpacity;
		ImVec4 meterHeaderHovered = style.Colors[ImGuiCol_HeaderHovered];
		meterHeaderHovered.w = effectiveOpacity;
		ImVec4 meterHeaderActive = style.Colors[ImGuiCol_HeaderActive];
		meterHeaderActive.w = effectiveOpacity;
		ImVec4 meterHeaderBg = style.Colors[ImGuiCol_TableHeaderBg];
		meterHeaderBg.w = effectiveOpacity;
		ImVec4 meterRowBg = style.Colors[ImGuiCol_TableRowBg];
		meterRowBg.w = effectiveOpacity;
		ImVec4 meterRowBgAlt = style.Colors[ImGuiCol_TableRowBgAlt];
		meterRowBgAlt.w = effectiveOpacity;
		ImVec4 meterBorderStrong = style.Colors[ImGuiCol_TableBorderStrong];
		meterBorderStrong.w = effectiveOpacity;
		ImVec4 meterBorderLight = style.Colors[ImGuiCol_TableBorderLight];
		meterBorderLight.w = effectiveOpacity;
		ImVec4 meterBorder = style.Colors[ImGuiCol_Border];
		meterBorder.w = effectiveOpacity;
		// Opacity controls the meter surface only. Keep all labels and values
		// fully opaque so dark/transparent backgrounds never wash out text.
		ImVec4 meterText = style.Colors[ImGuiCol_Text];
		meterText.w = 1.0f;
		ImVec4 meterTextDisabled = style.Colors[ImGuiCol_TextDisabled];
		meterTextDisabled.w = 1.0f;

		ImGuiWindowFlags windowFlag = ImGuiWindowFlags_None;
		windowFlag |= (ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoCollapse);
		if (stackedMeterMode) {
			// Clear any normal-layout size remembered by ImGui and cap the
			// initial width. Without AlwaysAutoResize the user can subsequently
			// resize the stacked window from every edge and corner.
			if (!_wasStackedMeterMode)
				ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
			// The meter is the application's detached/main viewport, so using its
			// size here would make the current height its own maximum. Query the
			// actual monitor work area instead.
			float displayHeight = 0.0f;
			MONITORINFO monitorInfo = {};
			monitorInfo.cbSize = sizeof(monitorInfo);
			const HMONITOR monitor = ::MonitorFromWindow(UIWINDOW.GetHWND(), MONITOR_DEFAULTTONEAREST);
			if (monitor != NULL && ::GetMonitorInfo(monitor, &monitorInfo))
				displayHeight = static_cast<float>(monitorInfo.rcWork.bottom - monitorInfo.rcWork.top);
			if (displayHeight <= 0.0f) {
				const ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
				if (platformIO.Monitors.Size > 0)
					displayHeight = platformIO.Monitors[0].WorkSize.y;
			}
			if (displayHeight <= 0.0f)
				displayHeight = 720.0f;
			const float maxStackedHeight = displayHeight > 264.0f ? displayHeight - 24.0f : 240.0f;
			ImGui::SetNextWindowSizeConstraints(ImVec2(STACKED_METER_WIDTH, 0.0f), ImVec2(STACKED_METER_MAX_WIDTH, maxStackedHeight));
		}
		else {
			windowFlag |= ImGuiWindowFlags_NoScrollbar;
		}
		if (tableOverlayMode) {
			windowFlag |= (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
			ImGui::SetNextWindowBgAlpha(0.0f);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, transparent);
			ImGui::PushStyleColor(ImGuiCol_ChildBg, transparent);
			ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, overlayHeader);
			ImGui::PushStyleColor(ImGuiCol_TableRowBg, transparent);
			ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, transparent);
			ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, overlayBorder);
			ImGui::PushStyleColor(ImGuiCol_TableBorderLight, overlayBorder);
			ImGui::PushStyleColor(ImGuiCol_Border, transparent);
			ImGui::PushStyleColor(ImGuiCol_Text, meterText);
			ImGui::PushStyleColor(ImGuiCol_TextDisabled, meterTextDisabled);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		}
		else {
			windowFlag |= ImGuiWindowFlags_MenuBar;
			ImGui::SetNextWindowBgAlpha(meterWindowBg.w);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, meterWindowBg);
			ImGui::PushStyleColor(ImGuiCol_ChildBg, meterChildBg);
			ImGui::PushStyleColor(ImGuiCol_PopupBg, meterPopupBg);
			ImGui::PushStyleColor(ImGuiCol_MenuBarBg, meterMenuBg);
			ImGui::PushStyleColor(ImGuiCol_TitleBg, meterTitleBg);
			ImGui::PushStyleColor(ImGuiCol_TitleBgActive, meterTitleBgActive);
			ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, meterTitleBgCollapsed);
			ImGui::PushStyleColor(ImGuiCol_FrameBg, meterFrameBg);
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, meterFrameBgHovered);
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, meterFrameBgActive);
			ImGui::PushStyleColor(ImGuiCol_Button, meterButton);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, meterButtonHovered);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, meterButtonActive);
			ImGui::PushStyleColor(ImGuiCol_Header, meterHeader);
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, meterHeaderHovered);
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, meterHeaderActive);
			ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, meterHeaderBg);
			ImGui::PushStyleColor(ImGuiCol_TableRowBg, meterRowBg);
			ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, meterRowBgAlt);
			ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, meterBorderStrong);
			ImGui::PushStyleColor(ImGuiCol_TableBorderLight, meterBorderLight);
			ImGui::PushStyleColor(ImGuiCol_Border, meterBorder);
			ImGui::PushStyleColor(ImGuiCol_Text, meterText);
			ImGui::PushStyleColor(ImGuiCol_TextDisabled, meterTextDisabled);
		}

		ImGui::Begin("SoulMeter###DamageMeter", 0, windowFlag);
		{
			if (!stackedMeterMode && (!_windowSizeInitialized || _tableResize))
				SetWindowSize();

			if (!stackedMeterMode) {
				StoreWindowWidth();
				_windowSizeInitialized = true;
			}

			SetMainWindowSize();

			BeginPopupMenu();
			if (!tableOverlayMode) {
				DrawMenuBar();
				DrawStatusStrip();
			}

			ImGui::OutlineText::PushOutlineText(ImGui::IMGUIOUTLINETEXT(UIOPTION.GetOutlineColor(), 1));
			ImGui::TextAlignCenter::SetTextAlignCenter();
			{
				if (stackedMeterMode)
					SetupStackedMeter();
				else
					SetupTable();
			}
			ImGui::TextAlignCenter::UnSetTextAlignCenter();
			ImGui::OutlineText::PopOutlineText();
		}
		ImGui::End();
		_wasStackedMeterMode = stackedMeterMode;

		if (tableOverlayMode) {
			ImGui::PopStyleVar();
			ImGui::PopStyleColor(10);
		}
		else {
			ImGui::PopStyleColor(24);
		}

		ShowSelectedTable();

		style.Colors[10] = prevInActiveColor;
		style.Colors[11] = prevActiveColor;
	}
	DAMAGEMETER.FreeLock();
}

void PlayerTable::SetWindowSize() {

	_tableResize = FALSE;

	ImGuiStyle& style = ImGui::GetStyle();

	if (ImGui::GetScrollMaxY() > 0)
		_curWindowSize += ImGui::GetScrollMaxY();

	ImGui::SetWindowSize(ImVec2(UIOPTION.GetWindowWidth(), FLOOR(_curWindowSize)));
}

void PlayerTable::SetMainWindowSize() {

	auto pos = ImGui::GetWindowPos();
	auto size = ImGui::GetWindowSize();


	if (UIOPTION.isTopMost()) {
		SetWindowPos(UIWINDOW.GetHWND(), HWND_TOPMOST, static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(size.x + 1), static_cast<int>(size.y + 1), SWP_NOACTIVATE);
	}
	else {
		SetWindowPos(UIWINDOW.GetHWND(), HWND_NOTOPMOST, static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(size.x + 1), static_cast<int>(size.y + 1), SWP_NOACTIVATE);
	}

	//SetWindowPos(UIWINDOW.GetHWND(), HWND_NOTOPMOST, pos.x, pos.y, size.x + 1, size.y + 1, SWP_NOACTIVATE);
	
}

void PlayerTable::StoreWindowWidth() {
	UIOPTION.SetWindowWidth(ImGui::GetWindowSize().x);
}

void PlayerTable::DrawMenuBar() {

	if (!ImGui::BeginMenuBar()) {
		return;
	}

	ImGui::PushStyleColor(ImGuiCol_Text, SoulMeterTheme::Accent());
	ImGui::TextUnformatted("SOULMETER");
	ImGui::PopStyleColor();
	ImGui::SameLine(0.0f, 12.0f);
	ImGui::TextDisabled("v%s", APP_VERSION);
	ImGui::SameLine(0.0f, 12.0f);
	ImGui::TextDisabled("@Lithiaza");

	if (ImGui::BeginMenu("Meter")) {
		if (ImGui::MenuItem(LANGMANAGER.GetText("STR_MENU_RESET").data())) {
			DAMAGEMETER.Clear();
			ClearTable();
		}

		if (ImGui::MenuItem(LANGMANAGER.GetText("STR_MENU_TOPMOST").data(), nullptr, UIOPTION.isTopMost())) {
			UIOPTION.ToggleTopMost();
		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Views")) {
		if (ImGui::MenuItem(LANGMANAGER.GetText("STR_MENU_UTILL").data())) {
			UTILLWINDOW.OpenWindow();
		}

		if (ImGui::MenuItem("Table Overlay", nullptr, UIOPTION.isTableOverlayMode())) {
			UIOPTION.ToggleTableOverlayMode();
		}

		if (ImGui::MenuItem(LANGMANAGER.GetText("STR_MENU_MEOW").data())) {
			PLOTWINDOW.OpenWindow();
		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Settings")) {
		if (ImGui::MenuItem(LANGMANAGER.GetText("STR_MENU_OPTIONS").data())) {
			UIOPTION.OpenOption();
		}
		ImGui::EndMenu();
	}

	if (ImGui::MenuItem(LANGMANAGER.GetText("STR_MENU_EXIT").data())) {
		PostMessage(UIWINDOW.GetHWND(), WM_CLOSE, 0, 0);
	}

	ImGui::EndMenuBar();
}

void PlayerTable::DrawStatusStrip() {

	const bool isConnected = PipeReceiverIsConnected();
	const bool isLive = isConnected && DAMAGEMETER.isRun();

	ImGui::TextColored(isLive ? SoulMeterTheme::Live() : SoulMeterTheme::Idle(),
		"%s", isLive ? "LIVE" : "WAITING");
	ImGui::SameLine();
	ImGui::TextDisabled("|");
	ImGui::SameLine();

	if (!isConnected) {
		ImGui::TextDisabled("%s", LANGMANAGER.GetText("STR_WAITING_FOR_GAME").data());
		ImGui::Separator();
		return;
	}

	const uint64_t time = DAMAGEMETER.GetTime();
	ImGui::Text("%s", DAMAGEMETER.GetWorldName());
	ImGui::SameLine();
	// Keep the original timer precision in the status strip.  The custom
	// status layout previously rendered only minutes and seconds, even though
	// the Timer Accuracy option still controlled the millisecond precision.
	unsigned int milliseconds = static_cast<unsigned int>(time % 1000);
	if (DAMAGEMETER.mswideness == 1)
		milliseconds /= 100;
	else if (DAMAGEMETER.mswideness == 2)
		milliseconds /= 10;

	char timerLabel[64] = { 0 };
	sprintf_s(timerLabel, "| %02llu:%02llu.%u",
		time / (60 * 1000),
		(time / 1000) % 60,
		milliseconds);
	ImGui::TextDisabled("%s", timerLabel);

	char pingLabel[64] = { 0 };
	sprintf_s(pingLabel, "%s: %ums", LANGMANAGER.GetText("STR_MENU_PING").data(), DAMAGEMETER.GetPing());

	const float rightAlignedX = ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(pingLabel).x;
	if (rightAlignedX > ImGui::GetCursorPosX() + ImGui::GetStyle().ItemSpacing.x) {
		ImGui::SameLine(rightAlignedX);
	}
	else {
		ImGui::SameLine();
	}
	ImGui::TextDisabled("%s", pingLabel);
	ImGui::Separator();
}

void PlayerTable::BeginPopupMenu() {

	if (ImGui::BeginPopupContextWindow("###DamageMeterContext", ImGuiPopupFlags_MouseButtonRight)) {
		//if (ImGui::MenuItem(STR_MENU_RESUME)) {
		//	//
		//	//DAMAGEMETER.Toggle();
		//}

		if (ImGui::MenuItem(LANGMANAGER.GetText("STR_MENU_RESET").data())) {
			DAMAGEMETER.Clear();
			PLAYERTABLE.ClearTable();
		}

		if (ImGui::MenuItem(LANGMANAGER.GetText("STR_MENU_TOPMOST").data(), nullptr, UIOPTION.isTopMost())) {
			UIOPTION.ToggleTopMost();
		}

		if (ImGui::MenuItem("Table Overlay", nullptr, UIOPTION.isTableOverlayMode())) {
			UIOPTION.ToggleTableOverlayMode();
		}

		if (ImGui::MenuItem(LANGMANAGER.GetText("STR_MENU_UTILL").data())) {
			UTILLWINDOW.OpenWindow();
		}

		bool history_open = false;
		if (HISTORY.size() > 0)
			history_open = true;

		if (ImGui::BeginMenu(LANGMANAGER.GetText("STR_MENU_HISTORY").data(), history_open)) {
			HISTORY.GetLock();
			{
				int32_t i = static_cast<int32_t>(HISTORY.size()), iSelectedID = 0;
				bool bChangeHistory = false;
				HISTORY_INFO* pSelectedHI = nullptr;
				for (auto itr = HISTORY.rbegin(); itr != HISTORY.rend(); itr++)
				{
					HISTORY_INFO* pHI = (HISTORY_INFO*)*itr;

					char label[512] = { 0 };
					char mapName[MAX_MAP_LEN] = { 0 };
					SWDB.GetMapName(pHI->_worldID, mapName, MAX_MAP_LEN);

					sprintf_s(label, "%d.[%02d:%02d:%02d] %s - %02d:%02d.%01d###history%d",
						i,
						pHI->_saveTime->wHour, pHI->_saveTime->wMinute, pHI->_saveTime->wSecond,
						mapName,
						(unsigned int)pHI->_time / (60 * 1000), (unsigned int)(pHI->_time / 1000) % 60, (unsigned int)pHI->_time % 1000 / 100,
						i
					);

					i--;

					if (ImGui::Selectable(label, DAMAGEMETER.GetCurrentHistoryId() == i) && !DAMAGEMETER.isRun()) 
					{
						if (!DAMAGEMETER.isRun()) {
							bChangeHistory = true;
							iSelectedID = i;
							pSelectedHI = pHI;
						}
					}
				}

				if (bChangeHistory)
				{
					DAMAGEMETER.SetCurrentHistoryId(iSelectedID);
					DAMAGEMETER.SetHistory((LPVOID)pSelectedHI);
					bChangeHistory = false;
				}
				HISTORY.FreeLock();
			}
			ImGui::EndMenu();
		}

		if (ImGui::MenuItem(LANGMANAGER.GetText("STR_MENU_MEOW").data())) {
			PLOTWINDOW.OpenWindow();
		}

		if (ImGui::MenuItem(LANGMANAGER.GetText("STR_MENU_OPTIONS").data())) {
			UIOPTION.OpenOption();
		}

		if (ImGui::MenuItem(LANGMANAGER.GetText("STR_MENU_EXIT").data())) {
			PostMessage(UIWINDOW.GetHWND(), WM_CLOSE, 0, 0);
		}

		ImGui::EndPopup();
	}
}

void PlayerTable::SetupTable() {

	EnsureStackedRowsInitialized();

	ImGuiTableFlags tableFlags = ImGuiTableFlags_None;
	tableFlags |= (ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Resizable |
		ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ContextMenuInBody);
	if (!UIOPTION.isTableOverlayMode()) {
		tableFlags |= ImGuiTableFlags_RowBg;
	}

	const int columnSize = 53;
	char tableId[128] = { 0 };
	// Include the shared visibility mask in the table ID. ImGui otherwise
	// restores an older table-layout entry before the persisted Rows... mask
	// has a chance to apply, which makes selections appear to reset after a
	// relaunch.
	const std::string stackedRowsVisibility = GetStackedRowsVisibility();
	sprintf_s(tableId, "###Player Table-%s", stackedRowsVisibility.c_str());
	if (ImGui::BeginTable(tableId, columnSize, tableFlags)) {

		ImGuiTableColumnFlags columnFlags = ImGuiTableColumnFlags_None;
		columnFlags |= ImGuiTableColumnFlags_NoSort;

		ImGui::SetWindowFontScale(_columnFontScale);

		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_NAME").data(), StackedSyncedColumnFlags(StackedRowName, ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoClip | ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthFixed | columnFlags), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_DPS").data(), StackedSyncedColumnFlags(StackedRowDps, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_DAMAGE_PERCENT").data(), StackedSyncedColumnFlags(StackedRowDamagePercent, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_TOTAL_DAMAGE").data(), StackedSyncedColumnFlags(StackedRowDamage, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_TOTAL_HIT").data(), StackedSyncedColumnFlags(StackedRowHit, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_CRIT_RATE").data(), StackedSyncedColumnFlags(StackedRowCrit, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_HIT_PER_SECOND").data(), StackedSyncedColumnFlags(StackedRowHitPerSecond, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_CRIT_HIT_PER_SECOND").data(), StackedSyncedColumnFlags(StackedRowCritHitPerSecond, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_SKILL_PER_SECOND").data(), StackedSyncedColumnFlags(StackedRowSkillPerSecond, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn("Skill", StackedSyncedColumnFlags(StackedRowSkillPicker, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn("S.Hit", StackedSyncedColumnFlags(StackedRowSkillHit, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn("S.Crit", StackedSyncedColumnFlags(StackedRowSkillCrit, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn("S.Hit/s", StackedSyncedColumnFlags(StackedRowSkillHitPerSecond, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn("S.Hit/Cast", StackedSyncedColumnFlags(StackedRowSkillHitPerCast, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_MAX_COMBO").data(), StackedSyncedColumnFlags(StackedRowMaxCombo, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_ATTACK_CDMG_SUM").data(), StackedSyncedColumnFlags(StackedRowAttackCritDamage, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_SOUL_GAUGE").data(), StackedSyncedColumnFlags(StackedRowSoulGauge, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_ATTACK_SPEED").data(), StackedSyncedColumnFlags(StackedRowAttackSpeed, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_ARMOR_BREAK").data(), StackedSyncedColumnFlags(StackedRowArmorBreak, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_BOSS_DAMAGE").data(), StackedSyncedColumnFlags(StackedRowBossDamage, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_STAMINA").data(), StackedSyncedColumnFlags(StackedRowStamina, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_SOUL_VAPOR").data(), StackedSyncedColumnFlags(StackedRowSoulVapor, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_SOULSTONE_PERCENT").data(), StackedSyncedColumnFlags(StackedRowStoneDamageRate, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_SOULSTONE_PROC").data(), StackedSyncedColumnFlags(StackedRowStoneProc, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_SOULSTONE_DAMAGE").data(), StackedSyncedColumnFlags(StackedRowStoneDamage, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_AVERAGE_AB").data(), StackedSyncedColumnFlags(StackedRowAvgArmorBreak, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_AVERAGE_AB_U").data(), StackedSyncedColumnFlags(StackedRowAvgArmorBreakUncapped, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_AVERAGE_BD").data(), StackedSyncedColumnFlags(StackedRowAvgBossDamage, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_MISS").data(), StackedSyncedColumnFlags(StackedRowMiss, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_MISS_RATE").data(), StackedSyncedColumnFlags(StackedRowMissPercent, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_PARTIAL").data(), StackedSyncedColumnFlags(StackedRowMissDamage, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_GET_HIT_INCLUDE_ZERO_DAMAGE").data(), StackedSyncedColumnFlags(StackedRowGetHitAll, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_GET_HIT").data(), StackedSyncedColumnFlags(StackedRowGetHit, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_GET_HIT_BS").data(), StackedSyncedColumnFlags(StackedRowGetHitBS, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_EVADE_RATE_A").data(), StackedSyncedColumnFlags(StackedRowEvadeRateA, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_EVADE_RATE_B").data(), StackedSyncedColumnFlags(StackedRowEvadeRateB, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_GIGA_ENLIGHTEN").data(), StackedSyncedColumnFlags(StackedRowGigaEnlighten, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_TERA_ENLIGHTEN").data(), StackedSyncedColumnFlags(StackedRowTeraEnlighten, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_TERA_FEVER").data(), StackedSyncedColumnFlags(StackedRowTeraFever, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_TERA_FURY").data(), StackedSyncedColumnFlags(StackedRowTeraFury, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_TERA_BACKSTEP").data(), StackedSyncedColumnFlags(StackedRowTeraBackstep, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_TERA_TECHNIC").data(), StackedSyncedColumnFlags(StackedRowTeraTechnic, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_LOSED_HP").data(), StackedSyncedColumnFlags(StackedRowLosedHp, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_DODGE_COUNT").data(), StackedSyncedColumnFlags(StackedRowDodge, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_DEATH").data(), StackedSyncedColumnFlags(StackedRowDeath, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_FULL_AB_TIME").data(), StackedSyncedColumnFlags(StackedRowFullArmorBreakTime, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_FULL_AB_PERCENT").data(), StackedSyncedColumnFlags(StackedRowFullArmorBreakPercent, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_GIGA_ENLIGHTEN_SKILL_PERCENT").data(), StackedSyncedColumnFlags(StackedRowGigaEnlightenSkillPercent, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_TERA_ENLIGHTEN_SKILL_PERCENT").data(), StackedSyncedColumnFlags(StackedRowTeraEnlightenSkillPercent, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_AGGRO_TIME_PERCENT").data(), StackedSyncedColumnFlags(StackedRowAggroPercent, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_FULL_AS_TIME").data(), StackedSyncedColumnFlags(StackedRowFullAttackSpeedTime, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_FULL_AS_PERCENT").data(), StackedSyncedColumnFlags(StackedRowFullAttackSpeedPercent, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		ImGui::TableSetupColumn(LANGMANAGER.GetText("STR_TABLE_AVG_AS_PERCENT").data(), StackedSyncedColumnFlags(StackedRowAvgAttackSpeedPercent, columnFlags | ImGuiTableColumnFlags_WidthFixed), -1);
		//ImGuiTableColumnFlags_WidthStretch

		ImGui::TableHeadersRow();

		float window_width = ImGui::GetWindowWidth();

		ImGui::SetWindowFontScale(_tableFontScale);

		UpdateTable(window_width);
		if (SyncStackedRowsFromNormalTable())
			UIOPTION.SaveOption(TRUE);

		ImGui::SetWindowFontScale(_globalFontScale);

		ImGui::EndTable();
	}

}

void PlayerTable::SetupStackedMeter() {

	EnsureStackedRowsInitialized();

	ImGuiTableFlags tableFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ContextMenuInBody |
		ImGuiTableFlags_Resizable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Reorderable;
	if (!UIOPTION.isTableOverlayMode()) {
		tableFlags |= ImGuiTableFlags_RowBg;
	}

	if (ImGui::BeginTable("###Player Stack", 2, tableFlags, ImVec2(STACKED_METER_WIDTH, 0.0f))) {
		ImGuiTableColumnFlags columnFlags = ImGuiTableColumnFlags_NoSort | ImGuiTableColumnFlags_NoReorder | ImGuiTableColumnFlags_NoHide;
		ImGui::TableSetupColumn("Metric", columnFlags | ImGuiTableColumnFlags_WidthFixed, STACKED_METER_LABEL_WIDTH);
		ImGui::TableSetupColumn("Value", columnFlags | ImGuiTableColumnFlags_WidthStretch, 1.0f);

		ImGui::SetWindowFontScale(_tableFontScale);
		ImGui::TextAlignCenter::UnSetTextAlignCenter();
		UpdateStackedMeter(STACKED_METER_WIDTH);
		ImGui::TextAlignCenter::SetTextAlignCenter();
		ImGui::SetWindowFontScale(_globalFontScale);

		ImGui::EndTable();
	}
}

void PlayerTable::DrawStackedMetric(const char* metric, const char* value, bool selectable, uint32_t playerId, ImU32 barColor, float barPercent) {

	ImGui::TableNextRow();
	ImGui::TableNextColumn();

	if (barColor != 0) {
		DrawBar(STACKED_METER_WIDTH, barPercent, barColor);
	}

	ImGui::TextDisabled("%s", metric);
	ImGui::TableNextColumn();

	if (selectable) {
		char selectableLabel[256] = { 0 };
		sprintf_s(selectableLabel, "%s###StackedPlayer%u%s", value, playerId, metric);
		if (ImGui::Selectable(selectableLabel, false)) {
			ToggleSelectInfo(playerId);
		}
	}
	else {
		const float textWidth = ImGui::CalcTextSize(value).x;
		const float rightAlignedX = ImGui::GetCursorPosX() + ImGui::GetColumnWidth() - textWidth - ImGui::GetStyle().CellPadding.x;
		if (rightAlignedX > ImGui::GetCursorPosX()) {
			ImGui::SetCursorPosX(rightAlignedX);
		}
		ImGui::TextUnformatted(value);
	}
}

void PlayerTable::UpdateStackedMeter(float windowWidth) {

	(void)windowWidth;

	uint64_t maxDamage = 1;
	std::string youLabel = std::string(LANGMANAGER.GetText("STR_TABLE_YOU"));
	std::string unknownLabel = std::string(LANGMANAGER.GetText("PLAYER_NAME_CANT_FIND"));

	for (auto itr = DAMAGEMETER.begin(); itr != DAMAGEMETER.end(); itr++) {
		const char* name = DAMAGEMETER.GetPlayerName((*itr)->GetID());
		if (UIOPTION.isSoloMode() && strcmp(name, youLabel.c_str()) != 0)
			continue;
		if (strcmp(name, unknownLabel.c_str()) == 0)
			continue;
		if ((*itr)->GetDamage() > maxDamage)
			maxDamage = (*itr)->GetDamage();
	}

	bool drewPlayer = false;
	bool firstPlotPlayer = true;

	for (auto itr = DAMAGEMETER.begin(); itr != DAMAGEMETER.end(); itr++) {
		const uint32_t playerId = (*itr)->GetID();
		const char* rawPlayerName = DAMAGEMETER.GetPlayerName(playerId);

		if (UIOPTION.isSoloMode() && strcmp(rawPlayerName, youLabel.c_str()) != 0)
			continue;
		if (strcmp(rawPlayerName, unknownLabel.c_str()) == 0)
			continue;

		const bool isYou = strcmp(rawPlayerName, youLabel.c_str()) == 0;
		const char* playerName = rawPlayerName;
		if (UIOPTION.doHideName() && !isYou)
			playerName = "";

		float damagePercent = static_cast<float>((double)(*itr)->GetDamage() / (double)maxDamage);
		if (damagePercent > 1.0f)
			damagePercent = 1.0f;
		else if (damagePercent < 0.0f)
			damagePercent = 0.0f;

		char value[128] = { 0 };
		char comma[128] = { 0 };
		uint64_t milliTableTime = (uint64_t)((double)_tableTime * 1000);
		if (IsStackedRowVisible(StackedRowName))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_NAME").data(), playerName, true, playerId, UIOPTION.GetJobColor(DAMAGEMETER.GetPlayerJob(playerId)), damagePercent);

		if (DAMAGEMETER.GetPlayerTotalDamage() == 0)
			sprintf_s(value, "%.0lf", 0.0);
		else
			sprintf_s(value, "%.0lf", ((double)(*itr)->GetDamage() / (double)DAMAGEMETER.GetPlayerTotalDamage()) * 100);
		if (IsStackedRowVisible(StackedRowDamagePercent))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_DAMAGE_PERCENT").data(), value);

		FormatDpsValue((*itr)->GetDamage(), _tableTime, value, sizeof(value));
		if (IsStackedRowVisible(StackedRowDps))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_DPS").data(), value);
		if (_tableTime >= 1.0f) {
			double dps = ((double)(*itr)->GetDamage()) / _tableTime;
			if (UIOPTION.is1K())
				dps /= 1000;
			else if (UIOPTION.is1M())
				dps /= 1000000;
			else if (UIOPTION.is10K())
				dps /= 10000;
			PLOTWINDOW.AddData(playerId, DAMAGEMETER.GetPlayerName(playerId), dps, _tableTime, firstPlotPlayer);
			firstPlotPlayer = false;
		}

		FormatScaledUnsigned((*itr)->GetDamage(), value, sizeof(value));
		if (IsStackedRowVisible(StackedRowDamage))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_TOTAL_DAMAGE").data(), value);

		FormatCommaUnsigned((*itr)->GetHitCount(), value, sizeof(value));
		if (IsStackedRowVisible(StackedRowHit))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_TOTAL_HIT").data(), value);

		float crit = 0.0f;
		if ((*itr)->GetHitCount() != 0)
			crit = (float)(*itr)->GetCritHitCountForCritRate() / (float)(*itr)->GetHitCountForCritRate() * 100.0f;
		sprintf_s(value, "%.1f", crit);
		if (IsStackedRowVisible(StackedRowCrit))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_CRIT_RATE").data(), value);

		if (_tableTime == 0.0f)
			sprintf_s(value, "%d", 0);
		else
			sprintf_s(value, "%.2lf", (double)(*itr)->GetHitCount() / _tableTime);
		if (IsStackedRowVisible(StackedRowHitPerSecond))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_HIT_PER_SECOND").data(), value);

		if (_tableTime == 0.0f)
			sprintf_s(value, "%d", 0);
		else
			sprintf_s(value, "%.2lf", (double)(*itr)->GetCritHitCount() / _tableTime);
		if (IsStackedRowVisible(StackedRowCritHitPerSecond))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_CRIT_HIT_PER_SECOND").data(), value);

			if (_tableTime == 0.0f)
				strcpy_s(value, "-");
			else
				sprintf_s(value, "%.2lf", (double)(*itr)->GetSkillUsed() / _tableTime);
			if (IsStackedRowVisible(StackedRowSkillPerSecond))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_SKILL_PER_SECOND").data(), value);

			const size_t skillSlotCount = GetTrackedSkillSlotCount(playerId);
			for (size_t slotIndex = 0; slotIndex < skillSlotCount; slotIndex++) {
				TRACKED_SKILL_SUMMARY trackedSkill;
				if (BuildTrackedSkillSummary(*itr, trackedSkill, slotIndex)) {
					DrawStackedSkillBlock(*itr, trackedSkill, slotIndex);
				}
			}
			DrawStackedSkillAddRow(*itr);

			SWDamageMeter::SW_PLAYER_METADATA* playerMetaData = DAMAGEMETER.GetPlayerMetaData(playerId);

		sprintf_s(value, "%u", (*itr)->GetMaxCombo());
		if (IsStackedRowVisible(StackedRowMaxCombo))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_MAX_COMBO").data(), value);

		if (playerMetaData != NULL && isYou && _tableTime >= 1.0f) {
			double attackCrit = (double)playerMetaData->GetStat(StatType::MaxAttack) + (double)playerMetaData->GetStat(StatType::CritDamage);
			if (UIOPTION.is1K())
				attackCrit /= 1000;
			else if (UIOPTION.is10K())
				attackCrit /= 10000;
			else if (UIOPTION.is1M())
				attackCrit /= 1000000;
			sprintf_s(value, "%.0f", attackCrit);
		}
		else {
			strcpy_s(value, "-");
		}
		if (IsStackedRowVisible(StackedRowAttackCritDamage))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_ATTACK_CDMG_SUM").data(), value);

		if (playerMetaData != NULL) {
			sprintf_s(value, "%.1f", playerMetaData->GetStat(StatType::SG));
		}
		else {
			strcpy_s(value, "-");
		}
		if (IsStackedRowVisible(StackedRowSoulGauge))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_SOUL_GAUGE").data(), value);

		if (playerMetaData != NULL) {
			sprintf_s(value, "%.1f", playerMetaData->GetStat(StatType::AttackSpeed));
		}
		else {
			strcpy_s(value, "-");
		}
		if (IsStackedRowVisible(StackedRowAttackSpeed))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_ATTACK_SPEED").data(), value);
		if (playerMetaData != NULL && isYou && _tableTime >= 1.0f) {
			sprintf_s(value, "%.1f", playerMetaData->GetStat(StatType::ArmorBreak));
			PLOTWINDOW.AddAbData(playerMetaData->GetStat(StatType::ArmorBreak), _tableTime);
		}
		else {
			strcpy_s(value, "-");
		}
		if (IsStackedRowVisible(StackedRowArmorBreak))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_ARMOR_BREAK").data(), value);

		if (playerMetaData != NULL && isYou && _tableTime >= 1.0f) {
			sprintf_s(value, "%.1f", playerMetaData->GetSpecialStat(SpecialStatType::BossDamageAddRate));
			PLOTWINDOW.AddBdData(playerMetaData->GetSpecialStat(SpecialStatType::BossDamageAddRate), _tableTime);
		}
		else {
			strcpy_s(value, "-");
		}
		if (IsStackedRowVisible(StackedRowBossDamage))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_BOSS_DAMAGE").data(), value);

		if (playerMetaData != NULL && isYou && _tableTime >= 1.0f)
			sprintf_s(value, "%.1f", playerMetaData->GetStat(StatType::Stamina));
		else
			strcpy_s(value, "-");
		if (IsStackedRowVisible(StackedRowStamina))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_STAMINA").data(), value);

		if (playerMetaData != NULL && isYou && _tableTime >= 1.0f)
			sprintf_s(value, "%.1f", playerMetaData->GetStat(StatType::SV));
		else
			strcpy_s(value, "-");
		if (IsStackedRowVisible(StackedRowSoulVapor))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_SOUL_VAPOR").data(), value);

		double soulstoneAllPercent = (*itr)->GetDamage() == 0 ? 0.0 : ((double)(*itr)->GetSoulstoneDamage()) / (*itr)->GetDamage() * 100;
		sprintf_s(value, "%.1f", soulstoneAllPercent);
		if (IsStackedRowVisible(StackedRowStoneDamageRate))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_SOULSTONE_PERCENT").data(), value);

		double soulstoneProcRate = (*itr)->GetCritHitCountForCritRate() == 0 ? 0.0 : ((double)(*itr)->GetSoulstoneCount()) / (*itr)->GetHitCountForCritRate() * 100;
		sprintf_s(value, "%.1f", soulstoneProcRate);
		if (IsStackedRowVisible(StackedRowStoneProc))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_SOULSTONE_PROC").data(), value);

		double soulstoneDamage = (*itr)->GetDamageForSoulstone() == 0 ? 0.0 : ((double)(*itr)->GetSoulStoneDamageForSoulstone()) / (*itr)->GetDamageForSoulstone() * 100;
		sprintf_s(value, "%.1f", soulstoneDamage);
		if (IsStackedRowVisible(StackedRowStoneDamage))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_SOULSTONE_DAMAGE").data(), value);

		static double savedResultAB = 0;
		if (!isYou || _tableTime == 0) {
			strcpy_s(value, "-");
		}
		else if (DAMAGEMETER.isHistoryMode()) {
			savedResultAB = (*itr)->GetHistoryAvgAB();
			sprintf_s(value, "%.1f", savedResultAB);
		}
		else if (playerMetaData != NULL) {
			if ((int64_t)(milliTableTime - playerMetaData->_avgABPreviousTime) >= 0) {
				uint64_t timeDifference = (milliTableTime - playerMetaData->_avgABPreviousTime);
				double currentAB = playerMetaData->GetStat(StatType::ArmorBreak);
				currentAB = currentAB > 100.0 ? 100.0 : currentAB;
				savedResultAB = (double)(playerMetaData->_avgABSum + timeDifference * currentAB) / milliTableTime;
			}
			sprintf_s(value, "%.1f", savedResultAB);
		}
		if (IsStackedRowVisible(StackedRowAvgArmorBreak))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_AVERAGE_AB").data(), value);

		static double savedResultABU = 0;
		if (!isYou || _tableTime == 0) {
			strcpy_s(value, "-");
		}
		else if (DAMAGEMETER.isHistoryMode()) {
			savedResultABU = (*itr)->GetHistoryAvgABU();
			sprintf_s(value, "%.1f", savedResultABU);
		}
		else if (playerMetaData != NULL) {
			if ((int64_t)(milliTableTime - playerMetaData->_avgABPreviousTimeU) >= 0) {
				uint64_t timeDifference = (milliTableTime - playerMetaData->_avgABPreviousTimeU);
				double currentABU = playerMetaData->GetStat(StatType::ArmorBreak);
				savedResultABU = (double)(playerMetaData->_avgABSumU + timeDifference * currentABU) / milliTableTime;
			}
			sprintf_s(value, "%.1f", savedResultABU);
		}
		if (IsStackedRowVisible(StackedRowAvgArmorBreakUncapped))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_AVERAGE_AB_U").data(), value);

		static double savedResultBD = 0;
		if (!isYou || _tableTime == 0) {
			strcpy_s(value, "-");
		}
		else if (DAMAGEMETER.isHistoryMode()) {
			savedResultBD = (*itr)->GetHistoryAvgBD();
			sprintf_s(value, "%.1f", savedResultBD);
		}
		else if (playerMetaData != NULL) {
			if ((int64_t)(milliTableTime - playerMetaData->_avgBDPreviousTime) >= 0) {
				uint64_t timeDifference = (milliTableTime - playerMetaData->_avgBDPreviousTime);
				double currentBD = playerMetaData->GetSpecialStat(SpecialStatType::BossDamageAddRate);
				savedResultBD = (double)(playerMetaData->_avgBDSum + timeDifference * currentBD) / milliTableTime;
			}
			sprintf_s(value, "%.1f", savedResultBD);
		}
		if (IsStackedRowVisible(StackedRowAvgBossDamage))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_AVERAGE_BD").data(), value);

		FormatCommaUnsigned((*itr)->GetMissCount(), value, sizeof(value));
		if (IsStackedRowVisible(StackedRowMiss))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_MISS").data(), value);

		if ((*itr)->GetMissCount() == 0 || (*itr)->GetHitCountForCritRate() == 0)
			sprintf_s(value, "%.1f", 0.0);
		else
			sprintf_s(value, "%.1f", (double)(*itr)->GetMissCount() / (*itr)->GetHitCountForCritRate() * 100);
		if (IsStackedRowVisible(StackedRowMissPercent))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_MISS_RATE").data(), value);

		if (playerMetaData != NULL)
			sprintf_s(value, "%.1f", playerMetaData->GetStat(StatType::PartialDamage));
		else
			strcpy_s(value, "-");
		if (IsStackedRowVisible(StackedRowMissDamage))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_PARTIAL").data(), value);

		FormatCommaUnsigned((*itr)->GetGetHitAll(), value, sizeof(value));
		if (IsStackedRowVisible(StackedRowGetHitAll))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_GET_HIT_INCLUDE_ZERO_DAMAGE").data(), value);

		FormatCommaUnsigned((*itr)->GetGetHit(), value, sizeof(value));
		if (IsStackedRowVisible(StackedRowGetHit))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_GET_HIT").data(), value);

		FormatCommaUnsigned((*itr)->GetGetHitBS(), value, sizeof(value));
		if (IsStackedRowVisible(StackedRowGetHitBS))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_GET_HIT_BS").data(), value);

		if ((*itr)->GetGetHitAll() == 0)
			strcpy_s(value, "-");
		else
			sprintf_s(value, "%.1f%%", (double)(*itr)->GetGetHitMissed() / (*itr)->GetGetHitAll() * 100);
		if (IsStackedRowVisible(StackedRowEvadeRateA))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_EVADE_RATE_A").data(), value);

		if ((*itr)->GetGetHit() == 0)
			strcpy_s(value, "-");
		else
			sprintf_s(value, "%.1f%%", (double)(*itr)->GetGetHitMissedReal() / (*itr)->GetGetHit() * 100);
		if (IsStackedRowVisible(StackedRowEvadeRateB))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_EVADE_RATE_B").data(), value);

		if (isYou && _tableTime != 0.0f)
			sprintf_s(value, "%u", (*itr)->GetGigaEnlighten());
		else
			strcpy_s(value, "-");
		if (IsStackedRowVisible(StackedRowGigaEnlighten))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_GIGA_ENLIGHTEN").data(), value);

		if (isYou && _tableTime != 0.0f)
			sprintf_s(value, "%u", (*itr)->GetTeraEnlighten());
		else
			strcpy_s(value, "-");
		if (IsStackedRowVisible(StackedRowTeraEnlighten))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_TERA_ENLIGHTEN").data(), value);

		if (isYou && _tableTime != 0.0f)
			sprintf_s(value, "%u", (*itr)->GetBroochProc(BROOCH_FEVER));
		else
			strcpy_s(value, "-");
		if (IsStackedRowVisible(StackedRowTeraFever))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_TERA_FEVER").data(), value);

		if (isYou && _tableTime != 0.0f)
			sprintf_s(value, "%u", (*itr)->GetBroochProc(BROOCH_FURY));
		else
			strcpy_s(value, "-");
		if (IsStackedRowVisible(StackedRowTeraFury))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_TERA_FURY").data(), value);

		if (isYou && _tableTime != 0.0f)
			sprintf_s(value, "%u", (*itr)->GetBroochProc(BROOCH_BACKSTEP));
		else
			strcpy_s(value, "-");
		if (IsStackedRowVisible(StackedRowTeraBackstep))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_TERA_BACKSTEP").data(), value);

		if (isYou && _tableTime != 0.0f)
			sprintf_s(value, "%u", (*itr)->GetBroochProc(BROOCH_TECHNIC));
		else
			strcpy_s(value, "-");
		if (IsStackedRowVisible(StackedRowTeraTechnic))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_TERA_TECHNIC").data(), value);

		double losedHP = 0.0;
		if (DAMAGEMETER.isHistoryMode()) {
			losedHP = (*itr)->GetHistoryLosedHP();
		}
		else if (playerMetaData != NULL) {
			losedHP = playerMetaData->_losedHp;
		}
		if (UIOPTION.is1K())
			losedHP /= 1000;
		else if (UIOPTION.is1M())
			losedHP /= 1000000;
		else if (UIOPTION.is10K())
			losedHP /= 10000;
		sprintf_s(value, "%.0f", losedHP);
		TextCommma(value, comma);
		if (IsStackedRowVisible(StackedRowLosedHp))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_LOSED_HP").data(), comma);

		if (isYou && _tableTime != 0.0f)
			sprintf_s(value, "%u", (*itr)->GetDodgeUsed());
		else
			strcpy_s(value, "-");
		if (IsStackedRowVisible(StackedRowDodge))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_DODGE_COUNT").data(), value);

		sprintf_s(value, "%u", (*itr)->GetDeathCount());
		if (IsStackedRowVisible(StackedRowDeath))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_DEATH").data(), value);

		static double savedResultFullAB = 0;
		if (isYou) {
			if (DAMAGEMETER.isHistoryMode()) {
				savedResultFullAB = (*itr)->GetHistoryABTime();
			}
			else if (playerMetaData != NULL) {
				playerMetaData->CalcFullABTime(DAMAGEMETER.GetTime());
				savedResultFullAB = playerMetaData->_fullABTime;
			}
			sprintf_s(value, "%.1f", savedResultFullAB);
		}
		else {
			strcpy_s(value, "-");
		}
		if (IsStackedRowVisible(StackedRowFullArmorBreakTime))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_FULL_AB_TIME").data(), value);

		if (isYou && DAMAGEMETER.GetTime() != 0)
			sprintf_s(value, "%.0f", ((double)(savedResultFullAB * 1000) / DAMAGEMETER.GetTime()) * 100);
		else
			strcpy_s(value, "-");
		if (IsStackedRowVisible(StackedRowFullArmorBreakPercent))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_FULL_AB_PERCENT").data(), value);

		if (!isYou || _tableTime == 0 || (*itr)->GetSkillUsed() <= 0)
			strcpy_s(value, "-");
		else
			sprintf_s(value, "%.1f", ((double)(*itr)->GetGigaEnlighten() / (*itr)->GetSkillUsed()) * 100);
		if (IsStackedRowVisible(StackedRowGigaEnlightenSkillPercent))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_GIGA_ENLIGHTEN_SKILL_PERCENT").data(), value);

		if (!isYou || _tableTime == 0 || (*itr)->GetSkillUsed() <= 0)
			strcpy_s(value, "-");
		else
			sprintf_s(value, "%.1f", ((double)(*itr)->GetTeraEnlighten() / (*itr)->GetSkillUsed()) * 100);
		if (IsStackedRowVisible(StackedRowTeraEnlightenSkillPercent))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_TERA_ENLIGHTEN_SKILL_PERCENT").data(), value);

		static double savedResultAggroTime = 0;
		if (DAMAGEMETER.GetTime() != 0) {
			if (DAMAGEMETER.isHistoryMode()) {
				savedResultAggroTime = (*itr)->GetHistoryAggroTime();
			}
			else if (playerMetaData != NULL) {
				playerMetaData->CalcAggroTime(DAMAGEMETER.GetTime());
				savedResultAggroTime = playerMetaData->_AggroTime;
			}
			sprintf_s(value, "%.0f", ((double)(savedResultAggroTime * 1000) / DAMAGEMETER.GetTime()) * 100);
		}
		else {
			strcpy_s(value, "-");
		}
		if (IsStackedRowVisible(StackedRowAggroPercent))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_AGGRO_TIME_PERCENT").data(), value);

		static double savedResultFullAS = 0;
		if (isYou) {
			if (DAMAGEMETER.isHistoryMode()) {
				savedResultFullAS = (*itr)->GetHistoryASTime();
			}
			else if (playerMetaData != NULL) {
				playerMetaData->CalcFullASTime(DAMAGEMETER.GetTime());
				savedResultFullAS = playerMetaData->_fullASTime;
			}
			sprintf_s(value, "%.1f", savedResultFullAS);
		}
		else {
			strcpy_s(value, "-");
		}
		if (IsStackedRowVisible(StackedRowFullAttackSpeedTime))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_FULL_AS_TIME").data(), value);

		if (isYou && DAMAGEMETER.GetTime() != 0)
			sprintf_s(value, "%.0f", ((double)(savedResultFullAS * 1000) / DAMAGEMETER.GetTime()) * 100);
		else
			strcpy_s(value, "-");
		if (IsStackedRowVisible(StackedRowFullAttackSpeedPercent))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_FULL_AS_PERCENT").data(), value);

		static double savedResultAS = 0;
		if (!isYou || _tableTime == 0) {
			strcpy_s(value, "-");
		}
		else if (DAMAGEMETER.isHistoryMode()) {
			savedResultAS = (*itr)->GetHistoryAvgAS();
			sprintf_s(value, "%.1f", savedResultAS);
		}
		else if (playerMetaData != NULL) {
			if ((int64_t)(milliTableTime - playerMetaData->_avgASPreviousTime) >= 0) {
				uint64_t timeDifference = (milliTableTime - playerMetaData->_avgASPreviousTime);
				double currentAS = playerMetaData->GetStat(StatType::AttackSpeed);
				savedResultAS = (double)(playerMetaData->_avgASSum + timeDifference * currentAS) / milliTableTime;
			}
			sprintf_s(value, "%.1f", savedResultAS);
		}
		if (IsStackedRowVisible(StackedRowAvgAttackSpeedPercent))
			DrawStackedMetric(LANGMANAGER.GetText("STR_TABLE_AVG_AS_PERCENT").data(), value);

		PLOTWINDOW.AddJqData((*itr)->GetJqStack(), _tableTime);
		drewPlayer = true;
	}

	if (!drewPlayer) {
		DrawStackedMetric("STATUS", LANGMANAGER.GetText("STR_WAITING_FOR_GAME").data());
	}

	// These are meter-wide controls, so keep one copy at the bottom rather
	// than repeating them once per player block.
	DrawStackedRowsMenu();
	DrawStackedOpacitySlider();
}

void PlayerTable::UpdateTable(float windowWidth) {
	uint64_t max_Damage = 1;
	char comma[128] = { 0 }; char label[128] = { 0 };

	for (auto itr = DAMAGEMETER.begin(); itr != DAMAGEMETER.end(); itr++) {

		// 
		if (UIOPTION.isSoloMode() && DAMAGEMETER.GetPlayerName((*itr)->GetID()) != LANGMANAGER.GetText("STR_TABLE_YOU")) {
			continue;
		}

		// Skip Unknown Player
		if (DAMAGEMETER.GetPlayerName((*itr)->GetID()) == LANGMANAGER.GetText("PLAYER_NAME_CANT_FIND").data())
			continue;

		// 
		if (itr == DAMAGEMETER.begin())
			max_Damage = (*itr)->GetDamage();

		float damage_percent = static_cast<float>((double)(*itr)->GetDamage() / (double)max_Damage);

		if (damage_percent > 1)
			damage_percent = 1;
		else if (damage_percent < 0)
			damage_percent = 0;

		ImGui::TableNextRow();
		ImGui::TableNextColumn();


		DrawBar(windowWidth, damage_percent, UIOPTION.GetJobColor(DAMAGEMETER.GetPlayerJob((*itr)->GetID())));
		uint64_t milliTableTime = (uint64_t)((double)_tableTime * 1000);

		// NAME
		const char* playerName = DAMAGEMETER.GetPlayerName((*itr)->GetID());
		if (UIOPTION.doHideName() && playerName != LANGMANAGER.GetText("STR_TABLE_YOU").data()) {
			playerName = "";
		}

		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4 saved = ImVec4(style.Colors[0].x, style.Colors[0].y, style.Colors[0].z, style.Colors[0].w);
		
		uint32_t playerId = (*itr)->GetID();
		if (playerId == DAMAGEMETER.GetAggro()) {
			style.Colors[0] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
		}
		else if (playerId == DAMAGEMETER.GetOwnerID(DAMAGEMETER.GetAggro())) {
			style.Colors[0] = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
		}

		if (DAMAGEMETER.PlayerInAwakening(playerId)) {
			style.Colors[0] = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
		}
		bool useImage = UIOPTION.isUseImage();


		Texture playerTexture = DIRECTX11.getcharacterTexture(DAMAGEMETER.GetPlayerJob((*itr)->GetID()));

		if (useImage && playerTexture.ptr) {
			ImGui::SetCursorPosX((ImGui::GetColumnWidth() * 0.5f) - ((ImGui::CalcTextSize(playerName).x + playerTexture.xSize) / 2)); //because font size can change, we need to use texture size to center properly
		}
		else
		{
			ImGui::SetCursorPosX(ImGui::GetColumnWidth() * 0.5f - (ImGui::CalcTextSize(playerName).x / 2)); // we dont use texture here so center text only, have to do it myself because disabled text centering for this part as it was breaking rendering
		}
		if (useImage && playerTexture.ptr) {
			ImGui::Image((void*)playerTexture.ptr, ImVec2((float)playerTexture.xSize, (float)playerTexture.ySize));
			ImGui::SameLine();
		}
		ImGui::TextAlignCenter::UnSetTextAlignCenter(); //some gay custom function, breaks text align with image
		if (ImGui::Selectable(playerName, false, ImGuiSelectableFlags_SpanAllColumns))
			ToggleSelectInfo((*itr)->GetID());

		ImGui::TextAlignCenter::SetTextAlignCenter();

		ImGui::TableNextColumn();
		style.Colors[0] = saved;


		// DPS
		if (_tableTime < 1) {
			ImGui::Text("-");
		}
		else {
			double dps = ((double)(*itr)->GetDamage()) / _tableTime;
			if (UIOPTION.is1K()) 
				dps /= 1000;
			else if (UIOPTION.is1M()) 
				dps /= 1000000;
			else if (UIOPTION.is10K())
				dps /= 10000;
			if (UIOPTION.is1M()) 
				TextCommmaIncludeDecimal(dps, sizeof(comma), comma);
			else {
				sprintf_s(label, 128, "%.0lf", dps);
				TextCommma(label, comma);
			}
			if (UIOPTION.is1K())
				strcat_s(comma, 128, LANGMANAGER.GetText("STR_DISPLAY_UNIT_1K").data());
			else if (UIOPTION.is1M()) 
				strcat_s(comma, 128, LANGMANAGER.GetText("STR_DISPLAY_UNIT_1M").data());
			else if (UIOPTION.is10K())
				strcat_s(comma, 128, LANGMANAGER.GetText("STR_DISPLAY_UNIT_10K").data());
			ImGui::Text(comma);

			bool isFirstElement = ((itr - DAMAGEMETER.begin()) == 0);
			PLOTWINDOW.AddData((*itr)->GetID(), DAMAGEMETER.GetPlayerName((*itr)->GetID()), dps, _tableTime, isFirstElement);
		}
		

		ImGui::TableNextColumn();

		// D%
		if (DAMAGEMETER.GetPlayerTotalDamage() == 0) {
			sprintf_s(label, 128, "%.0lf", (float)0);
			ImGui::Text(label);
		}
		else {
			sprintf_s(label, 128, "%.0lf", ((double)(*itr)->GetDamage() / (double)DAMAGEMETER.GetPlayerTotalDamage()) * 100);
			ImGui::Text(label);
		}

		ImGui::TableNextColumn();

		// DAMAGE
		uint64_t damage = (*itr)->GetDamage();
		if (UIOPTION.is1K())
			damage /= 1000;
		else if (UIOPTION.is1M())
			damage /= 1000000;
		else if (UIOPTION.is10K())
			damage /= 10000;
		sprintf_s(label, 128, "%llu", damage);
		TextCommma(label, comma);
		if (UIOPTION.is1K())
			strcat_s(comma, 128, LANGMANAGER.GetText("STR_DISPLAY_UNIT_1K").data());
		else if (UIOPTION.is1M())
			strcat_s(comma, 128, LANGMANAGER.GetText("STR_DISPLAY_UNIT_1M").data());
		else if (UIOPTION.is10K())
			strcat_s(comma, 128, LANGMANAGER.GetText("STR_DISPLAY_UNIT_10K").data());
		ImGui::Text(comma);

		ImGui::TableNextColumn();

		// HIT
		sprintf_s(label, 128, "%d", (*itr)->GetHitCount());
		TextCommma(label, comma);
		ImGui::Text(comma);

		ImGui::TableNextColumn();

		// CRIT
		float crit = 0;

		if ((*itr)->GetHitCount() != 0)
			crit = (float)(*itr)->GetCritHitCountForCritRate() / (float)(*itr)->GetHitCountForCritRate() * 100;

		sprintf_s(label, 128, "%.1f", crit);
		ImGui::Text(label);

		ImGui::TableNextColumn();

		// HIT/S
		if (_tableTime == (float)0) {
			sprintf_s(label, 128, "%d", 0);
			ImGui::Text(label);
		}
		else {
			sprintf_s(label, 128, "%.2lf", (double)(*itr)->GetHitCount() / _tableTime);
			ImGui::Text(label);

		}
		ImGui::TableNextColumn();

		//CRIT/S
		if (_tableTime == (float)0) {
			sprintf_s(label, 128, "%d", 0);
			ImGui::Text(label);
		}
		else {
			sprintf_s(label, 128, "%.2lf", (double)(*itr)->GetCritHitCount() / _tableTime);
			ImGui::Text(label);
		}

		ImGui::TableNextColumn();

			// Skill/s
			if (_tableTime == 0.0f) {
				sprintf_s(label, 128, "-");
			ImGui::Text(label);
		}
		else {
			sprintf_s(label, 128, "%.2lf", (double)(*itr)->GetSkillUsed() / _tableTime);
			ImGui::Text(label);
			}

			ImGui::TableNextColumn();

			DrawTrackedSkillColumns(*itr);

			// MAXC
			sprintf_s(label, 128, "%d", (*itr)->GetMaxCombo());
		TextCommma(label, comma);
		ImGui::Text(comma);

		ImGui::TableNextColumn();

		uint32_t playerID = (*itr)->GetID();
		SWDamageMeter::SW_PLAYER_METADATA* playerMetaData = DAMAGEMETER.GetPlayerMetaData(playerID);

		// Not found stat data
		if (playerMetaData == NULL) {
			continue;
		}

		if (DAMAGEMETER.GetPlayerName((*itr)->GetID()) != LANGMANAGER.GetText("STR_TABLE_YOU").data() || _tableTime < 1) {
			// Attack+Crit SUM
			sprintf_s(label, 128, "-");
			ImGui::Text(label);
			ImGui::TableNextColumn();

			// SG
			sprintf_s(label, 128, "%.1f", playerMetaData->GetStat(StatType::SG));
			ImGui::Text(label);
			ImGui::TableNextColumn();
			// AttackSpeed
			sprintf_s(label, 128, "%.1f", playerMetaData->GetStat(StatType::AttackSpeed));
			ImGui::Text(label);
			ImGui::TableNextColumn();

			// AB
			sprintf_s(label, 128, "-");
			ImGui::Text(label);
			ImGui::TableNextColumn();
			// BD
			sprintf_s(label, 128, "-");
			ImGui::Text(label);
			ImGui::TableNextColumn();
			// STAM
			sprintf_s(label, 128, "-");
			ImGui::Text(label);
			ImGui::TableNextColumn();
			// SV
			sprintf_s(label, 128, "-");
			ImGui::Text(label);
			ImGui::TableNextColumn();
		}
		else {
			// Attack+Crit SUM
			// TODO: Re-enable M option if we ever get there 
			double gongchihap = (double)playerMetaData->GetStat(StatType::MaxAttack) + (double)playerMetaData->GetStat(StatType::CritDamage);
			if (UIOPTION.is1K())
				gongchihap /= 1000;
			else if (UIOPTION.is1M()) {
				if (!strcmp(LANGMANAGER.GetText("STR_DISPLAY_DEFAULT_UNIT").data(), "K"))
					gongchihap /= 1000;
				else if (!strcmp(LANGMANAGER.GetText("STR_DISPLAY_DEFAULT_UNIT").data(), "10K"))
					gongchihap /= 10000;
				// gongchihap /= 1000000
			}
			else if (UIOPTION.is10K())
				gongchihap /= 10000;
			sprintf_s(label, 128, "%.0f", gongchihap);
			TextCommma(label, comma);
			if (UIOPTION.is1K())
				strcat_s(comma, 128, LANGMANAGER.GetText("STR_DISPLAY_UNIT_1K").data());
			else if (UIOPTION.is1M()) {
				if (!strcmp(LANGMANAGER.GetText("STR_DISPLAY_DEFAULT_UNIT").data(), "K"))
					strcat_s(comma, 128, LANGMANAGER.GetText("STR_DISPLAY_UNIT_1K").data());
				else if (!strcmp(LANGMANAGER.GetText("STR_DISPLAY_DEFAULT_UNIT").data(), "10K"))
					strcat_s(comma, 128, LANGMANAGER.GetText("STR_DISPLAY_UNIT_10K").data());
				// strcat_s(comma, 128, LANGMANAGER.GetText("STR_DISPLAY_UNIT_1M"));
			}
			else if (UIOPTION.is10K())
				strcat_s(comma, 128, LANGMANAGER.GetText("STR_DISPLAY_UNIT_10K").data());
			ImGui::Text(comma);
			ImGui::TableNextColumn();

			static float statTmp = 0;

			// SG
			sprintf_s(label, 128, "%.1f", playerMetaData->GetStat(StatType::SG));
			ImGui::Text(label);

			ImGui::TableNextColumn();
			// AttackSpeed
			sprintf_s(label, 128, "%.1f", playerMetaData->GetStat(StatType::AttackSpeed));
			ImGui::Text(label);

			ImGui::TableNextColumn();
			// AB
			statTmp = playerMetaData->GetStat(StatType::ArmorBreak);
			sprintf_s(label, 128, "%.1f", statTmp);
			PLOTWINDOW.AddAbData(statTmp, _tableTime);
			ImGui::Text(label);
			
			ImGui::TableNextColumn();
			// BD
			statTmp = playerMetaData->GetSpecialStat(SpecialStatType::BossDamageAddRate);
			sprintf_s(label, 128, "%.1f", statTmp);
			PLOTWINDOW.AddBdData(statTmp, _tableTime);
			ImGui::Text(label);

			ImGui::TableNextColumn();
			// stamina
			sprintf_s(label, 128, "%.1f", playerMetaData->GetStat(StatType::Stamina));
			ImGui::Text(label);

			ImGui::TableNextColumn();
			// SV
			sprintf_s(label, 128, "%.1f", playerMetaData->GetStat(StatType::SV));
			ImGui::Text(label);
			ImGui::TableNextColumn();
		}



		// Soulstone all percent
		double soulstoneAllPercent;
		if ((*itr)->GetDamage() == 0) {
			soulstoneAllPercent = 0.0;
		}
		else {
			soulstoneAllPercent = ((double)(*itr)->GetSoulstoneDamage()) / (*itr)->GetDamage() * 100;
		}

		sprintf_s(label, 128, "%.1f", soulstoneAllPercent);
		ImGui::Text(label);
		ImGui::TableNextColumn();

		// Soulstone crit rate
		double soulstoneProcRate;
		if ((*itr)->GetCritHitCountForCritRate() == 0) {
			soulstoneProcRate = 0.0;
		}
		else {
			soulstoneProcRate = ((double)(*itr)->GetSoulstoneCount()) / (*itr)->GetHitCountForCritRate() * 100;
		}

		sprintf_s(label, 128, "%.1f", soulstoneProcRate);
		ImGui::Text(label);
		ImGui::TableNextColumn();

		// Soulstone damage %
		double soulstoneDamage;
		if ((*itr)->GetDamageForSoulstone() == 0) {
			soulstoneDamage = 0.0;
		}
		else {
			soulstoneDamage = ((double)(*itr)->GetSoulStoneDamageForSoulstone()) / (*itr)->GetDamageForSoulstone() * 100;
		}
		sprintf_s(label, 128, "%.1f", soulstoneDamage);
		ImGui::Text(label);
		ImGui::TableNextColumn();

		// history data tmp
		static double savedResultAB = 0;

		if (DAMAGEMETER.GetPlayerName((*itr)->GetID()) != LANGMANAGER.GetText("STR_TABLE_YOU").data() || _tableTime == 0) {
			sprintf_s(label, 128, "-");
		}
		else if (DAMAGEMETER.isHistoryMode()) {
			savedResultAB = (*itr)->GetHistoryAvgAB();
			sprintf_s(label, 128, "%.1f", savedResultAB);
		}
		else {

			if ((int64_t)(milliTableTime - playerMetaData->_avgABPreviousTime) < 0) {
				sprintf_s(label, 128, "%.1f", savedResultAB);
			}
			else {
				uint64_t timeDifference = (milliTableTime - playerMetaData->_avgABPreviousTime);
				double currentAB = playerMetaData->GetStat(StatType::ArmorBreak);
				currentAB = currentAB > 100.0 ? 100.0 : currentAB;
				uint64_t calculatedAvgAB = static_cast<uint64_t>((playerMetaData->_avgABSum + timeDifference * currentAB));

				savedResultAB = (double)calculatedAvgAB / milliTableTime;
				sprintf_s(label, 128, "%.1f", savedResultAB);
			}
		}

		ImGui::Text(label);
		ImGui::TableNextColumn();

		// history data tmp
		static double savedResultABU = 0;

		if (DAMAGEMETER.GetPlayerName((*itr)->GetID()) != LANGMANAGER.GetText("STR_TABLE_YOU").data() || _tableTime == 0) {
			sprintf_s(label, 128, "-");
		}
		else if (DAMAGEMETER.isHistoryMode()) {
			savedResultABU = (*itr)->GetHistoryAvgABU();
			sprintf_s(label, 128, "%.1f", savedResultABU);
		}
		else {

			if ((int64_t)(milliTableTime - playerMetaData->_avgABPreviousTimeU) < 0) {
				sprintf_s(label, 128, "%.1f", savedResultABU);
			}
			else {
				uint64_t timeDifference = (milliTableTime - playerMetaData->_avgABPreviousTimeU);
				double currentABU = playerMetaData->GetStat(StatType::ArmorBreak);
				uint64_t calculatedAvgABU = static_cast<uint64_t>((playerMetaData->_avgABSumU + timeDifference * currentABU));

				savedResultABU = (double)calculatedAvgABU / milliTableTime;
				sprintf_s(label, 128, "%.1f", savedResultABU);
			}
		}

		ImGui::Text(label);
		ImGui::TableNextColumn();

		// BD
		static double savedResultBD = 0;

		if (DAMAGEMETER.GetPlayerName((*itr)->GetID()) != LANGMANAGER.GetText("STR_TABLE_YOU").data() || _tableTime == 0) {
			sprintf_s(label, 128, "-");
		}
		else if (DAMAGEMETER.isHistoryMode()) {
			savedResultBD = (*itr)->GetHistoryAvgBD();
			sprintf_s(label, 128, "%.1f", savedResultBD);
		}
		else {

			if ((int64_t)(milliTableTime - playerMetaData->_avgBDPreviousTime) < 0) {
				sprintf_s(label, 128, "%.1f", savedResultBD);
			}
			else {
				uint64_t timeDifference = (milliTableTime - playerMetaData->_avgBDPreviousTime);
				double currentBD = playerMetaData->GetSpecialStat(SpecialStatType::BossDamageAddRate);
				uint64_t calculatedAvgBD = static_cast<uint64_t>((playerMetaData->_avgBDSum + timeDifference * currentBD));

				savedResultBD = (double)calculatedAvgBD / milliTableTime;
				sprintf_s(label, 128, "%.1f", savedResultBD);
			}
		}

		ImGui::Text(label);
		ImGui::TableNextColumn();
		// Miss
		sprintf_s(label, 128, "%d", (*itr)->GetMissCount());
		TextCommma(label, comma);
		ImGui::Text(comma);

		ImGui::TableNextColumn();
		// Miss%
		if ((*itr)->GetMissCount() == 0 || (*itr)->GetHitCountForCritRate() == 0) {
			sprintf_s(label, 128, "%.1f", 0.0);
		}
		else {
			sprintf_s(label, 128, "%.1f", (double)(*itr)->GetMissCount() / (*itr)->GetHitCountForCritRate() * 100);
		}

		ImGui::Text(label);
		ImGui::TableNextColumn();

		// MissDamageRate
		sprintf_s(label, 128, "%.1f", playerMetaData->GetStat(StatType::PartialDamage));
		ImGui::Text(label);
		ImGui::TableNextColumn();

		// GetHit(Include Zero Damage)
		sprintf_s(label, 128, "%d", (*itr)->GetGetHitAll());
		TextCommma(label, comma);
		ImGui::Text(comma);

		ImGui::TableNextColumn();

		// GetHit
		sprintf_s(label, 128, "%d", (*itr)->GetGetHit());
		TextCommma(label, comma);
		ImGui::Text(comma);

		ImGui::TableNextColumn();

		// GetHit(BS)
		sprintf_s(label, 128, "%d", (*itr)->GetGetHitBS());
		TextCommma(label, comma);
		ImGui::Text(comma);

		ImGui::TableNextColumn();


		// Evade A
		if ((*itr)->GetGetHitAll() == 0) {
			sprintf_s(label, 128, "-");
		}
		else {
			sprintf_s(label, 128, "%.1f%%", (double)(*itr)->GetGetHitMissed() / (*itr)->GetGetHitAll() * 100);
		}
		ImGui::Text(label);
		ImGui::TableNextColumn();

		// Evade B
		if ((*itr)->GetGetHit() == 0) {
			sprintf_s(label, 128, "-");
		}
		else {
			sprintf_s(label, 128, "%.1f%%", (double)(*itr)->GetGetHitMissedReal() / (*itr)->GetGetHit() * 100);
		}
		ImGui::Text(label);
		ImGui::TableNextColumn();


		// Enlighten
		if (DAMAGEMETER.GetPlayerName((*itr)->GetID()) != LANGMANAGER.GetText("STR_TABLE_YOU").data() || _tableTime == 0) {
			sprintf_s(label, 128, "-");
			ImGui::Text(label);
			ImGui::TableNextColumn();

			sprintf_s(label, 128, "-");
			ImGui::Text(label);
			ImGui::TableNextColumn();
		}
		else {
			sprintf_s(label, 128, "%u", (*itr)->GetGigaEnlighten());
			ImGui::Text(label);
			ImGui::TableNextColumn();

			sprintf_s(label, 128, "%u", (*itr)->GetTeraEnlighten());
			ImGui::Text(label);
			ImGui::TableNextColumn();
		}

		// Brooch procs (Fever / Fury / Backstep / Technic)
		{
			const BroochProc broochProcs[] = { BROOCH_FEVER, BROOCH_FURY, BROOCH_BACKSTEP, BROOCH_TECHNIC };
			bool isYou = DAMAGEMETER.GetPlayerName((*itr)->GetID()) == LANGMANAGER.GetText("STR_TABLE_YOU").data() && _tableTime != 0;

			for (BroochProc type : broochProcs) {
				if (!isYou)
					sprintf_s(label, 128, "-");
				else
					sprintf_s(label, 128, "%u", (*itr)->GetBroochProc(type));

				ImGui::Text(label);
				ImGui::TableNextColumn();
			}
		}

		// HP
		// TODO: Re-enable M if we ever get there
		double losedHP = 0.0;
		if (DAMAGEMETER.isHistoryMode()) {
			losedHP = (*itr)->GetHistoryLosedHP();
		}
		else {
			losedHP = playerMetaData->_losedHp;
		}
		if (UIOPTION.is1K())
			losedHP /= 1000;
		else if (UIOPTION.is1M()) {
			if (!strcmp(LANGMANAGER.GetText("STR_DISPLAY_DEFAULT_UNIT").data(), "K"))
				losedHP /= 1000;
			else if (!strcmp(LANGMANAGER.GetText("STR_DISPLAY_DEFAULT_UNIT").data(), "10K"))
				losedHP /= 10000;
			// losedHP /= 1000000;
		}
		else if (UIOPTION.is10K())
			losedHP /= 10000;
		sprintf_s(label, 128, "%.0f", losedHP);
		TextCommma(label, comma);

		if (UIOPTION.is1K())
			strcat_s(comma, 128, LANGMANAGER.GetText("STR_DISPLAY_UNIT_1K").data());
		else if (UIOPTION.is1M())
		{
			if (!strcmp(LANGMANAGER.GetText("STR_DISPLAY_DEFAULT_UNIT").data(), "K"))
				strcat_s(comma, 128, LANGMANAGER.GetText("STR_DISPLAY_UNIT_1K").data());
			else if (!strcmp(LANGMANAGER.GetText("STR_DISPLAY_DEFAULT_UNIT").data(), "10K"))
				strcat_s(comma, 128, LANGMANAGER.GetText("STR_DISPLAY_UNIT_10K").data());
			// strcat_s(comma, 128, LANGMANAGER.GetText("STR_DISPLAY_UNIT_1M"));
		}
		else if (UIOPTION.is10K())
			strcat_s(comma, 128, LANGMANAGER.GetText("STR_DISPLAY_UNIT_10K").data());

		ImGui::Text(comma);
		ImGui::TableNextColumn();

		// Dodge
		if (DAMAGEMETER.GetPlayerName((*itr)->GetID()) != LANGMANAGER.GetText("STR_TABLE_YOU").data() || _tableTime == 0) {
			sprintf_s(label, 128, "-");
			ImGui::Text(label);
			ImGui::TableNextColumn();
		}
		else {
			sprintf_s(label, 128, "%u", (*itr)->GetDodgeUsed());
			ImGui::Text(label);
			ImGui::TableNextColumn();
		}

		// Death Counter
		sprintf_s(label, 128, "%u", (*itr)->GetDeathCount());
		ImGui::Text(label);
		ImGui::TableNextColumn();

		static double savedResultFullAB = 0;
		// Full AB Time
		if (DAMAGEMETER.GetPlayerName((*itr)->GetID()) == LANGMANAGER.GetText("STR_TABLE_YOU").data()) {
			if (DAMAGEMETER.isHistoryMode()) {
				savedResultFullAB = (*itr)->GetHistoryABTime();
			}
			else {
				playerMetaData->CalcFullABTime(DAMAGEMETER.GetTime());
				savedResultFullAB = playerMetaData->_fullABTime;
			}
			sprintf_s(label, 128, "%.1f", savedResultFullAB);
		}
		else {
			sprintf_s(label, 128, "-");
		}
		ImGui::Text(label);
		ImGui::TableNextColumn();

		// Full AB Percent
		if (DAMAGEMETER.GetPlayerName((*itr)->GetID()) == LANGMANAGER.GetText("STR_TABLE_YOU").data()) {
			sprintf_s(label, 128, "%.0f", ((double)(savedResultFullAB * 1000) / DAMAGEMETER.GetTime()) * 100);
		}
		else {
			sprintf_s(label, 128, "-");
		}
		ImGui::Text(label);
		ImGui::TableNextColumn();

		// Enli/Skill(%)
		if (DAMAGEMETER.GetPlayerName((*itr)->GetID()) != LANGMANAGER.GetText("STR_TABLE_YOU").data() || _tableTime == 0 || (*itr)->GetSkillUsed() <= 0) {
			sprintf_s(label, 128, "-");
			ImGui::Text(label);
			ImGui::TableNextColumn();

			sprintf_s(label, 128, "-");
			ImGui::Text(label);
			ImGui::TableNextColumn();
		}
		else {
			sprintf_s(label, 128, "%.1f", ((double)(*itr)->GetGigaEnlighten() / (*itr)->GetSkillUsed()) * 100);
			ImGui::Text(label);
			ImGui::TableNextColumn();

			sprintf_s(label, 128, "%.1f", ((double)(*itr)->GetTeraEnlighten() / (*itr)->GetSkillUsed()) * 100);
			ImGui::Text(label);
			ImGui::TableNextColumn();
		}

		// Aggro Percent
		static double savedResultAggroTime = 0;
		if (DAMAGEMETER.isHistoryMode()) {
			savedResultAggroTime = (*itr)->GetHistoryAggroTime();
		}
		else {
			playerMetaData->CalcAggroTime(DAMAGEMETER.GetTime());
			savedResultAggroTime = playerMetaData->_AggroTime;
		}
		sprintf_s(label, 128, "%.0f", ((double)(savedResultAggroTime * 1000) / DAMAGEMETER.GetTime()) * 100);
		ImGui::Text(label);
		ImGui::TableNextColumn();

		static double savedResultFullAS = 0;
		// Full AS Time
		if (DAMAGEMETER.GetPlayerName((*itr)->GetID()) == LANGMANAGER.GetText("STR_TABLE_YOU").data()) {
			if (DAMAGEMETER.isHistoryMode()) {
				savedResultFullAS = (*itr)->GetHistoryASTime();
			}
			else {
				playerMetaData->CalcFullASTime(DAMAGEMETER.GetTime());
				savedResultFullAS = playerMetaData->_fullASTime;
			}
			sprintf_s(label, 128, "%.1f", savedResultFullAS);
		}
		else {
			sprintf_s(label, 128, "-");
		}
		ImGui::Text(label);
		ImGui::TableNextColumn();

		// Full AS Percent
		if (DAMAGEMETER.GetPlayerName((*itr)->GetID()) == LANGMANAGER.GetText("STR_TABLE_YOU").data()) {
			sprintf_s(label, 128, "%.0f", ((double)(savedResultFullAS * 1000) / DAMAGEMETER.GetTime()) * 100);
		}
		else {
			sprintf_s(label, 128, "-");
		}
		ImGui::Text(label);
		ImGui::TableNextColumn();

		// AS
		static double savedResultAS = 0;
		if (DAMAGEMETER.GetPlayerName((*itr)->GetID()) != LANGMANAGER.GetText("STR_TABLE_YOU").data() || _tableTime == 0) {
			sprintf_s(label, 128, "-");
		}
		else if (DAMAGEMETER.isHistoryMode()) {
			savedResultAS = (*itr)->GetHistoryAvgAS();
			sprintf_s(label, 128, "%.1f", savedResultAS);
		}
		else {

			if ((int64_t)(milliTableTime - playerMetaData->_avgASPreviousTime) < 0) {
				sprintf_s(label, 128, "%.1f", savedResultAS);
			}
			else {
				uint64_t timeDifference = (milliTableTime - playerMetaData->_avgASPreviousTime);
				double currentAS = playerMetaData->GetStat(StatType::AttackSpeed);
				uint64_t calculatedAvgAS = static_cast<uint64_t>((playerMetaData->_avgASSum + timeDifference * currentAS));

				savedResultAS = (double)calculatedAvgAS / milliTableTime;
				sprintf_s(label, 128, "%.1f", savedResultAS);
			}
		}
		ImGui::Text(label);
		ImGui::TableNextColumn();

		//  (etc)
		PLOTWINDOW.AddJqData((*itr)->GetJqStack(), _tableTime);
	}
}

void PlayerTable::DrawBar(float window_Width, float percent, ImU32 color) {

	auto draw_list = ImGui::GetWindowDrawList();

	if (UIOPTION.isTableOverlayMode()) {
		ImVec4 overlayColor = ImGui::ColorConvertU32ToFloat4(color);
		overlayColor.w = 0.24f;
		color = ImGui::ColorConvertFloat4ToU32(overlayColor);
	}

	float result_width = window_Width * percent;
	float height = ImGui::GetFontSize();
	ImVec2 line = ImGui::GetCursorScreenPos();

	line.x = FLOOR(line.x);	line.y = line.y;
	height = height;
	ImGui::TablePushBackgroundChannel(); //without this image drawing will break bar
	draw_list->AddRectFilled(ImVec2(line.x, line.y), ImVec2(line.x + result_width, line.y + height), color, 0, 0);
	ImGui::TablePopBackgroundChannel();
}

bool PlayerTable::ToggleSelectInfo(uint32_t id) {

	for (auto itr = _selectInfo.begin(); itr != _selectInfo.end(); itr++) {
		if ((*itr)->_playerID == id) {
			(*itr)->_isSelected = !(*itr)->_isSelected;

			return (*itr)->_isSelected;
		}
	}

	SELECTED_PLAYER* selectinfo = new SELECTED_PLAYER(id, TRUE, new SpecificInformation(id));
	_selectInfo.push_back(selectinfo);

	return selectinfo->_isSelected;
}

void PlayerTable::ShowSelectedTable() {

	for (auto itr = _selectInfo.begin(); itr != _selectInfo.end(); itr++) {
		if ((*itr)->_isSelected == TRUE) {
			(*itr)->_specificInfo->Update(&(*itr)->_isSelected, itr - _selectInfo.begin());
		}
	}
}
