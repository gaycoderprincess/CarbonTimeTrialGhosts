std::string GetCarName(const std::string& carModel) {
	auto tmp = CreateStockCarRecord(carModel.c_str());
	std::string carName = GetLocalizedString(FECarRecord::GetNameHash(&tmp));
	if (carName == GetLocalizedString(0x881F8EFA)) { // unlocalized string
		carName = carModel;
		std::transform(carName.begin(), carName.end(), carName.begin(), [](char c){ return std::toupper(c); });
	}
	return carName;
}

std::string GetTrackName(const std::string& eventId, uint32_t nameHash) {
	auto trackName = GetLocalizedString(nameHash);
	if (trackName == GetLocalizedString(0x881F8EFA)) { // unlocalized string
		return eventId;
	}
	return trackName;
}

std::string GetCarNameForGhost(const std::string& carPreset) {
	auto carName = carPreset;
	if (auto preset = FindFEPresetCar(FEHashUpper(carName.c_str()))) {
		carName = preset->CarTypeName;
		std::transform(carName.begin(), carName.end(), carName.begin(), [](char c){ return std::tolower(c); });
	}
	return carName;
}

struct tChallengeSeriesEvent {
	std::string sEventName;
	std::string sCarPreset;
	int nLapCountOverride = 0;

	int nNumGhosts = 0;

	GRaceParameters* GetRace() const {
		return GRaceDatabase::GetRaceFromHash(GRaceDatabase::mObj, Attrib::StringHash32(sEventName.c_str()));
	}

	int GetLapCount() {
		if (nLapCountOverride > 0) return nLapCountOverride;
		return GRaceParameters::GetNumLaps(GetRace());
	}

	uint32_t GetPBTime() {
		tReplayGhost temp;
		LoadPB(&temp, GetCarNameForGhost(sCarPreset), sEventName, GetLapCount(), 0, nullptr);
		return temp.nFinishTime;
	}

	uint32_t GetTargetTime() {
		tReplayGhost targetTime;
		auto times = CollectReplayGhosts(GetCarNameForGhost(sCarPreset), sEventName, GetLapCount(), nullptr);
		if (!times.empty()) targetTime = times[0];
		nNumGhosts = times.size();
		return targetTime.nFinishTime;
	}

	std::string GetTargetName() {
		tReplayGhost targetTime;
		auto times = CollectReplayGhosts(GetCarNameForGhost(sCarPreset), sEventName, GetLapCount(), nullptr);
		if (!times.empty()) targetTime = times[0];
		return targetTime.sPlayerName;
	}
};

std::vector<tChallengeSeriesEvent> aNewChallengeSeries = {
	{"mu.1.3", "CE_240SX"},
};

tChallengeSeriesEvent* GetChallengeEvent(uint32_t hash) {
	for (auto& event : aNewChallengeSeries) {
		if (Attrib::StringHash32(event.sEventName.c_str()) == hash) return &event;
	}
	return nullptr;
}

tChallengeSeriesEvent* GetChallengeEvent(const std::string& str) {
	for (auto& event : aNewChallengeSeries) {
		if (event.sEventName == str) return &event;
	}
	return nullptr;
}

int CalculateTotalTimes() {
	uint32_t totalTime = 0;
	for (auto& event : aNewChallengeSeries) {
		auto time = event.GetPBTime();
		if (!time) return 0;
		totalTime += time;
	}
	return totalTime;
}

tChallengeSeriesEvent* pSelectedEvent = nullptr;

bool __stdcall GetChallengeSeriesEventCount(int* out) {
	*out = aNewChallengeSeries.size();
	return true;
}

bool __stdcall GetChallengeSeriesEventHash(int* out, int id) {
	*out = Attrib::StringHash32(aNewChallengeSeries[id].sEventName.c_str());
	return true;
}

bool __stdcall GetChallengeSeriesEventMedalLevel(int* out, uint32_t hash) {
	*out = (GetChallengeEvent(hash) - &aNewChallengeSeries[0]) % 3;
	return true;
}

bool __stdcall GetChallengeSeriesEventGroup(int* out, uint32_t hash) {
	*out = ((GetChallengeEvent(hash) - &aNewChallengeSeries[0]) / 3) + 1;
	return true;
}

bool __stdcall GetChallengeSeriesEventUnlocked(int* out, int id) {
	*out = 1;
	return true;
}

bool __stdcall GetChallengeSeriesEventCompleted(int* out, uint32_t hash) {
	DLLDirSetter _setdir;

	auto event = GetChallengeEvent(hash);
	auto pb = event->GetPBTime();
	if (pb && pb <= event->GetTargetTime()) {
		*out = 1;
	}
	else {
		*out = 0;
	}
	return true;
}

// todo
bool __stdcall GetChallengeSeriesEventDescription(int* out, uint32_t hash) {

}

bool __stdcall GetChallengeSeriesEventSolo(int* out, uint32_t hash) {
	*out = 1;
	return true;
}

bool __thiscall GetChallengeSeriesEventPlayerCar(GRaceParameters* pThis) {
	auto event = GetChallengeEvent(GRaceParameters::GetEventID(pThis));
	if (!event) return Attrib::StringHash32("911turbo");
	return Attrib::StringHash32(event->sCarPreset.c_str());
}

bool __thiscall GetChallengeSeriesEventUsePresetRide(GRaceParameters* pThis) {
	auto event = GetChallengeEvent(GRaceParameters::GetEventID(pThis));
	if (!event) return false;
	return FindFEPresetCar(FEHashUpper(event->sCarPreset.c_str())) != nullptr;
}

int __thiscall GetNumOpponentsHooked(GRaceParameters* pThis) {
	if (!pSelectedEvent) return 8;

	if (bViewReplayMode) return 0;
	if (bChallengesOneGhostOnly) return 1;
	return nDifficulty != DIFFICULTY_EASY ? std::min(pSelectedEvent->nNumGhosts, 3) : 1; // only spawn one ghost for easy difficulty
}

void ApplyCustomEventsHooks() {
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x63C660, &GetNumOpponentsHooked);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4C3739, &GetChallengeSeriesEventCount);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4C374F, &GetChallengeSeriesEventHash);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4C377B, &GetChallengeSeriesEventMedalLevel);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4C3857, &GetChallengeSeriesEventGroup);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4C3791, &GetChallengeSeriesEventUnlocked);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4C37A7, &GetChallengeSeriesEventCompleted);
	//NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4C3899, &GetChallengeSeriesEventDescription);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4C37FF, &GetChallengeSeriesEventSolo);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x63F4B0, &GetChallengeSeriesEventPlayerCar);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x63E660, &GetChallengeSeriesEventUsePresetRide);

	// use default fallback icons
	NyaHookLib::Fill(0x4AA2B2, 0x90, 6);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x4AA2D0, 0x4AA364);
	NyaHookLib::Fill(0x4AA405, 0x90, 6);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x4AA425, 0x4AA4AD);
}