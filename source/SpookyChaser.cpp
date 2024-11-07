/*
The "chaser" is the sp00ky Luigi that the player can see chasing them when sp00ky mode is activated.
*/

#include "SpookyChaser.hpp"
#include "SpookyController.hpp"

#include "nsmb/system/function.hpp"
using namespace Lighting;

ncp_over(0x020C619C, 0) const ObjectInfo objectInfo = Chaser::objectInfo; //Stage Actor ID 192
ncp_over(0x02039AEC) static constexpr const ActorProfile* profile = &Chaser::profile; //objectID 92

static u16* getBGExtPltt(u32 destSlotAddr);
static u16* getOBJExtPltt(u32 destSlotAddr);
static u16* getTexPltt(u32 destSlotAddr);
static u16 makeMonochrome(u16 color);
static void makeRangeMonochrome(u16* startAddr, u32 count);
bool Game_addPlayerCoin_backup(s32 playerID);
void Game_addPlayerScore_backup(s32 playerID, s32 count);
void GoalFlag_onPoleBarrierCollided_backup(void* goal, ActiveCollider* other);
bool applyPowerup_backup(PlayerBase* player, PowerupState powerup);
bool startSeq_backup(s32 seqID, bool restart);
void startStageThemeSeq_backup(s32 seqID);

Chaser* Chaser::instance = nullptr;

Chaser* Chaser::getInstance(){
    return instance;
}

asm(R"(
	GoalFlag_updateGoalGrab = 0x0213042C
	func20BE084 = 0x020BE084
)");

extern "C" {
	bool GoalFlag_updateGoalGrab(void* goal);
	void func20BE084(void* stage);
}

bool Chaser::onPrepareResources(){
    void* nsbtxFile1;
    if(!Game::getPlayerCharacter(Game::getLocalPlayer()->playerID)){
        nsbtxFile1 = FS::Cache::loadFile(2089 - 131, false);
    } else {
        nsbtxFile1 = FS::Cache::loadFile(2090 - 131, false);
    }
    texID = 0;
	spookyNsbtx.setup(nsbtxFile1, Vec2(64, 64), Vec2(0, 0), 0, 0);

    void* nsbtxFile2;
    nsbtxFile2 = FS::Cache::loadFile(2088 - 131, false);
	staticNsbtx.setup(nsbtxFile2, Vec2(64, 64), Vec2(0, 0), 0, 0);

    instance = this;

    return 1;
}

bool Chaser::loadResources() {
    return true;
}

// Code that runs the first time the Actor loads in
s32 Chaser::onCreate() {
	ctrl = SpookyController::getInstance();

    onPrepareResources();
	loadResources();

    usingSpookyPalette = false;
    isRenderingStatic = false;
    isSpooky = false;

    for (u32 row = 0; row < 3; row++) {
		for (u32 col = 0; col < 4; col++) {
			nsbtxTexID[row][col] = Net::getRandom() % 4;
		}
	}

    viewOffset = Vec2(50, 50);
    activeSize = Vec2(1000.0, 3000.0);

    resetMusic = true;

    switchState(&Chaser::waitSpawnChaserState);
    return 1;
}

// Code that runs every frame
bool Chaser::updateMain() {
    spookyNsbtx.setTexture(texID);
    spookyNsbtx.setPalette(texID);
    if(deathTimer % 5 == 0){
        texID = (texID + 1) % 6;
    }
	moveTowardsPlayer();

    deathTimer -= 1;

    if (deathTimer <= 0) {
        closestPlayer->damage(*this, 0, 0, PlayerDamageType::Death);
		deathTimer = 0;
        onDestroy();
    }

    return 1;
}

void Chaser::waitSpawnChaserState() {
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
		switchState(&Chaser::transitionState);
	}
}

