#include <windows.h>
#include <format>
#include <fstream>
#include <thread>

#include "nya_dx9_hookbase.h"
#include "nya_commonhooklib.h"
#include "nya_commonmath.h"
#include "nfsc.h"
#include "chloemenulib.h"

bool bChallengeSeriesMode = false;

#include "util.h"
#include "d3dhook.h"
#include "../MostWantedTimeTrialGhosts/timetrials.h"
#include "../MostWantedTimeTrialGhosts/verification.h"
#include "hooks/carrender.h"

#ifdef TIMETRIALS_CHALLENGESERIES
#include "../MostWantedTimeTrialGhosts/challengeseries.h"
#include "hooks/customevents.h"

void SetChallengeSeriesMode(bool on) {
	bChallengeSeriesMode = on;
	if (on) {
		bViewReplayMode = false;
		bOpponentsOnly = true;
		nNitroType = NITRO_ON;
		nSpeedbreakerType = NITRO_ON;
		ApplyCustomEventsHooks();
	}
	else {
		bOpponentsOnly = false;
	}
}
#endif

ISimable* VehicleConstructHooked(Sim::Param params) {
	DLLDirSetter _setdir;

	auto vehicle = (VehicleParams*)params.mSource;
	if (vehicle->carClass == DRIVER_RACER) {
		// copy player car for all opponents
		auto player = GetLocalPlayerVehicle();
		vehicle->matched = nullptr;
		vehicle->pvehicle = Attrib::Instance(Attrib::FindCollection(Attrib::StringHash32("pvehicle"), player->GetVehicleKey()), 0, nullptr);
		vehicle->customization = (VehicleCustomizations*)player->GetCustomizations();

		// do a config save in every loading screen
		DoConfigSave();
	}
	return PVehicle::Construct(params);
}

// this function runs a million times for some reason
void MainLoop() {
	static double simTime = -1;

	if (!Sim::Exists() || Sim::GetState() != Sim::STATE_ACTIVE) {
		simTime = -1;
		return;
	}
	if (simTime == Sim::GetTime()) return;
	simTime = Sim::GetTime();

	DLLDirSetter _setdir;
	TimeTrialLoop();
}

void RenderLoop() {
	VerifyTimers();
	TimeTrialRenderLoop();
}

auto Game_NotifyRaceFinished = (void(*)(ISimable*))0x660D20;
void OnEventFinished(ISimable* a1) {
	bool isPlayer = (!a1 || a1 == GetLocalPlayerSimable()) && !GetLocalPlayerVehicle()->IsDestroyed();
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x660E0F, isPlayer ? 0x6BE1F0 : 0x6BE2CC); // don't call DriftScoring::Finalize from other racers finishing

	Game_NotifyRaceFinished(a1);

	if (isPlayer) {
		DLLDirSetter _setdir;
		OnFinishRace();

		// do a config save when finishing a race
		DoConfigSave();
	}
}

float TrafficDensityHooked() {
	return 0.0;
}

auto RaceCountdownHooked_orig = (void(__thiscall*)(void*))nullptr;
void __thiscall RaceCountdownHooked(void* pThis) {
	OnRaceRestart();
	RaceCountdownHooked_orig(pThis);
}

bool __thiscall FinishTimeHooked(DALRacer* pThis, float* out, GRacerInfo* racer) {
	if (bViewReplayMode) {
		return DALRacer::GetRaceTime(pThis, out, racer);
	}
	else {
		*out = nLastFinishTime * 0.001;
		return true;
	}
}

bool __stdcall RaceTimeHooked(float* out, GRacerInfo* racer) {
	*out = (nGlobalReplayTimerNoCountdown / 120.0);
	return true;
}

bool __thiscall GetIsCrewRaceHooked(GRaceStatus* pThis) {
	return false;
}

DriftScoreReport* __thiscall GetDriftScoreHooked(DALRacer* pThis, int racerId) {
	auto racer = DALRacer::GetRacerInfo(pThis, racerId);
	if (racer == &GRaceStatus::fObj->mRacerInfo[0]) {
		return DALRacer::GetDriftScoreReport(pThis, racerId);
	}

	static DriftScoreReport tmp;
	tmp.totalPoints = 0;

	int opponentId = racerId-3;
	if (OpponentGhosts.size() <= opponentId) return &tmp;

	auto& opponent = OpponentGhosts[opponentId];
	auto tick = opponent.GetCurrentTick();
	if (tick >= opponent.aTicks.size()) {
		tmp.totalPoints = opponent.nFinishPoints;
	}
	else {
		tmp.totalPoints = opponent.aTicks[tick].v4.driftPoints;
	}
	return &tmp;
}

