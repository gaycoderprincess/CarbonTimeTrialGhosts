std::string GetCarNameForGhost(const std::string& carPreset) {
	auto carName = carPreset;
	if (auto preset = FindFEPresetCar(FEHashUpper(carName.c_str()))) {
		auto car = Attrib::FindCollection(Attrib::StringHash32("pvehicle"), preset->VehicleKey);
		if (!car) {
			MessageBoxA(0, std::format("Failed to find pvehicle for {} ({} {:X})", carPreset, preset->CarTypeName, preset->VehicleKey).c_str(), "nya?!~", MB_ICONERROR);
			exit(0);
		}
		return *(const char**)Attrib::Collection::GetData(car, Attrib::StringHash32("CollectionName"), 0);
	}
	return carName;
}

struct tChallengeSeriesEvent {
	std::string sEventName;
	std::string sCarPreset;
	int nLapCountOverride = 0;

	std::string sGhostCarName;
	int nNumGhosts = 0;

	GRaceParameters* GetRace() const {
		return GRaceDatabase::GetRaceFromHash(GRaceDatabase::mObj, Attrib::StringHash32(sEventName.c_str()));
	}

	int GetLapCount() {
		if (nLapCountOverride > 0) return nLapCountOverride;
		return GRaceParameters::GetNumLaps(GetRace());
	}

	tReplayGhost GetPBGhost() {
		if (sGhostCarName.empty()) sGhostCarName = GetCarNameForGhost(sCarPreset);

		tReplayGhost temp;
		LoadPB(&temp, sGhostCarName, sEventName, GetLapCount(), 0, nullptr);
		return temp;
	}

	tReplayGhost GetTargetGhost() {
		if (sGhostCarName.empty()) sGhostCarName = GetCarNameForGhost(sCarPreset);

		tReplayGhost targetTime;
		auto times = CollectReplayGhosts(sGhostCarName, sEventName, GetLapCount(), nullptr);
		if (!times.empty()) targetTime = times[0];
		nNumGhosts = times.size();
		return targetTime;
	}
};

std::vector<tChallengeSeriesEvent> aNewChallengeSeries = {
	// custom events
	{"tn.1.2", "IS300_2"},
	{"ex.1.3", "EUROPA_2"},
	{"sf.3.1", "MR2_2"},

	{"qr.4.2", "CROSS"},
	{"mu.1.3", "CE_240SX"},
	{"sf.2.1", "NIKKI"},

	{"tn.99.99", "CS_RX8"},
	{"tn.1.1", "KENJI"},
	{"ct.2.2", "CE_IMPREZA"},

	{"ce.1.1", "ANGIE_CASINO"},
	{"mu.3.3", "CS_SKYLINE"},
	{"4.war", "CS_350Z"},

	{"ex.2.1", "WOLF"},
	{"mu.5.3", "CS_SUPRA"},
	{"mu.3.1", "ANGIE"},

	{"ex.3.1", "CE_MURCIELAGO"},
	{"5.boss", "KENJI_CASINO"},
	{"ps2.rx7", "CS_RX7"},

	{"ct.3.1", "DARIUS"},
	{"cs.10.1", "CS_CORVETTEZ06"},
	{"qr.4.6", "M3GTRCAREERSTART"},

	// challenge series
	// canyon
	//{"cs.1.1", "IS300_2"},
	//{"cs.1.2", "roadrunner"},
	//{"cs.1.3", "CROSS"},
	// canyon sprint
	//{"cs.2.1", "CS_RX8"},
	//{"cs.2.2", "CS_ECLIPSEGT"},
	//{"cs.2.3", "CS_350Z"},
	// checkpoint
	//{"cs.3.1", "CS_CLIO"},
	//{"cs.3.2", "CS_CUDA"},
	////{"cs.3.3", "997gt3rs"},
	// checkpoint
	//{"ce.4.1", "ANGIE_CASINO"},
	//{"ce.4.2", "KENJI_CASINO"},
	//{"ce.4.3", "WOLF_CASINO"},
	// circuit
	//{"cs.8.1", "CS_BRERA"},
	//{"cs.8.2", "CS_GALLARDO"},
	//{"cs.8.3", "CS_MUSTANGSHLBYO"},
	// sprint
	//{"cs.9.1", "CS_CHARGER69"},
	//{"cs.9.2", "CS_CAYMANS"},
	//{"cs.9.3", "CS_LANCEREVO9"},
	// drift
	////{"cs.10.1", "EUROPA_2"},
	//{"cs.10.2", "CS_SUPRA"},
	//{"cs.10.3", "CS_CORVETTEZ06"},
	// speedtrap
	//{"cs.11.1", "CS_300C"},
	//{"cs.11.2", "CS_MUSTANGGT"},
	//{"cs.11.3", "CS_MUSTANGSHLBYO"},
	// circuit
	//{"cs.12.1", "ECLIPSE_2"},
	//{"cs.12.2", "CS_SL65"},
	//{"cs.12.3", "CS_CHALLENGERN"},
	// checkpoint
	//{"cs.13.1", "CS_CAMARO"},
	//{"cs.13.2", "CS_RX7"},
	//{"cs.13.3", "M3GTRCAREERSTART"},
	// canyon drift
	//{"cs.14.2", "MR2_2"},
	//{"cs.14.3", "CE_MURCIELAGO"},
	//{"cs.14.4", "CS_DB9"},
	// circuit
	//{"ce.15.1", "CE_240SX"},
	//{"ce.15.2", "MUSCLE_2"},
	////{"ce.15.3", "ccx"},
};