void Chaser::transitionState() {
	if (updateStep == Func::Init) {
        isRenderingStatic = true;

        if(!Game::vsMode){
            for (s32 i = 0; i < Game::getPlayerCount(); i++) {
                Stage::setZoom(1.0fx, 0, i, 0);
                Game::getPlayer(i)->updateLocked = true;
                Game::getPlayer(i)->freezeStage();
            }
        }

        transitionDuration = 5 + Net::getRandom() % (50 - 10 + 1);
        spookTimer = transitionDuration;
        updateStep = 1;
        return;
	}
	if (updateStep == Func::Exit) {
		isRenderingStatic = false;
		
		for (s32 i = 0; i < Game::getPlayerCount(); i++) {
			Game::getPlayer(i)->updateLocked = false;
			Game::getPlayer(i)->unfreezeStage();
		}

		return;
	}

	if (spookTimer > 0) {
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
			
			StageView* view = StageView::get(Game::getLocalPlayer()->viewID, nullptr);
			SND::pauseBGM(false);
			if(!Game::getLocalPlayer()->defeatedFlag){
				if(Entrance::getEntranceSpawnType(0) == PlayerSpawnType::TransitNormal){
					SND::playBGM(21, false);
				} else if(SND::bgmSeqID == 7 || view->bgmID == 80 || view->bgmID == 81 || view->bgmID == 82 || view->bgmID == 86){
					SND::playBGM(7, false);
				} else {
					SND::playBGM(view->bgmID, false);
				}
			}
			setLightingFromProfile(rcast<u8(*)(u8)>(0x0201f0d8)(Game::getLocalPlayer()->viewID));
			switchState(&Chaser::waitSpawnChaserState);
			
		} else {
			usingSpookyPalette = true;
			setLightingFromProfile(11);
			switchState(&Chaser::chaseState);
		}
	}
}

void Chaser::chaseState() {
	if (updateStep == Func::Init) {
    	deathTimer = 1200;
    	suspenseTime = 900;
		isSpooky = true;

		updateStep = 1;
		return;
	}

	if(currentProfileID != 11){
		setLightingFromProfile(11);
	}

	if (updateStep == Func::Exit) {
		this->Base::destroy();
		isSpooky = false;
		return;
	}
}

void Chaser::onAreaChange(bool isSpooky){
    onPrepareResources();
    if (isSpooky){
        spookyPalette();
    }
}

