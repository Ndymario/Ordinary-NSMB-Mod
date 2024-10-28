#include "SpookyController.hpp"

#include "nsmb/system/function.hpp"

static u16* getBGExtPltt(u32 destSlotAddr);
static u16* getOBJExtPltt(u32 destSlotAddr);
static u16* getTexPltt(u32 destSlotAddr);
static u16 makeMonochrome(u16 color);
static void makeRangeMonochrome(u16* startAddr, u32 count);
static Player* getLeftmostPlayer();

bool Game_addPlayerCoin_backup(s32 playerID);
void Game_addPlayerScore_backup(s32 playerID, s32 count);
void GoalFlag_onPoleBarrierCollided_backup(void* goal, ActiveCollider* other);
bool applyPowerup_backup(PlayerBase* player, PowerupState powerup);
bool startSeq_backup(s32 seqID, bool restart);
void startStageThemeSeq_backup(s32 seqID);

asm(R"(
	GoalFlag_updateGoalGrab = 0x0213042C
	func20BE084 = 0x020BE084
)");

extern "C" {
	bool GoalFlag_updateGoalGrab(void* goal);
	void func20BE084(void* stage);
}

static inline bool Stage_hasLevelFinished() { return *rcast<u32*>(0x020CA8C0) & ~0x80010000; }

SpookyController* SpookyController::instance = nullptr;

SpookyController* SpookyController::getInstance() {
    return instance;
}

void SpookyController::onPrepareResources() {
	void* nsbtxFile = FS::Cache::loadFile(2089 - 131, false);
	staticNsbtx.setup(nsbtxFile, Vec2(64, 64), Vec2(0, 0), 0, 0);
}

void SpookyController::onCreate() {
	usingSpookyPalette = false;
	isRenderingStatic = false;
	isSpooky = false;

	paletteBackup = new u16[256 + (256 * 16) + (256 * 32) + 256 + 256];

	onPrepareResources();

	for (u32 row = 0; row < 3; row++) {
		for (u32 col = 0; col < 4; col++) {
			nsbtxTexID[row][col] = Wifi::getRandom() % 4;
		}
	}

	switchState(&SpookyController::waitSpawnChaserState);
}

void SpookyController::onUpdate() {
	if (Stage_hasLevelFinished()) {
		return;
	}
	updateFunc(this);
}

void SpookyController::onRender() {
	if (isRenderingStatic) {
		Vec3 scale(1fx);
		Vec3 cameraPos = Vec3(0, 0, 512.0fx);
		fx32 cameraPosXStart = Stage::cameraX[Game::localPlayerID];
		fx32 cameraPosYStart = -Stage::cameraY[Game::localPlayerID] - 64.0fx;

		for (u32 row = 0; row < 3; row++) {
			for (u32 col = 0; col < 4; col++) {

				cameraPos.x = cameraPosXStart + (col * 64.0fx);
				cameraPos.y = cameraPosYStart - (row * 64.0fx);

				u32 texID = nsbtxTexID[row][col];

				staticNsbtx.setTexture(texID);
				staticNsbtx.render(cameraPos, scale);

				nsbtxTexID[row][col] = (texID + 1) % 4;
			}
		}
	}
}

void SpookyController::onDestroy() {
	if (paletteBackup != nullptr) {
		delete[] paletteBackup;
	}
}

void SpookyController::onAreaChange() {
	onPrepareResources();
	if (isSpooky) {
		spookyPalette();
	}
}

void SpookyController::onBlockHit() {
	if (isSpooky) {
		switchState(&SpookyController::transitionState);
	}
}

void SpookyController::waitSpawnChaserState() {
	if (updateStep == Func::Init) {
		spookTimer = 600;
		updateStep = 1;
		return;
	}
	if (updateStep == Func::Exit) {
		return;
	}

	if (spookTimer > 0) {
		spookTimer--;
	} else {
		switchState(&SpookyController::transitionState);
	}
}