bool __thiscall GetRacerNameHooked(DALRacer* pThis, char* out, int outLen, int racerId) {
	auto raceType = GRaceParameters::GetRaceType(GRaceStatus::fObj->mRaceParms);
	if (raceType != GRace::kRaceType_DriftRace && raceType != GRace::kRaceType_CanyonDrift) {
		return DALRacer::GetName(pThis, out, outLen, racerId);
	}

	auto racer = DALRacer::GetRacerInfo(pThis, racerId);
	if (!racer) {
		int opponentId = racerId-3;
		if (opponentId < 0) {
			strcpy_s(out, outLen, GetUserProfile()->mName);
			return true;
		}

		if (OpponentGhosts.size() <= opponentId) return false;
		strcpy_s(out, outLen, OpponentGhosts[opponentId].sPlayerName.c_str());
		return true;
	}
	strcpy_s(out, outLen, racer->mName);
	return true;
}

// drift scores are so fucked up i have no idea how any of this works
/*int GetPlayerDriftScore(int id) {
	if (id == 0) return DALRacer::GetDriftScoreReport(nullptr, 0)->totalPoints;

	int opponentId = id-1;
	if (OpponentGhosts.size() <= opponentId) return 0;

	auto& opponent = OpponentGhosts[opponentId];
	auto tick = opponent.GetCurrentTick();
	if (tick >= opponent.aTicks.size()) return opponent.nFinishPoints;
	return opponent.aTicks[tick].v4.driftPoints;
}

int GetPlayerDriftPosition(int id) {
	std::vector<int> players;
	int numRacers = 1 + OpponentGhosts.size();
	for (int i = 0; i < numRacers; i++) {
		players.push_back(i);
	}
	std::sort(players.begin(), players.end(), [](const int a, const int b) { return GetPlayerDriftScore(a) > GetPlayerDriftScore(b); });

	for (auto& player : players) {
		if (player == id) return (&player - &players[0]) + 1;
	}
	return 1;
}

int GetPlayerByDriftPosition(int id) {
	std::vector<int> players;
	int numRacers = 1 + OpponentGhosts.size();
	for (int i = 0; i < numRacers; i++) {
		players.push_back(i);
	}
	std::sort(players.begin(), players.end(), [](const int a, const int b) { return GetPlayerDriftScore(a) > GetPlayerDriftScore(b); });
	return players[id-1];
}

DriftScoreReport* __thiscall GetDriftScoreHooked(DALRacer* pThis, int racerId) {
	// 0 and 1 return the player always
	// 2 onwards returns player at ranking

	if (racerId < 2) {
		return DALRacer::GetDriftScoreReport(pThis, racerId);
	}

	static DriftScoreReport tmp;

	auto localPlayer = DALRacer::GetDriftScoreReport(pThis, 0);
	auto report = DALRacer::GetDriftScoreReport(pThis, racerId);
	if (!report) report = &tmp;
	auto ply = GetPlayerByDriftPosition(racerId-1);
	if (ply != 0 && report != localPlayer) {
		report->totalPoints = GetPlayerDriftScore(ply);
	}
	return report;
}

bool __thiscall GetRacerPositionHooked(DALRacer* pThis, int* out, int racerId) {
	auto raceType = GRaceParameters::GetRaceType(GRaceStatus::fObj->mRaceParms);
	if (raceType != GRace::kRaceType_DriftRace && raceType != GRace::kRaceType_CanyonDrift) {
		return DALRacer::GetRacerPosition(pThis, out, racerId);
	}

	if (racerId < 2) {
		*out = GetPlayerDriftPosition(0);
		return true;
	}
	return racerId-1;
}

bool __thiscall GetRacerNameHooked(DALRacer* pThis, char* out, int outLen, int racerId) {
	auto raceType = GRaceParameters::GetRaceType(GRaceStatus::fObj->mRaceParms);
	if (raceType != GRace::kRaceType_DriftRace && raceType != GRace::kRaceType_CanyonDrift) {
		return DALRacer::GetName(pThis, out, outLen, racerId);
	}

	if (racerId < 2) {
		return DALRacer::GetName(pThis, out, outLen, racerId);
	}

	auto ply = GetPlayerByDriftPosition(racerId-1);
	if (ply == 0) {
		strcpy_s(out, outLen, GRaceStatus::fObj->mRacerInfo[0].mName);
		return true;
	}
	else {
		auto opponentId = ply-1;
		if (OpponentGhosts.size() <= opponentId) return false;
		strcpy_s(out, outLen, OpponentGhosts[opponentId].sPlayerName.c_str());
		return true;
	}
}*/

int __thiscall GetRaceTypeForMusicHooked(GRaceParameters* pThis) {
	return GRace::kRaceType_Tollbooth;
}