void Chaser::unspookyPalette() {
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

void Chaser::switchState(void (Chaser::*updateFunc)()) {
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

void Chaser::moveTowardsPlayer() {
    if (Game::vsMode){
        closestPlayer = Game::getPlayer(currentTarget);
    } else {
        closestPlayer = getClosestPlayer(nullptr, nullptr);
    }

    if (deathTimer >= suspenseTime) {
        position.x = closestPlayer->position.x - deathTimer * 1.0fx;
    } else {
        position.x = closestPlayer->position.x - playerBuffer - deathTimer * 0.25fx;
    }

    position.y = closestPlayer->position.y - 16fx;
    position.z = closestPlayer->position.z;
    wrapPosition(position);
    
    if(resetMusic){
        SND::pauseBGM(true);
        SND::playSFXUnique(380);
        resetMusic = false;
    }
}

s32 Chaser::onRender() {
    if (isRenderingStatic) {
		Vec3 scale(1fx);
		Vec3 cameraPos = Vec3(0, 0, 1023fx);
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
    Vec3 scale(1fx);
    spookyNsbtx.render(position, scale);
    return 1;
}

// Code runs when this Actor is being destroyed
s32 Chaser::onDestroy() {
    SpookyController* controller = SpookyController::getInstance();
	controller->chasers[chaserID] = nullptr;
    return 1;
}

void Chaser::onBlockHit() {
	if (isSpooky) {
		switchState(&Chaser::transitionState);
	}
}

void Chaser::spookyPalette() {
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

// Hooks
ncp_hook(0x0209E7D0, 0)
void Chaser::hitBlock_hook() {
	if (instance != nullptr) {
		instance->onBlockHit();
	}
}

ncp_jump(0x02020354)
bool Chaser::getCoin_hook(s32 playerID) {
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
void Chaser::getScore_hook(s32 playerID, s32 count) {
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
bool Chaser::goalGrab_hook(void* goal) {
	if (instance != nullptr && instance->isSpooky) {
		return false;
	}
	return GoalFlag_updateGoalGrab(goal);
}

ncp_jump(0x021307C0, 12)
void Chaser::goalBarrier_hook(void* goal, ActiveCollider* other) {
	if (instance != nullptr && !instance->isSpooky) {
		GoalFlag_onPoleBarrierCollided_backup(goal, other);
	}
}

ncp_call(0x02006F78)
void Chaser::oamLoad_hook() {
	if (instance != nullptr && instance->isRenderingStatic) {
		OAM::reset();
	}
	OAM::load();
}

ncp_call(0x020C0530, 0)
void Chaser::doNotUpdateSomeDbObjPltt_hook(void* stage) {
	if (instance != nullptr && (instance->isSpooky || instance->isRenderingStatic)) {
		return;
	}
	func20BE084(stage);
}

ncp_jump(0x0212b9f8, 11)
bool Chaser::applyPowerup_hook(PlayerBase* player, PowerupState powerup)
{
	if (instance != nullptr && instance->isSpooky){
		instance->onBlockHit();
		return true;
	} else {
		return applyPowerup_backup(player, powerup);
	}
}

ncp_jump(0x02011f04)
void Chaser::startStageThemeSeq_hook(s32 seqID){
	if (instance != nullptr && instance->isSpooky && !Game::getLocalPlayer()->defeatedFlag){
		return;
	} else {
		startStageThemeSeq_backup(seqID);
	}
}

ncp_jump(0x02011e7c)
void Chaser::startSeq_hook(s32 seqID, bool restart){
	if (instance != nullptr && instance->isSpooky && !Game::getLocalPlayer()->defeatedFlag){
		return;
	} else {
		startSeq_backup(seqID, restart);
	}
}

ncp_call(0x020a2514, 0)
void Chaser::unpauseResumeMusic(){
	if(instance != nullptr && instance->isSpooky){
		return;
	}

	SND::pauseBGM(0);
}

ncp_hook(0x021409B8, 28)	// Bowser Jr KO state
void Chaser::jrEndLevel(){
	//	Don't let Jr remove spooky mode in the final fight
	if (Entrance::getEntranceSpawnType(0) != PlayerSpawnType::TransitNormal){
		endLevel();
	}
}

void Chaser::endLevel(){
    if(instance != nullptr){
        instance->onBlockHit();
	    instance->levelOver = true;
    }
}

ncp_set_hook(0x02118030, 10, Chaser::endLevel);	// Player::goalBeginPoleGrab()
ncp_set_hook(0x0211A9A8, 10, Chaser::endLevel);	// Player::bossDefeatTransitState()
ncp_set_hook(0x0211F2A4, 10, Chaser::endLevel);	// Player::beginBossDefeatCutscene()
ncp_set_hook(0x0211A370, 10, Chaser::endLevel);	// Player::bossVictoryTransitState()
//ncp_set_hook(0x02133454, 16, Chaser::endLevel);	// Mummypokey KO state
ncp_set_hook(0x021303C4, 18, Chaser::endLevel);	// Cheepskipper KO state
ncp_set_hook(0x021328E4, 14, Chaser::endLevel);	// Mega Goomba KO state
ncp_set_hook(0x021307BC, 15, Chaser::endLevel);	// Petey Piranha KO state
ncp_set_hook(0x021332EC, 19, Chaser::endLevel);	// Montey Tank KO state
ncp_set_hook(0x0212FD14, 17, Chaser::endLevel);	// Lakithunder KO state

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
        stmdb	sp!,{r4,lr}
        B       0x02011e80
    )");}

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
