#pragma once

#include "nsmb.hpp"
#include "nsmb_nitro.hpp"

#include "SpookyChaser.hpp"
#include "NSBTX.hpp"

#include "lighting/extralighting.hpp"

using namespace Lighting;

class SpookyController {
public:
    inline SpookyController() = default;  // Private constructor to prevent multiple instances
    inline ~SpookyController() = default; // Private destructor for proper cleanup (singleton)

	void onPrepareResources();

	void onCreate();
	void onUpdate();
	void onRender();
	void onDestroy();

	void onAreaChange();

	void spawnChaser();

    static SpookyController* instance;

	void spookyPalette();     // Make the tiles look sp00ky
    void unspookyPalette();   // Make the tiles look unsp00ky

	u16 colorBackup;
	u16* paletteBackup; // Contains the normal palette

    static SpookyController* getInstance();

    Chaser* chasers[2]; // Chaser pointer to manage the spooky chaser
	s32 playerCount;
	s32 currTarget;

	ChaserVSMode currVSMode;
	bool hasSpawnedForBoss = false;

	bool changeLighting();

private:
	// Hooks
	static void stageSetup_hook();
	static void stageUpdate_hook();
	static void stageRender_hook();
	static void stageDestroy_hook();

	bool doTicks = true;

	bool* isSpooky;
};
