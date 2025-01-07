#pragma once
#include <Windows.h>
#include "../Utils.hpp"
#include <unordered_set>

void SetupClassExpansion();
void SetupCustomClassName();
void InitBurstMod();
void UpdateQueuedShots(double& delta);
void InitInfocardEdits();
void InitMiscFixes();
void InitMissileFixes();
void InitAmmoLimit();

extern std::unordered_set<uint> self_detonating_mines;
extern std::unordered_map<uint, float> missile_arming_time;
extern std::unordered_map<uint, float> top_speed_map;
extern std::unordered_map<uint, float> mine_arming_time;
extern std::unordered_map<uint, uint> gun_barrel_counts;


struct ammoStruct
{
	int maxAmmo;
	int launcherMaxStack;
};

extern std::unordered_map<uint, ammoStruct> ammoMap;