tChallengeSeriesEvent* GetChallengeEvent(uint32_t hash) {
	for (auto& event : aNewChallengeSeries) {
		if (!GRaceDatabase::GetRaceFromHash(GRaceDatabase::mObj, Attrib::StringHash32(event.sEventName.c_str()))) {
			MessageBoxA(0, std::format("Failed to find event {}", event.sEventName).c_str(), "nya?!~", MB_ICONERROR);
			exit(0);
		}
	}
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
		auto time = event.GetPBGhost().nFinishTime;
		if (!time) return 0;
		totalTime += time;
	}
	return totalTime;
}

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
	auto pb = event->GetPBGhost().nFinishTime;
	*out = pb != 0;
	//if (pb && pb <= event->GetTargetTime()) {
	//	*out = 1;
	//}
	//else {
	//	*out = 0;
	//}
	return true;
}

bool __stdcall GetChallengeSeriesEventDescription(int* out, uint32_t hash) {
	auto event = GetChallengeEvent(hash);
	if (!event) return false;
	*out = (event - &aNewChallengeSeries[0]) + 1000;
	return true;
}

bool __stdcall GetChallengeSeriesEventSolo(int* out, uint32_t hash) {
	*out = 1;
	return true;
}

const char* __thiscall GetChallengeSeriesEventPlayerCar(GRaceParameters* pThis) {
	auto event = GetChallengeEvent(GRaceParameters::GetEventID(pThis));
	if (!event) return "911turbo";
	return event->sCarPreset.c_str();
}

uint32_t __thiscall GetChallengeSeriesEventPlayerCarHash(GRaceParameters* pThis) {
	auto event = GetChallengeEvent(GRaceParameters::GetEventID(pThis));
	if (!event) return Attrib::StringHash32("911turbo");
	return FEHashUpper(event->sCarPreset.c_str());
}

bool __thiscall GetChallengeSeriesEventUsePresetRide(GRaceParameters* pThis) {
	auto event = GetChallengeEvent(GRaceParameters::GetEventID(pThis));
	if (!event) return false;
	return FindFEPresetCar(FEHashUpper(event->sCarPreset.c_str())) != nullptr;
}

bool __thiscall GetIsChallengeSeriesEvent(GRaceParameters* pThis) {
	return GetChallengeEvent(GRaceParameters::GetEventID(pThis)) != nullptr;
}

bool __stdcall GetChallengeSeriesEventTime(float* out, uint32_t hash) {
	DLLDirSetter _setdir;

	auto event = GetChallengeEvent(hash);
	if (!event) return false;
	auto time = event->GetPBGhost().nFinishTime;
	if (!time) return false;
	*out = time * 0.001;
	return true;
}

