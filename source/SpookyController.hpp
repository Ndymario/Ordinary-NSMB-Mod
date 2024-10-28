#pragma once

#include "nsmb.hpp"
#include "nsmb_nitro.hpp"

#include "SpookyChaser.hpp"
#include "NSBTX.hpp"

class SpookyController {
private:
    inline SpookyController() = default;  // Private constructor to prevent multiple instances
    inline ~SpookyController() = default; // Private destructor for proper cleanup (singleton)

	void onPrepareResources();

	void onCreate();
	void onUpdate();
	void onRender();
	void onDestroy();

	void onAreaChange();
	void onBlockHit();

	void waitSpawnChaserState();
	void transitionState();
	void chaseState();

	void spawnChaser();
    void spookyOverlay();     // A static overlay to transition to sp00ky mode
    void spookyPalette();     // Make the tiles look sp00ky
    void unspookyPalette();   // Make the tiles look unsp00ky

	void switchState(void (SpookyController::*updateFunc)());

    static SpookyController* instance; // Static pointer to the singleton instance

	void (*updateFunc)(SpookyController*);
	s8 updateStep;

    /*
    Timers!

    The first time sp00ky happens, the death timer starts at 30 seconds.
    Every subsequent time, the time remaining is somewhere inbetween 30 seconds and 15 seconds.
    */

    s32 spookTimer;     // Time left until sp00ky mode happens
	bool isSpooky;
	s32 transitionDuration;
	bool usingSpookyPalette;

	u16 colorBackup;
	u16* paletteBackup; // Contains the normal palette

	NSBTX staticNsbtx;
	u32 nsbtxTexID[3][4];
	bool isRenderingStatic;

public:
    static SpookyController* getInstance();

    // Singleton access methods
    Chaser* chaser = nullptr; // Chaser pointer to manage the spooky chaser

    s32 deathTimer;   // Timer that anchors the behavior of the actor so it's not too RNG
    s32 suspenseTime;     // The time that triggers "suspense" mode

private:
	// Hooks
	static void stageSetup_hook();
	static void stageUpdate_hook();
	static void stageRender_hook();
	static void stageDestroy_hook();

	static void hitBlock_hook();
	static bool getCoin_hook(s32 playerID);
	static void getScore_hook(s32 playerID, s32 count);
	static bool goalGrab_hook(void* goal);
	static void goalBarrier_hook(void* goal, ActiveCollider* other);
	static void oamLoad_hook();
	static void doNotUpdateSomeDbObjPltt_hook(void* stage);
	static bool applyPowerup_hook(PlayerBase* player, PowerupState powerup);
	static bool startSeq_hook(s32 seqID, bool restart);
	static void startStageThemeSeq_hook(s32 seqID);
	static void playerBeginEnteranceTransition_hook(Player* player, EntranceType type);
};
