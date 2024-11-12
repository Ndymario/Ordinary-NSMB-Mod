#include "SpookyController.hpp"

#include "nsmb/system/function.hpp"

using namespace Lighting;

static u16* getBGExtPltt(u32 destSlotAddr);
static u16* getOBJExtPltt(u32 destSlotAddr);
static u16* getTexPltt(u32 destSlotAddr);
static u16 makeMonochrome(u16 color);
static void makeRangeMonochrome(u16* startAddr, u32 count);
static Player* getLeftmostPlayer();

static void lerpColor(GXRgb& color, GXRgb target, fx32 step);
static void lerpLighting(StageLighting& current, const StageLighting& target, fx32 step);

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

SpookyController* SpookyController::instance = nullptr;

SpookyController* SpookyController::getInstance() {
    return instance;
}

void SpookyController::onPrepareResources() {
	void* nsbtxFile = FS::Cache::loadFile(2088 - 131, false);
	staticNsbtx.setup(nsbtxFile, Vec2(64, 64), Vec2(0, 0), 0, 0);
}

void SpookyController::onCreate() {
	usingSpookyPalette = false;
	isRenderingStatic = false;
	isSpooky = false;
	hasSpawnedForBoss = false;

	paletteBackup = new u16[256 + (256 * 16) + (256 * 32) + 256 + 256];

	onPrepareResources();

	for (u32 row = 0; row < 3; row++) {
		for (u32 col = 0; col < 4; col++) {
			nsbtxTexID[row][col] = Net::getRandom() % 4;
		}
	}

	switchState(&SpookyController::waitSpawnChaserState);

	currentTarget = scast<s32>(Net::getRandom() % 2);
}

void SpookyController::onUpdate() {
	if (!doTicks) {
		return;
	}
	updateFunc(this);
}

