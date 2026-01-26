#include <windows.h>
#include <format>
#include <fstream>

#include "nya_dx9_hookbase.h"
#include "nya_commonhooklib.h"
#include "nya_commonmath.h"
#include "nfsc.h"
#include "chloemenulib.h"

bool bChallengeSeriesMode = false;

#include "util.h"
#include "d3dhook.h"
#include "../MostWantedTimeTrialGhosts/timetrials.h"
#include "hooks/carrender.h"

#ifdef TIMETRIALS_CHALLENGESERIES
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

void DebugMenu() {
	ChloeMenuLib::BeginMenu();

	QuickValueEditor("Show Inputs While Driving", bShowInputsWhileDriving);
	QuickValueEditor("Player Name Override", sPlayerNameOverride, sizeof(sPlayerNameOverride));

	if (TheGameFlowManager.CurrentGameFlowState == GAMEFLOW_STATE_IN_FRONTEND) {
		QuickValueEditor("Replay Viewer", bViewReplayMode);
		if (bViewReplayMode) {
			if (DrawMenuOption(std::format("Replay Viewer Ghost - {}", bViewReplayTargetTime ? "Target Time" : "Personal Best"))) {
				bViewReplayTargetTime = !bViewReplayTargetTime;
			}
		}
		if (bChallengeSeriesMode) {
			const char* difficultyNames[] = {
				"Easy",
				"Normal",
				"Hard",
				"Very Hard",
			};
			const char* difficultyDescs[] = {
				"Easier ghosts",
				"Average ghosts",
				"Faster community ghosts",
				"Speedrunners' community ghosts",
			};
			if (DrawMenuOption(std::format("Difficulty - {}", difficultyNames[nDifficulty]))) {
				ChloeMenuLib::BeginMenu();
				for (int i = 0; i < NUM_DIFFICULTY; i++) {
					if (DrawMenuOption(difficultyNames[i], difficultyDescs[i])) {
						nDifficulty = (eDifficulty)i;
					}
				}
				ChloeMenuLib::EndMenu();
			}
			if (nDifficulty != DIFFICULTY_EASY) {
				QuickValueEditor("Show Target Ghost Only", bChallengesOneGhostOnly);
				QuickValueEditor("Show Personal Ghost", bChallengesPBGhost);
			}
		}
		else {
			QuickValueEditor("Opponent Ghosts Only", bOpponentsOnly);

			if (bViewReplayMode) bOpponentsOnly = false;

			if (DrawMenuOption("NOS")) {
				ChloeMenuLib::BeginMenu();
				if (DrawMenuOption("Off")) {
					nNitroType = NITRO_OFF;
				}
				if (DrawMenuOption("On")) {
					nNitroType = NITRO_ON;
				}
				if (DrawMenuOption("Infinite")) {
					nNitroType = NITRO_INF;
				}
				ChloeMenuLib::EndMenu();
			}

			if (DrawMenuOption("Speedbreaker")) {
				ChloeMenuLib::BeginMenu();
				if (DrawMenuOption("Off")) {
					nSpeedbreakerType = NITRO_OFF;
				}
				if (DrawMenuOption("On")) {
					nSpeedbreakerType = NITRO_ON;
				}
				if (DrawMenuOption("Infinite")) {
					nSpeedbreakerType = NITRO_INF;
				}
				ChloeMenuLib::EndMenu();
			}
		}
	}

	if (DrawMenuOption("Ghost Visuals")) {
		ChloeMenuLib::BeginMenu();
		if (DrawMenuOption("Hidden")) {
			nGhostVisuals = GHOST_HIDE;
		}
		if (DrawMenuOption("Shown")) {
			nGhostVisuals = GHOST_SHOW;
		}
		if (DrawMenuOption("Hide Too Close")) {
			nGhostVisuals = GHOST_HIDE_NEARBY;
		}
		ChloeMenuLib::EndMenu();
	}

	auto status = GRaceStatus::fObj;
	if (status && status->mRaceParms) {
		DrawMenuOption(std::format("Race - {}", GRaceParameters::GetEventID(status->mRaceParms)));
	}

	ChloeMenuLib::EndMenu();
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
	// todo is this needed in carbon?
	//g_WorldLodLevel = std::min(g_WorldLodLevel, 2); // force world detail to one lower than max for props

	if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING) return;
	if (IsInLoadingScreen() || IsInNIS()) return;

	DisplayLeaderboard();

	if (!ShouldGhostRun()) return;

	if (bViewReplayMode) {
		auto ghost = bChallengeSeriesMode && bViewReplayTargetTime && !OpponentGhosts.empty() ? &OpponentGhosts[0] : &PlayerPBGhost;

		auto tick = ghost->GetCurrentTick();
		if (ghost->aTicks.size() > tick) {
			DisplayInputs(&ghost->aTicks[tick].v1.inputs);
		}
	}
	else if (bShowInputsWhileDriving) {
		auto inputs = GetPlayerControls(GetLocalPlayerVehicle());
		DisplayInputs(&inputs);
	}

	DisplayPlayerNames();
}

auto Game_NotifyRaceFinished = (void(*)(ISimable*))0x660D20;
void OnEventFinished(ISimable* a1) {
	Game_NotifyRaceFinished(a1);

	if ((!a1 || a1 == GetLocalPlayerSimable()) && !GetLocalPlayerVehicle()->IsDestroyed()) {
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

			GetCurrentDirectoryW(MAX_PATH, gDLLDir);

			NyaHooks::PlaceD3DHooks();
			NyaHooks::D3DEndSceneHook::aFunctions.push_back(D3DHookMain);
			NyaHooks::D3DResetHook::aFunctions.push_back(OnD3DReset);
			NyaHooks::SimServiceHook::Init();
			NyaHooks::SimServiceHook::aFunctions.push_back(MainLoop);
			NyaHooks::LateInitHook::Init();
			NyaHooks::LateInitHook::aFunctions.push_back([]() {
				*(float*)0x9DB360 = 1.0 / 120.0; // set sim framerate
				*(float*)0x9EBB6C = 1.0 / 120.0; // set sim max framerate
				*(void**)0xA9E764 = (void*)&VehicleConstructHooked;
				if (GetModuleHandleA("NFSCLimitAdjuster.asi")) {
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

			NyaHookLib::Fill(0x6DA51B, 0x90, 5); // don't run PVehicle::UpdateListing when changing driver class

			NyaHookLib::Patch(0xA611A4, 0); // tollbooth -> sprint
			NyaHookLib::Patch(0xA6119C, 1); // lap knockout -> circuit
			NyaHookLib::Patch(0xA611AC, 0); // speedtrap -> sprint
			NyaHookLib::Patch(0xA611EC, 0); // canyon -> sprint
			NyaHookLib::Patch(0xA611BC, 0); // checkpoint -> sprint
			NyaHookLib::Patch(0xA611FC, 1); // pursuit knockout -> circuit

			NyaHookLib::Patch<uint8_t>(0x65118A, 0xEB); // disable SpawnCop, fixes dday issues

			// increase max racers to 30
			NyaHookLib::Patch<uint8_t>(0x668EC9, 0xEB);
			NyaHookLib::Patch<uint8_t>(0x668EEC, 0xEB);

			NyaHookLib::Patch<uint16_t>(0x63F65E, 0x9090); // don't spawn boss characters
			NyaHookLib::Patch<uint16_t>(0x63F66C, 0x9090); // don't spawn boss characters

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