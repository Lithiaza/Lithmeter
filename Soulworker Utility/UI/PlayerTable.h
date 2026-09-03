#pragma once
#include ".\UI\SpecificInfomation.h"
#include <unordered_map>
#include <vector>

class SWDamagePlayer;

typedef struct _TRACKED_SKILL_SUMMARY {
	bool _found = false;
	bool _pinned = false;
	uint32_t _skillID = 0;
	uint64_t _damage = 0;
	uint32_t _hitCount = 0;
	uint32_t _critHitCount = 0;
	uint32_t _casts = 0;
	uint32_t _castHitCount = 0;
	uint32_t _castCritHitCount = 0;
	bool _hasCast = false;
	char _name[64] = { 0 };
}TRACKED_SKILL_SUMMARY;

typedef struct _SELECTED_PLAYER {
	
	uint32_t _playerID;
	bool _isSelected;
	SpecificInformation* _specificInfo;

	_SELECTED_PLAYER(uint32_t playerID, bool isSelected, SpecificInformation* specificInfo) : _playerID(playerID), _isSelected(isSelected), _specificInfo(specificInfo) { }
}SELECTED_PLAYER;

#define PLAYERTABLE PlayerTable::getInstance()

class PlayerTable : public Singleton<PlayerTable> {
private:
	std::vector<SELECTED_PLAYER*> _selectInfo;
	std::unordered_map<uint32_t, std::vector<uint32_t>> _trackedSkillByPlayer;

	bool ToggleSelectInfo(uint32_t id);
	void ShowSelectedTable();
	void BeginPopupMenu();
	void DrawMenuBar();
	void DrawStatusStrip();

	void SetWindowSize();
	void SetMainWindowSize();
	void StoreWindowWidth();
	void SetupFontScale();

	void DrawBar(float window_Width, float percent, ImU32 color);
	void SetupTable();
	void UpdateTable(float windowWidth);
	void SetupStackedMeter();
	void UpdateStackedMeter(float windowWidth);
	void DrawStackedMetric(const char* metric, const char* value, bool selectable = false, uint32_t playerId = 0, ImU32 barColor = 0, float barPercent = 0.0f);
	bool BuildTrackedSkillSummary(SWDamagePlayer* player, TRACKED_SKILL_SUMMARY& summary, size_t slotIndex = 0) const;
	void DrawTrackedSkillCombo(SWDamagePlayer* player, const TRACKED_SKILL_SUMMARY& trackedSkill, size_t slotIndex = 0);
	void DrawTrackedSkillColumns(SWDamagePlayer* player);
	void DrawStackedSkillBlock(SWDamagePlayer* player, const TRACKED_SKILL_SUMMARY& trackedSkill, size_t slotIndex);
	void DrawStackedSkillAddRow(SWDamagePlayer* player);
	void DrawStackedRowsMenu();
	void DrawStackedOpacitySlider();

	float _globalFontScale;
	float _columnFontScale;
	float _tableFontScale;

	float _curWindowSize;

	bool _tableResize;

	float _tableTime;
	float _accumulatedTime;
	bool _windowSizeInitialized;
	bool _wasStackedMeterMode;

public:
	PlayerTable();
	~PlayerTable();

	void Update();
	void ClearTable();
	void ResizeTalbe();
	void TrackPlayerSkill(uint32_t playerID, uint32_t skillID, size_t slotIndex = 0);
	uint32_t GetTrackedSkill(uint32_t playerID, size_t slotIndex = 0) const;
	void ClearTrackedSkill(uint32_t playerID, size_t slotIndex = 0);
	void AddTrackedSkillSlot(uint32_t playerID);
	void RemoveTrackedSkillSlot(uint32_t playerID, size_t slotIndex);
	size_t GetTrackedSkillSlotCount(uint32_t playerID) const;
	// Persist the shared metric visibility used by the wide and stacked views.
	std::string GetStackedRowsVisibility();
	void SetStackedRowsVisibility(const char* visibility);
	void ResetStackedRowsVisibility();

	LONG64 _lastSendTimestamp = 0;
	LONG64 _ping = 0;
	uint32_t _tick = 0;
	bool _isNewestVersion = TRUE;

	float GetTableTime()
	{
		return _tableTime;
	}

};