void SpookyController::onRender() {
	if (isRenderingStatic) {
		Vec3 scale(1fx);
		Vec3 cameraPos = Vec3(0, 0, 1023fx);
		fx32 cameraPosXStart = Stage::cameraX[Game::localPlayerID];
		fx32 cameraPosYStart = -Stage::cameraY[Game::localPlayerID] - 64.0fx;

		for (u32 row = 0; row < 3; row++) {
			for (u32 col = 0; col < 4; col++) {

				cameraPos.x = cameraPosXStart + (col * 64.0fx);
				cameraPos.y = cameraPosYStart - (row * 64.0fx);

				Game::wrapPosition(cameraPos);

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
	hasSpawnedForBoss = false;
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

	if (levelOver){
		doTicks = false;
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

		if(!Game::vsMode) {
			Stage::setZoom(1.0fx, 0, 0, 0);
			Game::getPlayer(0)->updateLocked = true;
			Game::getPlayer(0)->freezeStage();
		} else {
			Game::getPlayer(0)->updateLocked = true;
			Game::getPlayer(1)->updateLocked = true;
			Game::getPlayer(0)->freezeStage();
			Game::getPlayer(1)->freezeStage();
		}

        transitionDuration = 5 + Net::getRandom() % (50 - 10 + 1);
        spookTimer = transitionDuration;
        updateStep = 1;
        return;
	}
	if (updateStep == Func::Exit) {
		isRenderingStatic = false;
		
		if(!Game::vsMode) {
			Game::getPlayer(0)->updateLocked = false;
			Game::getPlayer(0)->unfreezeStage();
		} else {
			Game::getPlayer(0)->updateLocked = false;
			Game::getPlayer(1)->updateLocked = false;
			Game::getPlayer(0)->unfreezeStage();
			Game::getPlayer(1)->unfreezeStage();
		}

		return;
	}

	if (spookTimer > 0) {
		if (spookTimer == transitionDuration / 2) {
			if (usingSpookyPalette) {
				if(Game::getLocalPlayer()->playerID == currentTarget){
					unspookyPalette();
				}
			} else {
				if(Game::getLocalPlayer()->playerID == currentTarget){
					spookyPalette();
				}
			}
		}
		spookTimer--;
	} else {
		if (usingSpookyPalette) {
			usingSpookyPalette = false;
			
			if(!Game::vsMode){
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
			}

			if(Game::getLocalPlayer()->playerID == currentTarget){
				setLightingFromProfile(rcast<u8(*)(u8)>(0x0201f0d8)(Game::getLocalPlayer()->viewID));
			}

			switchState(&SpookyController::waitSpawnChaserState);
			if (levelOver){
				doTicks = false;
			}
			
		} else {
			usingSpookyPalette = true;

			if(Game::getLocalPlayer()->playerID == currentTarget){
				setLightingFromProfile(11);
			}

			switchState(&SpookyController::chaseState);
		}
	}
}

void SpookyController::chaseState() {
	if (updateStep == Func::Init) {
    	deathTimer = 1200;
    	suspenseTime = 900;
		isSpooky = true;
		spawnChaser();

		updateStep = 1;
		return;
	}

	if(currentProfileID != 11 && Game::getLocalPlayer()->playerID == currentTarget){
		setLightingFromProfile(11);
	}

	if (updateStep == Func::Exit) {
		if (chaser != nullptr) {
			chaser->Base::destroy();
		}
		isSpooky = false;
		return;
	}

	if(!Game::vsMode){
		if (chaser == nullptr) {
			// If the chaser gets despawned in a boss room, disable spooky mode as the boss is defeated
			StageView* view = StageView::get(Game::getLocalPlayer()->viewID, nullptr);
			if (view->bgmID == 80 || view->bgmID == 81 || view->bgmID == 82 || view->bgmID == 86){
				if(hasSpawnedForBoss){
					endLevel();
					return;
				}
				hasSpawnedForBoss = true;
			}
			spawnChaser();
		}
	}
}

void SpookyController::spawnChaser() {
	Player* leftmostPlayer = getLeftmostPlayer();
	Vec3 spawnPos = leftmostPlayer->position;
	spawnPos.x - 1000fx;
	s32 settings = currentTarget;
    chaser = scast<Chaser*>(Actor::spawnActor(92, settings, &spawnPos)); // Spawn the chaser actor
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

// ------------ Hooks ------------
ncp_hook(0x0209E13C, 0)
void SpookyController::trySpawnBattleStar_hook(Player* player, int isPlayerDead, int wasGroundPound){
	if(instance == nullptr){
		return;
	}

	if(!instance->isSpooky){
		return;
	}

	if(instance->chaser == nullptr){
		return;
	}

	if(player->playerID == instance->currentTarget){
		return;
	}

	instance->onBlockHit();
	instance->currentTarget ^= 1;
	instance->chaser->currentTarget = instance->currentTarget;

	if(instance->currentTarget == getWinningPlayerID(Game::getPlayerBattleStars(0), Game::getPlayerBattleStars(1))){
		instance->spookTimer /= 2;
	}

	if(wasGroundPound != 0){
		instance->deathTimer /= 2;
	}
}

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
	if (instance != nullptr){
		instance->onUpdate();
	}
	// Do our setup in stageUpdate in MvsL
	if (instance == nullptr && Game::vsMode){
		instance = new SpookyController();
		instance->onCreate();
	}
	
}

ncp_hook(0x020A2E50, 0)
void SpookyController::stageRender_hook() {
	if(instance != nullptr){
		instance->onRender();
	}
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
	if (instance != nullptr && !Game::vsMode) {
		instance->onBlockHit();
	}
}

ncp_jump(0x02020354)
bool SpookyController::getCoin_hook(s32 playerID) {
	if (instance != nullptr && instance->isSpooky && playerID == instance->currentTarget) {
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

ncp_call(0x020C0530, 0)
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

ncp_hook(0x021409B8, 28)	// Bowser Jr KO state
void SpookyController::jrEndLevel(){
	//	Don't let Jr remove spooky mode in the final fight
	if (Entrance::getEntranceSpawnType(0) != PlayerSpawnType::TransitNormal){
		endLevel();
	}
}

void SpookyController::endLevel(){
	instance->onBlockHit();
	instance->levelOver = true;
}

ncp_set_hook(0x02118030, 10, SpookyController::endLevel);	// Player::goalBeginPoleGrab()
ncp_set_hook(0x0211BE18, 10, SpookyController::endLevel);	// Player::goalBeginMegaClear()
ncp_set_hook(0x0211A9A8, 10, SpookyController::endLevel);	// Player::bossDefeatTransitState()
ncp_set_hook(0x0211F2A4, 10, SpookyController::endLevel);	// Player::beginBossDefeatCutscene()
ncp_set_hook(0x0211A370, 10, SpookyController::endLevel);	// Player::bossVictoryTransitState()
//ncp_set_hook(0x02133454, 16, SpookyController::endLevel);	// Mummypokey KO state
ncp_set_hook(0x021303C4, 18, SpookyController::endLevel);	// Cheepskipper KO state
ncp_set_hook(0x021328E4, 14, SpookyController::endLevel);	// Mega Goomba KO state
ncp_set_hook(0x021307BC, 15, SpookyController::endLevel);	// Petey Piranha KO state
ncp_set_hook(0x021332EC, 19, SpookyController::endLevel);	// Montey Tank KO state
ncp_set_hook(0x0212FD14, 17, SpookyController::endLevel);	// Lakithunder KO state

ncp_jump(0x02011f04)
void SpookyController::startStageThemeSeq_hook(s32 seqID){
	if (instance != nullptr && instance->isSpooky && !Game::getLocalPlayer()->defeatedFlag){
		return;
	} else {
		startStageThemeSeq_backup(seqID);
	}
}

ncp_jump(0x02011e7c)
void SpookyController::startSeq_hook(s32 seqID, bool restart){
	if (instance != nullptr && instance->isSpooky && !Game::getLocalPlayer()->defeatedFlag){
		return;
	} else {
		startSeq_backup(seqID, restart);
	}
}

ncp_call(0x020a2514, 0)
void SpookyController::unpauseResumeMusic(){
	if(instance->isSpooky && !Game::vsMode){
		return;
	}

	SND::pauseBGM(0);
}

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

void SpookyController::lerpColor(GXRgb& color, GXRgb target, fx32 step) {

	if (color == target) return;

	u16 c[3];
	for (u32 i=0; i<3; i++) {
		c[i] = color >> i*5 & 31;
		c[i] = Math::mul((target >> i*5 & 31) - c[i], step) + c[i];
	}
	color = GX_RGB(c[0],c[1],c[2]);
}

void SpookyController::lerpLighting(StageLighting& current, const StageLighting& target, fx32 step) {

	for (u32 i=0; i<4; i++) {
		DirLight& light = current.lights[i];
		const DirLight& tLight = target.lights[i];
		
		if (!light.active) {
			if (tLight.active) light.active = true;
			else continue;			
		}		

		lerpColor(light.color, tLight.color, step);

		Vec3 vec = Vec3(light.direction);
		vec.lerp(Vec3(tLight.direction),step);
		vec = vec.normalize();

		NNS_G3dGlbLightColor(scast<GXLightId>(i),light.color);
		NNS_G3dGlbLightVector(scast<GXLightId>(i),vec.x,vec.y,vec.z);
	}

	lerpColor(current.diffuse, target.diffuse, step);
	lerpColor(current.ambient, target.ambient, step);
	lerpColor(current.emission, target.emission, step);
	lerpColor(current.specular, target.specular, step);

	setMatLighting(current);
}

bool SpookyController::getWinningPlayerID(s32 starsP0, s32 starsP1){
	if (starsP0 < starsP1) {
		return starsP0 < starsP1;
	}
	if (starsP0 == starsP1) {
		return scast<bool>(Net::getRandom() % 2);
	}
	return starsP0 < starsP1;
};
