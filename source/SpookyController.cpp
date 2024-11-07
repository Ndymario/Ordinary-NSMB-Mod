#include "SpookyController.hpp"
#include "nsmb/system/function.hpp"

static u16* getBGExtPltt(u32 destSlotAddr);
static u16* getOBJExtPltt(u32 destSlotAddr);
static u16* getTexPltt(u32 destSlotAddr);
static u16 makeMonochrome(u16 color);
static void makeRangeMonochrome(u16* startAddr, u32 count);

SpookyController* SpookyController::instance = nullptr;

SpookyController* SpookyController::getInstance() {
    return instance;
}

void SpookyController::onPrepareResources() {
	if(!Game::vsMode){
		Vec3 spawnPos = Game::getLocalPlayer()->position;
		spawnPos.x - 1000fx;
		u32 settings = (Game::getLocalPlayer()->playerID << 8) | 0;
		chasers[0] = scast<Chaser*>(Actor::spawnActor(92, settings, &spawnPos));
		return;
	}

	if(currVSMode == ChaserVSMode::OneChaser){
		currTarget = Net::getRandom() % (Game::getPlayerCount() - 1);
		Vec3 spawnPos = Game::getPlayer(currTarget)->position;
		spawnPos.x - 1000fx;
		spawnPos.y + 36fx;
		u32 settings = (currTarget << 8) | 0;
		chasers[0] = scast<Chaser*>(Actor::spawnActor(92, 0, &spawnPos));
		return;
	}

	if(currVSMode == ChaserVSMode::AllChaser){
		for (s32 i = 0; i < playerCount; i++){
			currTarget = i;
			Vec3 spawnPos = Game::getPlayer(i)->position;
			spawnPos.x - 1000fx;
			spawnPos.y + 36fx;
			u32 settings = (currTarget << 8) | i;
			chasers[i] = scast<Chaser*>(Actor::spawnActor(92, 0, &spawnPos));
		}
	}
	
}

void SpookyController::onCreate() {
	hasSpawnedForBoss = false;

	playerCount = Game::getPlayerCount();

	paletteBackup = new u16[256 + (256 * 16) + (256 * 32) + 256 + 256];

	onPrepareResources();
}

void SpookyController::onUpdate() {
	if (!doTicks) {
		return;
	}
}

void SpookyController::onDestroy() {
	if (paletteBackup != nullptr) {
		delete[] paletteBackup;
	}
}

void SpookyController::onAreaChange() {
	hasSpawnedForBoss = false;
	for (s32 i = 0; i < playerCount; i++){
		if(chasers[i] != nullptr){
			chasers[i]->onAreaChange(isSpooky[i]);
		}
	}
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
	if(instance != nullptr){
		instance->onUpdate();
	}

	// Do our "setup" in the update step for MvsL
	if (Game::vsMode){
		if(instance == nullptr){
			instance = new SpookyController();
			instance->onCreate();
		}
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