bool __stdcall GetChallengeSeriesEventPoints(int* out, uint32_t hash) {
	DLLDirSetter _setdir;

	auto event = GetChallengeEvent(hash);
	if (!event) return false;
	auto pts = event->GetPBGhost().nFinishPoints;
	if (!pts) return false;
	*out = pts;
	return true;
}

int __thiscall GetNumOpponentsHooked(GRaceParameters* pThis) {
	auto event = GetChallengeEvent(GRaceParameters::GetEventID(pThis));
	if (!event) return 8;

	if (bViewReplayMode) return 0;
	if (bChallengesOneGhostOnly) return 1;
	return nDifficulty != DIFFICULTY_EASY ? std::min(event->nNumGhosts, 29) : 1; // only spawn one ghost for easy difficulty
}

const char* GetLocalizedStringHooked(uint32_t key) {
	auto id = key - 1000;
	if (id < aNewChallengeSeries.size()) {
		DLLDirSetter _setdir;

		auto target = aNewChallengeSeries[id].GetTargetGhost();
		auto targetTime = target.nFinishTime;
		auto targetPoints = target.nFinishPoints;
		auto targetName = target.sPlayerName;

		static std::string str;
		str = "";
		if (targetPoints > 0) {
			str += std::format("Target: {} PTS", FormatScore(targetPoints));
			if (!targetName.empty()) {
				str += std::format(" ({})", targetName);
			}
		}
		else if (targetTime > 0) {
			str += std::format("Target Time: {}", GetTimeFromMilliseconds(targetTime));
			str.pop_back();
			if (!targetName.empty()) {
				str += std::format(" ({})", targetName);
			}
		}
		auto total = CalculateTotalTimes();
		if (total > 0) {
			str += std::format("\nCompletion Time: {}", GetTimeFromMilliseconds(total));
			str.pop_back();
		}
		return str.c_str();
	}
	return GetLocalizedString(key);
}

bool __thiscall GetIsDDayRaceHooked(GRaceParameters* pThis) {
	return false;
}

bool __thiscall GetIsTutorialRaceHooked(GRaceParameters* pThis) {
	return false;
}

bool __thiscall GetIsBossRaceHooked(GRaceParameters* pThis) {
	return false;
}

bool __thiscall GetIsRollingStartHooked(GRaceParameters* pThis) {
	return false;
}

int __thiscall GetNumLapsHooked(GRaceParameters* pThis) {
	if (!GRaceParameters::GetIsLoopingRace(pThis)) return 1;
	auto event = GetChallengeEvent(GRaceParameters::GetEventID(pThis));
	if (!event || !event->nLapCountOverride) return 2;
	return event->nLapCountOverride;
}

bool IntroNISHooked(GRaceParameters* a1) {
	return false;
}

const char* GetNISHooked(GRaceParameters* a1) {
	return nullptr;
}

const char* GetCountdownNISHooked(GRaceParameters* a1) {
	return "";
}