void SpookyController::transitionState() {
	if (updateStep == Func::Init) {
        isRenderingStatic = true;
        Stage::freezeFlag = 1;

		for (s32 i = 0; i < Game::getPlayerCount(); i++) {
			Stage::setZoom(1.0fx, 0, i, 0);
			Game::getPlayer(i)->updateLocked = true;
		}

        transitionDuration = 10 + Wifi::getRandom() % (50 - 10 + 1);
        spookTimer = transitionDuration;
        updateStep = 1;
        return;
	}
	if (updateStep == Func::Exit) {
		isRenderingStatic = false;
		
		for (s32 i = 0; i < Game::getPlayerCount(); i++) {
			Game::getPlayer(i)->updateLocked = false;
		}

		Stage::freezeFlag = 0;

		return;
	}

	if (spookTimer > 0) {
		Stage::freezeFlag = 1;
		
		if (spookTimer == transitionDuration / 2) {
			if (usingSpookyPalette) {
				unspookyPalette();
			} else {
				spookyPalette();
			}
		}
		spookTimer--;
	} else {
		if (usingSpookyPalette) {
			usingSpookyPalette = false;
			SND::pauseBGM(false);
			switchState(&SpookyController::waitSpawnChaserState);
		} else {
			usingSpookyPalette = true;
			switchState(&SpookyController::chaseState);
		}
	}
}

void SpookyController::chaseState() {
	if (updateStep == Func::Init) {
    	deathTimer = 1200;
    	suspenseTime = 900;
		isSpooky = true;
		SND::pauseBGM(true);
		SND::playSFXUnique(380);
		spawnChaser();

		updateStep = 1;
		return;
	}

	if (updateStep == Func::Exit) {
		if (chaser != nullptr) {
			chaser->Base::destroy();
		}
		isSpooky = false;
		return;
	}

	if (chaser == nullptr) {
		spawnChaser();
	}
}

void SpookyController::spawnChaser() {
	Player* leftmostPlayer = getLeftmostPlayer();
    chaser = scast<Chaser*>(Actor::spawnActor(92, 0, &leftmostPlayer->position)); // Spawn the chaser actor
}

void SpookyController::spookyPalette() {
	// TOP SCREEN
	u16* objColors = rcast<u16*>(HW_OBJ_PLTT);
	MI_CpuCopyFast(objColors, paletteBackup, 256 * 2);
	makeRangeMonochrome(objColors, 256);

	GX_BeginLoadOBJExtPltt();
	u16* extObjColors = getOBJExtPltt(0);
	MI_CpuCopyFast(extObjColors, &paletteBackup[256], 256 * 16 * 2);
	makeRangeMonochrome(extObjColors, 256 * 16);
	GX_EndLoadOBJExtPltt();

	GX_BeginLoadBGExtPltt();
	u16* bgColors = getBGExtPltt(0x4000);
	MI_CpuCopyFast(bgColors, &paletteBackup[256 + (256 * 16)], 256 * 32 * 2);
	makeRangeMonochrome(bgColors, 256 * 32);
	GX_EndLoadBGExtPltt();

	/*GX_BeginLoadTexPltt();
	u16* texPlttColors = getTexPltt(0);
	makeRangeMonochrome(texPlttColors, 256 * 64);
	GX_EndLoadTexPltt();*/

	// BOTTOM SCREEN
	u16* dbBgColors = rcast<u16*>(HW_DB_BG_PLTT);
	MI_CpuCopyFast(dbBgColors, &paletteBackup[256 + (256 * 16) + (256 * 32)], 256 * 2);
	makeRangeMonochrome(dbBgColors, 256);

	u16* dbObjColors = rcast<u16*>(HW_DB_OBJ_PLTT);
	MI_CpuCopyFast(dbObjColors, &paletteBackup[256 + (256 * 16) + (256 * 32) + 256], 256 * 2);
	makeRangeMonochrome(dbObjColors, 256);
}

