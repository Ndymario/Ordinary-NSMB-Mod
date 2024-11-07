#include "SpookyController.hpp"

#include "nsmb/system/function.hpp"

using namespace Lighting;

static Player* getLeftmostPlayer();

SpookyController* SpookyController::instance = nullptr;

SpookyController* SpookyController::getInstance() {
    return instance;
}

void SpookyController::onPrepareResources() {
	if(!Game::vsMode){
		Vec3 spawnPos = Game::getLocalPlayer()->position;
		spawnPos.x - 1000fx;
		chasers[0] = scast<Chaser*>(Actor::spawnActor(92, 0, &spawnPos));
		chasers[0]->currentTarget = Game::getLocalPlayer()->id;
		return;
	}

	if(currVSMode == ChaserVSMode::OneChaser){
		currTarget = Net::getRandom() % (Game::getPlayerCount() - 1);
		Vec3 spawnPos = Game::getPlayer(currTarget)->position;
		spawnPos.x - 1000fx;
		spawnPos.y + 36fx;
		chasers[0] = scast<Chaser*>(Actor::spawnActor(92, 0, &spawnPos));
		chasers[0]->currentTarget = currTarget;
		return;
	}

	if(currVSMode == ChaserVSMode::AllChaser){
		for (s32 i = 0; i < playerCount; i++){
			currTarget = i;
			Vec3 spawnPos = Game::getPlayer(i)->position;
			spawnPos.x - 1000fx;
			spawnPos.y + 36fx;
			chasers[i] = scast<Chaser*>(Actor::spawnActor(92, 0, &spawnPos));
			chasers[i]->currentTarget = currTarget;
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
		delete[] chasers;
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

ncp_hook(0x020A2EF8, 0)
void SpookyController::stageDestroy_hook() {
	// You can remove the condition to destroy on area changes
	if (Scene::nextSceneID != scast<u16>(SceneID::Stage)) {
		instance->onDestroy();
		delete instance;
		instance = nullptr;
	}
}