void ApplyCustomEventsHooks() {
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x651A80, &IntroNISHooked);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x651860, &GetNISHooked);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x63F0B0, &GetCountdownNISHooked);
	//NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x650E3D, 0x650FDC); // disable NIS_Play
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x63E0C0, &GetIsDDayRaceHooked);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x63E180, &GetIsTutorialRaceHooked);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x63E120, &GetIsBossRaceHooked);
	//NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x63E8F0, &GetIsRollingStartHooked);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x63F450, &GetNumLapsHooked);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x63C660, &GetNumOpponentsHooked);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x63E540, &GetIsChallengeSeriesEvent);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4C3739, &GetChallengeSeriesEventCount);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4C374F, &GetChallengeSeriesEventHash);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4C377B, &GetChallengeSeriesEventMedalLevel);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4C3857, &GetChallengeSeriesEventGroup);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4C3791, &GetChallengeSeriesEventUnlocked);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4C37A7, &GetChallengeSeriesEventCompleted);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4C3899, &GetChallengeSeriesEventDescription);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4C37FF, &GetChallengeSeriesEventSolo);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4B71F7, &GetChallengeSeriesEventTime);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4C3841, &GetChallengeSeriesEventPoints);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x63E990, &GetChallengeSeriesEventPlayerCar);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x63F4B0, &GetChallengeSeriesEventPlayerCarHash);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x63E660, &GetChallengeSeriesEventUsePresetRide);

	// use default fallback icons
	NyaHookLib::Fill(0x4AA2B2, 0x90, 6);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x4AA2D0, 0x4AA364);
	NyaHookLib::Fill(0x4AA405, 0x90, 6);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x4AA425, 0x4AA4AD);

	uint32_t localizedStringCalls[] = {
			0x4A2A98,
			0x4A2F3C,
			0x4A6AA2,
			0x4A9600,
			0x4A9627,
			0x4A964E,
			0x4A974C,
			0x4A9792,
			0x4A97F9,
			0x4A98B3,
			0x4A9C1E,
			0x4A9CBB,
			0x4AA8AC,
			0x4AA8BC,
			0x4AA8CC,
			0x4B17FE,
			0x4B1808,
			0x4B1816,
			0x4B185F,
			0x4B1869,
			0x4B1877,
			0x4B1995,
			0x4B199F,
			0x4B19AD,
			0x4B2567,
			0x4B257A,
			0x4B25A9,
			0x4B3B1C,
			0x4B3B5B,
			0x4B3B6D,
			0x4B3BBA,
			0x4B42FF,
			0x4B439F,
			0x4B4413,
			0x4B445E,
			0x4B5E4B,
			0x4B5EBB,
			0x4B5F45,
			0x4B61EA,
			0x4B6260,
			0x4B746B,
			0x4B74C6,
			0x4B7529,
			0x4BBE82,
			0x4BBEF7,
			0x4BBF02,
			0x4BBF21,
			0x4BBF50,
			0x4BC5C9,
			0x4BC5D7,
			0x4BCD99,
			0x4BD7CF,
			0x4BD89F,
			0x4BD963,
			0x4BDA22,
			0x4C73E2,
			0x4C7404,
			0x4C7426,
			0x4CFCE1,
			0x4CFD22,
			0x4CFD4E,
			0x4D5112,
			0x578885,
			0x57FD26,
			0x57FD36,
			0x580759,
			0x580769,
			0x5808A5,
			0x582954,
			0x583E24,
			0x586B5C,
			0x58DE0D,
			0x5921DB,
			0x592420,
			0x592CAF,
			0x592D09,
			0x592D61,
			0x592DB3,
			0x594727,
			0x594775,
			0x5947BE,
			0x59490E,
			0x594957,
			0x5949A1,
			0x594A74,
			0x594ADB,
			0x594B2C,
			0x594B71,
			0x594BD4,
			0x595666,
			0x5968FB,
			0x596965,
			0x59D0EF,
			0x59F27F,
			0x59F60B,
			0x5A1F47,
			0x5A375E,
			0x5A3780,
			0x5A3C92,
			0x5A3DB3,
			0x5A64A3,
			0x5A6721,
			0x5A6FB6,
			0x5A7029,
			0x5A7184,
			0x5A72D3,
			0x5A72ED,
			0x5A7390,
			0x5A7430,
			0x5A74D0,
			0x5A7560,
			0x5A7600,
			0x5A7690,
			0x5A7730,
			0x5A77C0,
			0x5A7850,
			0x5A7C97,
			0x5A927C,
			0x5A9286,
			0x5A92AA,
			0x5A953B,
			0x5A955D,
			0x5A9574,
			0x5AA639,
			0x5AA7D5,
			0x5AA7EC,
			0x5AA80D,
			0x5AA824,
			0x5AA845,
			0x5AA85C,
			0x5AA873,
			0x5AA88A,
			0x5AA8A6,
			0x5AA8BD,
			0x5AA8DE,
			0x5AA8F5,
			0x5AA91F,
			0x5AA936,
			0x5AA94D,
			0x5AA964,
			0x5AA980,
			0x5AA997,
			0x5AA9AE,
			0x5AA9C5,
			0x5AA9E1,
			0x5AA9F8,
			0x5AAA0F,
			0x5AAA26,
			0x5AAF42,
			0x5AAF7D,
			0x5AAFF4,
			0x5AB02B,
			0x5AB804,
			0x5AED0F,
			0x5AFEC7,
			0x5B2BE9,
			0x5B3E1C,
			0x5B50D1,
			0x5B5D56,
			0x5B5DC1,
			0x5B5DCF,
			0x5B5EF1,
			0x5B5F38,
			0x5B5F72,
			0x5B618C,
			0x5B6283,
			0x5B6782,
			0x5B6793,
			0x5B7FF5,
			0x5B83A3,
			0x5B8582,
			0x5B8596,
			0x5B85B6,
			0x5B860D,
			0x5B862E,
			0x5B869F,
			0x5B87E5,
			0x5B88B7,
			0x5B8A34,
			0x5B9FA4,
			0x5BA1F7,
			0x5BA208,
			0x5BEAE4,
			0x5C0B34,
			0x5C0B59,
			0x5C0B89,
			0x5C4289,
			0x5C46FC,
			0x5C6379,
			0x5C6F49,
			0x5C7646,
			0x5C7EB6,
			0x5C9EB0,
			0x5CA300,
			0x5CA341,
			0x5CA6EC,
			0x5CD21E,
			0x5CD22C,
			0x5CF408,
			0x5CF449,
			0x5CF467,
			0x5CF475,
			0x5CF499,
			0x5CF4B7,
			0x5CF4C5,
			0x5CF4E9,
			0x5CF506,
			0x5CF51C,
			0x5CF532,
			0x5CF5BC,
			0x5CF5CF,
			0x5CF5FC,
			0x5CF608,
			0x5CF631,
			0x5CF642,
			0x5CF650,
			0x5CF6D9,
			0x5CF6EA,
			0x5CF6F8,
			0x5CF74A,
			0x5CF75B,
			0x5CF769,
			0x5CF79B,
			0x5CF7AF,
			0x5CF7C3,
			0x5CF7D7,
			0x5CF7EE,
			0x5CF847,
			0x5CF8DC,
			0x5CF8F1,
			0x5CF979,
			0x5CF990,
			0x5CF99E,
			0x5CF9C0,
			0x5CF9CE,
			0x5CF9DC,
			0x5CF9EA,
			0x5CFA1D,
			0x5CFA2B,
			0x5CFA39,
			0x5CFA4F,
			0x5CFA5D,
			0x5CFA72,
			0x5CFA80,
			0x5CFA8E,
			0x5CFAA3,
			0x5CFAB1,
			0x5CFABF,
			0x5CFACD,
			0x5CFAF3,
			0x5CFB8B,
			0x5CFB99,
			0x5CFBA7,
			0x5CFBB5,
			0x5CFBCC,
			0x5CFBE0,
			0x5CFBF4,
			0x5CFC08,
			0x5CFC1C,
			0x5CFC77,
			0x5CFC8B,
			0x5CFC9F,
			0x5CFCB3,
			0x5CFCD5,
			0x5CFD49,
			0x5CFD6D,
			0x5CFDA8,
			0x5CFDD9,
			0x5CFDE7,
			0x5CFDF5,
			0x5CFE0A,
			0x5CFE18,
			0x5CFE26,
			0x5CFE34,
			0x5CFE51,
			0x5CFE6B,
			0x5CFE85,
			0x5CFE9F,
			0x5CFEB9,
			0x5CFEDA,
			0x5CFF36,
			0x5CFFBF,
			0x5CFFD3,
			0x5CFFE7,
			0x5CFFFC,
			0x5D000F,
			0x5D0042,
			0x5D0057,
			0x5D006C,
			0x5D0081,
			0x5D0096,
			0x5D00AB,
			0x5D00C0,
			0x5D00D5,
			0x5D00EA,
			0x5D00FF,
			0x5D0114,
			0x5D0129,
			0x5D013E,
			0x5D0153,
			0x5D0168,
			0x5D017D,
			0x5D0192,
			0x5D01A7,
			0x5D01BC,
			0x5D01D1,
			0x5D01E6,
			0x5D01FB,
			0x5D0210,
			0x5D0225,
			0x5D023A,
			0x5D024F,
			0x5D0264,
			0x5D0279,
			0x5D028E,
			0x5D02A3,
			0x5D02B8,
			0x5D02CD,
			0x5D03AC,
			0x5D03BF,
			0x5D03EC,
			0x5D03FF,
			0x5D0983,
			0x5D0998,
			0x5D09C0,
			0x5D09DE,
			0x5D09F8,
			0x5D0A0F,
			0x5D0A24,
			0x5D0A36,
			0x5D0A51,
			0x5D0A69,
			0x5D0A8B,
			0x5D0AAD,
			0x5D0AC4,
			0x5D0ADD,
			0x5D0C01,
			0x5D0C3D,
			0x5D0C70,
			0x5D0C94,
			0x5D0DBE,
			0x5D0DDA,
			0x5D0DEF,
			0x5D0E0B,
			0x5D0E20,
			0x5D0E35,
			0x5D0E4A,
			0x5D0E5F,
			0x5D0E77,
			0x5D0E8C,
			0x5D0EA1,
			0x5D0EB6,
			0x5D0ECE,
			0x5D0EE3,
			0x5D0EF8,
			0x5D38F5,
			0x5D3905,
			0x5D3C3F,
			0x5D3D79,
			0x5D3E1E,
			0x5D3F0D,
			0x5D4167,
			0x5D447D,
			0x5D44DD,
			0x5D4561,
			0x5D4586,
			0x5D5A84,
			0x5D5D13,
			0x5D63A2,
			0x5D6407,
			0x5D6500,
			0x5D67C9,
			0x5D67D7,
			0x5D686F,
			0x5D69F2,
			0x5D6C2B,
			0x5D6CFF,
			0x5D7B3F,
			0x5D7B4D,
			0x5D7B7F,
			0x5D7B8D,
			0x5D7BB0,
			0x5D7BBE,
			0x5D7C00,
			0x5D9F4C,
			0x5D9FC0,
			0x5D9FE4,
			0x5DA045,
			0x5DA069,
			0x5DA0E9,
			0x5DA0F3,
			0x5DA492,
			0x5DAB43,
			0x5DAB64,
			0x5DACDC,
			0x5DAD3D,
			0x5DAD98,
			0x5DAE16,
			0x5DAE31,
			0x5DAE47,
			0x5DB036,
			0x5DB06D,
			0x5DB8A9,
			0x5DBA8C,
			0x5DBF49,
			0x5DD376,
			0x5DD448,
			0x5DD7A5,
			0x5DDA56,
			0x5DDC6B,
			0x5DDC7E,
			0x5DDCBB,
			0x5DDCDF,
			0x5DE087,
			0x5DE116,
			0x5DE1EE,
			0x5DE2E1,
			0x5DE3FF,
			0x5DE480,
			0x5DFCBF,
			0x5E1E73,
			0x5E3AFD,
			0x5E3BDB,
			0x613135,
			0x617C2B,
			0x617C6B,
			0x617D28,
			0x617D5A,
			0x617F0B,
			0x625005,
			0x62501D,
			0x62506F,
			0x627B7B,
			0x62C0BE,
			0x62C4A1,
			0x62C61B,
			0x62C676,
			0x62C6D1,
			0x62C77D,
			0x62C858,
			0x62DAB4,
			0x644B98,
			0x655BE9,
			0x656617,
			0x656625,
			0x685BAC,
			0x6930C9,
			0x693E92,
			0x693EA0,
			0x6BD83B,
			0x6BD84A,
			0x6BDB9A,
			0x6BE02C,
			0x6BE037,
			0x6BE0C6,
			0x6BE0D0,
			0x6BEC43,
			0x6BEC91,
			0x6BED97,
			0x7167A5,
			0x716BCB,
			0x716BDF,
			0x716BF3,
			0x716C07,
			0x716C1B,
			0x716C2F,
			0x716C43,
			0x716C57,
			0x716C6B,
			0x716C7F,
			0x716C93,
			0x716CA7,
			0x71F6D6,
			0x71F815,
			0x71F8A7,
			0x71FA3D,
			0x71FBEA,
			0x720560,
			0x723F8D,
			0x724FBC,
			0x75DC56,
			0x75DC89,
			0x75DCD2,
			0x761F44,
			0x7A9D74,
			0x7BBF99,
			0x828FCB,
			0x828FEC,
			0x834C66,
			0x83D3C5,
			0x840979,
			0x840989,
			0x840999,
			0x8409A9,
			0x8409B9,
			0x8409D3,
			0x8409E3,
			0x8409F3,
			0x840C94,
			0x842077,
			0x84208B,
			0x84209F,
			0x8420B3,
			0x8438C8,
			0x8438DC,
			0x84467D,
			0x844694,
			0x8446A9,
			0x8446D0,
			0x8446E5,
			0x8446FA,
			0x84470F,
			0x84473C,
			0x844751,
			0x844766,
			0x84477D,
			0x84478B,
			0x844799,
			0x844FA6,
			0x844FB4,
			0x844FC2,
			0x844FEA,
			0x844FF8,
			0x84501A,
			0x845028,
			0x845400,
			0x845414,
			0x84593A,
			0x845959,
			0x845967,
			0x84598A,
			0x8459A6,
			0x8459E3,
			0x845A02,
			0x845A10,
			0x845A33,
			0x845A4C,
			0x845A65,
			0x845A73,
			0x845A8D,
			0x845AA4,
			0x845ABB,
			0x845EE7,
			0x845EFB,
			0x845F0F,
			0x845F23,
			0x845F37,
			0x846653,
			0x846668,
			0x84667D,
			0x846692,
			0x8466A7,
			0x8466BC,
			0x8466ED,
			0x846703,
			0x846719,
			0x84672F,
			0x846745,
			0x846785,
			0x84679B,
			0x846E63,
			0x846E77,
			0x846E8B,
			0x8475DB,
			0x847899,
			0x8478B3,
			0x8478C1,
			0x8478EE,
			0x8478FC,
			0x84790A,
			0x847925,
			0x84796B,
			0x847984,
			0x84799D,
			0x8479B5,
			0x847F73,
			0x847F87,
			0x847F9B,
			0x848D11,
			0x848D8B,
			0x84C633,
			0x84C783,
			0x84C82E,
			0x84C916,
			0x84C924,
			0x84C9E9,
			0x84CA87,
			0x84E7A1,
			0x84E8A2,
			0x84E8F3,
			0x853A8D,
			0x853A9B,
			0x853AA9,
			0x853AD0,
			0x853AE7,
			0x853AFE,
			0x853B0C,
			0x853B1A,
			0x85509D,
			0x8550E2,
			0x8553C4,
			0x8554B5,
			0x855655,
			0x85578A,
			0x8558BA,
			0x855A15,
			0x855B51,
			0x855C02,
			0x8570E2,
			0x857115,
			0x858D56,
			0x858DE7,
			0x858E2F,
			0x85C573,
			0x85CED0,
			0x85CF4B,
			0x85CF63,
			0x85E1A3,
			0x85F915,
			0x86013D,
			0x860CEB,
			0x860E39,
			0x860E82,
			0x86104D,
			0x86108E,
			0x8690F8,
			0x869106,
			0x869A54,
			0x869ADA,
			0x869AFE,
			0x86A6BB,
			0x86A741,
			0x86A765,
	};
	for (auto& addr : localizedStringCalls) {
		NyaHookLib::PatchRelative(NyaHookLib::CALL, addr, &GetLocalizedStringHooked);
	}

	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x5788A0, &GetLocalizedStringHooked);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x578912, &GetLocalizedStringHooked);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x57892B, &GetLocalizedStringHooked);
}