bool StartGridHooked(int index, UMath::Vector3* pos, UMath::Vector3* dir, void* startMarker) {
	return GStartGrid::GetGrid(0, pos, dir, startMarker);
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			if (NyaHookLib::GetEntryPoint() != 0x47E926) {
				MessageBoxA(nullptr, "Unsupported game version! Make sure you're using v1.3 (.exe size of 7217152 bytes)", "nya?!~", MB_ICONERROR);
				return TRUE;
			}

			gDLLPath = std::filesystem::current_path();
			GetCurrentDirectoryW(MAX_PATH, gDLLDir);

			NyaHooks::SimServiceHook::Init();
			NyaHooks::SimServiceHook::aFunctions.push_back(MainLoop);
			NyaHooks::LateInitHook::Init();
			NyaHooks::LateInitHook::aPreFunctions.push_back(FileIntegrity::VerifyGameFiles);
			NyaHooks::LateInitHook::aFunctions.push_back([]() {
				NyaHooks::PlaceD3DHooks();
				NyaHooks::D3DEndSceneHook::aPreFunctions.push_back(CollectPlayerPos);
				NyaHooks::D3DEndSceneHook::aFunctions.push_back(D3DHookMain);
				NyaHooks::D3DEndSceneHook::aFunctions.push_back(CheckPlayerPos);
				NyaHooks::D3DResetHook::aFunctions.push_back(OnD3DReset);

				ApplyVerificationPatches();

				*(float*)0x9DB360 = 1.0 / 120.0; // set sim framerate
				*(float*)0x9EBB6C = 1.0 / 120.0; // set sim max framerate
				*(void**)0xA9E764 = (void*)&VehicleConstructHooked;
				if (GetModuleHandleA("NFSCLimitAdjuster.asi") || std::filesystem::exists("NFSCLimitAdjuster.ini")) {
					MessageBoxA(nullptr, "Incompatible mod detected! Please remove NFSCLimitAdjuster.asi from your game before using this mod.", "nya?!~", MB_ICONERROR);
					exit(0);
				}
			});

			Game_NotifyRaceFinished = (void(*)(ISimable*))(*(uint32_t*)(0x669E49 + 1));
			NyaHookLib::Patch(0x669E49 + 1, &OnEventFinished);

			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x445087, &TrafficDensityHooked);

			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4A6F25, &GetDriftScoreHooked);
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4CEAD6, &GetRacerNameHooked);
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4CF258, &FinishTimeHooked);
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4D2CD2, &RaceTimeHooked);

			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x66987D, &StartGridHooked);

			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x53E246, &GetRaceTypeForMusicHooked);

			NyaHookLib::Patch<uint8_t>(0x492F51, 0xEB); // disable CameraMover::MinGapCars

			// remove career mode and multiplayer
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x83C486, 0x5B2373); // career
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x83C5BE, 0x5B2373); // custom match
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x83C60C, 0x5B2373); // quick match
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x83C65A, 0x5B2373); // reward cards

			RaceCountdownHooked_orig = (void(__thiscall*)(void*))NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x6863E3, &RaceCountdownHooked);

			NyaHookLib::Patch(0x9C4F80, 0x43EE30); // replace AIVehicleRacecar update with AIVehicleEmpty update
			//NyaHookLib::Fill(0x6DA51B, 0x90, 5); // don't run PVehicle::UpdateListing when changing driver class

			NyaHookLib::Patch(0xA611A4, 0); // tollbooth -> sprint
			NyaHookLib::Patch(0xA6119C, 1); // lap knockout -> circuit
			NyaHookLib::Patch(0xA611AC, 0); // speedtrap -> sprint
			NyaHookLib::Patch(0xA611EC, 0); // canyon -> sprint
			NyaHookLib::Patch(0xA611BC, 0); // checkpoint -> sprint
			NyaHookLib::Patch(0xA611FC, 1); // pursuit knockout -> circuit
			NyaHookLib::Patch<uint8_t>(0x655960, 0xC3); // remove speedtraps

			NyaHookLib::Patch<uint8_t>(0x65118A, 0xEB); // disable SpawnCop, fixes dday issues

			// increase max racers to 30
			NyaHookLib::Patch<uint8_t>(0x668EC9, 0xEB);
			NyaHookLib::Patch<uint8_t>(0x668EEC, 0xEB);

			NyaHookLib::Patch<uint16_t>(0x63F65E, 0x9090); // don't spawn boss characters
			NyaHookLib::Patch<uint16_t>(0x63F66C, 0x9090); // don't spawn boss characters

			static int tmpWorldDetail = 0;
			NyaHookLib::Patch(0x711121, &tmpWorldDetail); // remove props regardless of world lod

			NyaHookLib::Patch<uint8_t>(0x6BE6D4, 1); // make drift scoring consistently higher regardless of controller type

			ApplyCarRenderHooks();

			ChloeMenuLib::RegisterMenu("Time Trial Ghosts", &DebugMenu);

			DoConfigLoad();

			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x61BA70, &GetIsCrewRaceHooked); // remove wingmen

#ifdef TIMETRIALS_CHALLENGESERIES
			SetChallengeSeriesMode(true);
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x83C570, 0x5B2373); // remove quick race
#else
			UnlockAllThings = true;
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x83C522, 0x5B2373); // remove challenge series
#endif

			WriteLog("Mod initialized");
		} break;
		default:
			break;
	}
	return TRUE;
}