void SpookyController::unspookyPalette() {
	// TOP SCREEN
	u16* objColors = rcast<u16*>(HW_OBJ_PLTT);
	MI_CpuCopyFast(paletteBackup, objColors, 256 * 2);

	GX_BeginLoadOBJExtPltt();
	u16* extObjColors = getOBJExtPltt(0);
	MI_CpuCopyFast(&paletteBackup[256], extObjColors, 256 * 16 * 2);
	GX_EndLoadOBJExtPltt();

	GX_BeginLoadBGExtPltt();
	u16* bgColors = getBGExtPltt(0x4000);
	MI_CpuCopyFast(&paletteBackup[256 + (256 * 16)], bgColors, 256 * 32 * 2);
	GX_EndLoadBGExtPltt();

	// BOTTOM SCREEN
	u16* dbBgColors = rcast<u16*>(HW_DB_BG_PLTT);
	MI_CpuCopyFast(&paletteBackup[256 + (256 * 16) + (256 * 32)], dbBgColors, 256 * 2);

	u16* dbObjColors = rcast<u16*>(HW_DB_OBJ_PLTT);
	MI_CpuCopyFast(&paletteBackup[256 + (256 * 16) + (256 * 32) + 256], dbObjColors, 256 * 2);
}

void SpookyController::switchState(void (SpookyController::*updateFunc)()) {
	auto updateFuncRaw = ptmf_cast(updateFunc);

	if (this->updateFunc != updateFuncRaw) {
		if (this->updateFunc) {
			this->updateStep = Func::Exit;
			this->updateFunc(this);
		}

		this->updateFunc = updateFuncRaw;

		this->updateStep = Func::Init;
		this->updateFunc(this);
	}
}

/*struct StageLight {
	VecFx16 direction;
	GXRgb color;
	GXRgb diffuse;
	GXRgb ambient;
	GXRgb emission;
};

ncp_over(0x020C2544, 0)
static const StageLight newViewLightingTable[] = {
	{ {0, -1.0fxs, -1.0fxs}, GX_RGB(15, 15, 15), GX_RGB(31, 31, 31), GX_RGB(0, 0, 0), GX_RGB(0, 0, 0) }
};*/

// ------------ Hooks ------------

ncp_hook(0x0215ECA8, 54)
void SpookyController::stageSetup_hook() {	
	if (instance == nullptr) {
		instance = new SpookyController();
		instance->onCreate();
	} else {
		instance->onAreaChange();
	}
}

ncp_hook(0x020A2C88, 0)
void SpookyController::stageUpdate_hook() {
	instance->onUpdate();
}

ncp_hook(0x020A2E50, 0)
void SpookyController::stageRender_hook() {
	instance->onRender();
}

ncp_hook(0x020A2EF8, 0)
void SpookyController::stageDestroy_hook() {
	// You can remove the condition to destroy on area changes
	if (Scene::nextSceneID != scast<u16>(SceneID::Stage)) {
		instance->onDestroy();
		delete instance;
		instance = nullptr;
	}
}

ncp_hook(0x0209E7D0, 0)
void SpookyController::hitBlock_hook() {
	if (instance != nullptr) {
		instance->onBlockHit();
	}
}

ncp_jump(0x02020354)
bool SpookyController::getCoin_hook(s32 playerID) {
	if (instance != nullptr && instance->isSpooky) {
		if (Game::playerCoins[playerID] > 0) {
			Game::playerCoins[playerID]--;
			return true;
		} else {
			return false;
		}
	} else {
		return Game_addPlayerCoin_backup(playerID);
	}
}

ncp_jump(0x02020300)
void SpookyController::getScore_hook(s32 playerID, s32 count) {
	if (instance != nullptr && instance->isSpooky) {
		if ((playerID & 0xFFFFFFFE) == 0) {
			s32 newScore = Game::playerScore[playerID] - count;
			if (newScore < 0) {
				newScore = 0;
			}
  			Game::playerScore[playerID] = newScore;
  			Save::mainSave.tempScore = newScore;
  		}
	} else {
		Game_addPlayerScore_backup(playerID, count);
	}
}

ncp_call(0x021302C8, 12)
bool SpookyController::goalGrab_hook(void* goal) {
	if (instance != nullptr && instance->isSpooky) {
		return false;
	}
	return GoalFlag_updateGoalGrab(goal);
}

ncp_jump(0x021307C0, 12)
void SpookyController::goalBarrier_hook(void* goal, ActiveCollider* other) {
	if (instance != nullptr && !instance->isSpooky) {
		GoalFlag_onPoleBarrierCollided_backup(goal, other);
	}
}

