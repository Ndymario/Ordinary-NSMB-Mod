#pragma once

#include "nsmb.hpp"
#include "nsmb_nitro.hpp"
#include "NSBTX.hpp"

class SpookyController;

enum ChaserVSMode {
	OneChaser,
	AllChaser
};

class Chaser: public StageEntity {
private:
	void moveTowardsPlayer();

    fx32 playerBuffer = 40fx;

    bool levelOver = false;

public:
    static bool loadResources();
    virtual bool onPrepareResources() override;

    virtual s32 onCreate() override;
    virtual bool updateMain() override;
    virtual s32 onRender() override;
    virtual s32 onDestroy() override;

        static constexpr u16 objectID = 92; //objectID

    static constexpr ObjectInfo objectInfo = {
        0, 0,
        0, 0,
        0, 0,
        0, 0,
        CollisionSwitch::None,
    };

    static constexpr u16 updatePriority = objectID;
    static constexpr u16 renderPriority = objectID;
    static constexpr ActorProfile profile = {&constructObject<Chaser>, updatePriority, renderPriority, loadResources};

    SpookyController* ctrl;

    static Chaser* instance;
    Chaser* getInstance();

	void onBlockHit();

    void waitSpawnChaserState();
	void transitionState();
	void chaseState();

    void spookyOverlay();     // A static overlay to transition to sp00ky mode
    void spookyPalette();     // Make the tiles look sp00ky
    void unspookyPalette();   // Make the tiles look unsp00ky

    void onAreaChange(bool isSpooky);

	void switchState(void (Chaser::*updateFunc)());
    void (*updateFunc)(Chaser*);
	s8 updateStep;

    /*
    Timers!

    The first time sp00ky happens, the death timer starts at 30 seconds.
    Every subsequent time, the time remaining is somewhere inbetween 30 seconds and 15 seconds.
    */

	s32 spookTimer;
	bool isSpooky;
	s32 transitionDuration;
	bool usingSpookyPalette;
	bool isRenderingStatic;

    // Hooks
	static void hitBlock_hook();
	static bool getCoin_hook(s32 playerID);
	static void getScore_hook(s32 playerID, s32 count);
	static bool goalGrab_hook(void* goal);
	static void goalBarrier_hook(void* goal, ActiveCollider* other);
	static void oamLoad_hook();
	static void doNotUpdateSomeDbObjPltt_hook(void* stage);
	static bool applyPowerup_hook(PlayerBase* player, PowerupState powerup);
	static void startSeq_hook(s32 seqID, bool restart);
	static void startStageThemeSeq_hook(s32 seqID);
	static void playerBeginEnteranceTransition_hook(Player* player, EntranceType type);
	static void unpauseResumeMusic();
    static void endLevel();
	static void jrEndLevel();

	NSBTX staticNsbtx;
	u32 nsbtxTexID[3][4];

	u16 colorBackup;
	u16* paletteBackup; // Contains the normal palette
    
    Player* closestPlayer;
    s32 currentTarget = (settings & 0xFF00) >> 8;
    NSBTX spookyNsbtx;
    u8 texID = 0;
    bool resetMusic;
    s32 chaserID = settings & 0xFF;

    ChaserVSMode currVSMode;

    s32 deathTimer;   // Timer that anchors the behavior of the actor so it's not too RNG
    s32 suspenseTime;     // The time that triggers "suspense" mode
};