ncp_call(0x02006F78)
void SpookyController::oamLoad_hook() {
	if (instance != nullptr && instance->isRenderingStatic) {
		OAM::reset();
	}
	OAM::load();
}

ncp_call(0x020C0530, main)
void SpookyController::doNotUpdateSomeDbObjPltt_hook(void* stage) {
	if (instance != nullptr && (instance->isSpooky || instance->isRenderingStatic)) {
		return;
	}
	func20BE084(stage);
}

ncp_jump(0x0212b9f8, 11)
bool SpookyController::applyPowerup_hook(PlayerBase* player, PowerupState powerup)
{
	if (instance != nullptr && instance->isSpooky){
		instance->onBlockHit();
		return true;
	} else {
		return applyPowerup_backup(player, powerup);
	}
}

// ncp_jump(0x02011f04)
// void SpookyController::startStageThemeSeq_hook(s32 seqID){
// 	if (instance != nullptr && instance->isSpooky){
// 		return;
// 	} else {
// 		startStageThemeSeq_backup(seqID);
// 	}
// }

// ncp_jump(0x02011e7c)
// bool SpookyController::startSeq_hook(s32 seqID, bool restart){
// 	if (instance != nullptr && instance->isSpooky){
// 		return;
// 	} else {
// 		return startSeq_backup(seqID, restart);
// 	}
// }

// ------------ Backups ------------

NTR_NAKED bool Game_addPlayerCoin_backup(s32 playerID) {asm(R"(
	PUSH    {R4,LR}
	B       0x02020358
)");}

NTR_NAKED void Game_addPlayerScore_backup(s32 playerID, s32 count) {asm(R"(
	BICS    R2, R0, #1
	B       0x02020304
)");}

NTR_NAKED void GoalFlag_onPoleBarrierCollided_backup(void* goal, ActiveCollider* other) {asm(R"(
	SUB     SP, SP, #0x10
	B       0x021307C4
)");}

NTR_NAKED bool applyPowerup_backup(PlayerBase* player, PowerupState powerup)
{asm(R"(
        stmdb	sp!,{lr}
        B       0x0212b9fc
    )");}

NTR_NAKED void startStageThemeSeq_backup(s32 seqID)
{asm(R"(
        stmdb	sp!,{lr}
        B       0x02011f08
    )");}

NTR_NAKED bool startSeq_backup(s32 seqID, bool restart)
{asm(R"(
        stmdb	sp!,{lr}
        B       0x02011e80
    )");}

// ------------ Utils ------------

static u16* getBGExtPltt(u32 destSlotAddr) {
	return rcast<u16*>(*rcast<u32*>(0x02094274) + destSlotAddr - *rcast<u32*>(0x02094270));
}

static u16* getOBJExtPltt(u32 destSlotAddr) {
	return rcast<u16*>(*rcast<u32*>(0x02094268) + destSlotAddr);
}

static u16* getTexPltt(u32 destSlotAddr) {
	return rcast<u16*>(*rcast<u32*>(0x02094280) + destSlotAddr);
}

static u16 makeMonochrome(u16 color) {
  	u8 r, g, b;
  	r = (color >> 0) & 0x1F;
  	g = (color >> 5) & 0x1F;
  	b = (color >> 10) & 0x1F;
	u16 gray = (scast<u16>(r) + scast<u16>(g) + scast<u16>(b)) / 3;
	return (gray << 10) | (gray << 5) | gray;
}

static void makeRangeMonochrome(u16* startAddr, u32 count) {
	for (u32 i = 0; i < count; i++) {
		startAddr[i] = makeMonochrome(startAddr[i]);
	}
}

static Player* getLeftmostPlayer() {
    Player* leftmostPlayer = Game::getPlayer(0);

    for (s32 i = 1; i < Game::getPlayerCount(); i++) {
        Player* currentPlayer = Game::getPlayer(i);
        if (currentPlayer->position.x < leftmostPlayer->position.x) {
            leftmostPlayer = currentPlayer;
        }
    }

    return leftmostPlayer;
}
