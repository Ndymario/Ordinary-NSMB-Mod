#include "nsmb.hpp"
#include "SpookyBoss.hpp"
#include "SpookyController.hpp"

ncp_over(0x020c560c, 0) const ObjectInfo objectInfo = SpookyBoss::objectInfo; //Stage Object ID 44 (use this in the editor)
ncp_over(0x02039a34) static constexpr const ActorProfile* profile = &SpookyBoss::profile; //objectID 46

static void copyChunk(u32 source, u32 destination);
static inline void refreshChunks();

bool SpookyBoss::loadResources() {
    FS::Cache::loadFile(bossModelID, false);
    FS::Cache::loadFile(idleAnimationID, false);
    return true;
}

s32 SpookyBoss::onCreate(){
    SpookyController* instance = SpookyController::getInstance();
    
    // Disable the spooky controller
    instance->switchState(&SpookyController::waitSpawnChaserState);
    instance->doTicks = false;
    instance->finalBoss = true;

    // Remove spooky mode if needed
    if(instance->usingSpookyPalette){
        instance->switchState(&SpookyController::transitionState);
    }

    loadResources();

    void* bossModelFile = FS::Cache::getFile(bossModelID);
    void* idleAnmFile = FS::Cache::getFile(idleAnimationID);

    model.create(bossModelFile, idleAnmFile, 0, 0, 0);

    fogFlag = false;
	alpha = 0xff;
	renderOffset = {0, 0};
	rotationTranslation = {180, 0};

    model.init(1, FrameCtrl::Looping, 1fx, 0);

    initialPosition = position;

    switchState(&SpookyBoss::idleState);

	return true;
}

s32 SpookyBoss::onDestroy(){
	return true;
}

bool SpookyBoss::updateMain(){
    updateAnimation();
    updateFunc(this);
	return true;
}

void SpookyBoss::idleState(){
    if (updateStep == Func::Init) {
        updateStep = 1;
        return;
    }

    if (updateStep == 1){
        Vec3 playerLocation = player->position;
        if (position.x - playerLocation.x <= 16fx * fightActivationRange){
            switchState(&SpookyBoss::introAnimation);
        }
    }

    if (updateStep == Func::Exit) {
        return;
    }
}

void SpookyBoss::introAnimation(){
    if (updateStep == Func::Init) {
        player->beginCutscene(true);
        updateStep = 1;
        return;
    }

    if(updateStep == 1){
        model.init(1, FrameCtrl::Type::Standard, 1fx, 0);

        if(model.frameController.finished()){
            SpookyController* instance = SpookyController::getInstance();
            instance->switchState(&SpookyController::transitionState);
            copyChunk(9, 1);
            refreshChunks();
            updateStep++;
        }
    }

    if (updateStep == 2){
        Log() << "continue animation";

        model.init(1, FrameCtrl::Type::Standard, 1fx, 0);

        if(model.frameController.finished()){
            switchState(&SpookyBoss::mimicState);
        }
    }

    if (updateStep == Func::Exit) {
        position.y += 16fx * 5;
        position.x -= 16fx * 7;
        player->endCutscene();
        player->position.y -= 16fx;
        Vec2 bumpVelocity = Vec2(-4fx, 0);
        player->doBossBump(bumpVelocity);
        SND::playBGM(21, false);
        return;
    }
}

void SpookyBoss::mimicState(){
    if (updateStep == Func::Init) {
        zone = StageZone::get(0, zoneRect);
        model.init(0, FrameCtrl::Type::Standard, 1fx, 0);
        mirrorPosition = position;
        updateStep = 1;
        bossTimer = 200;
        return;
    }

    if (updateStep == Func::Exit) {
        return;
    }

    // We always want Mario/Luigi to look at the boss
    Game::setPlayerLookAtPosition(Vec3(position.x, position.y + 32fx, 0));
    Game::setPlayerLookingAtTarget(true);

    if (player->position.x < zoneRect->x || player->position.x > zoneRect->x + zoneRect->width || player->position.y > zoneRect->y || player->position.y < zoneRect->y - zoneRect->height){
            SpookyController* instance = SpookyController::getInstance();
            instance->staticDuration = 10;
            player->position = Vec3(initialPosition.x - 4fx, initialPosition.y, player->position.z);
            player->doPlayerBump(Vec2(-2fx, 0), true);
    }
    

    // Mimic mode: Ordinary Luigi is simply mirroring the player's position
    if (updateStep == 1){
        position.lerp(Vec3(mirrorPosition.x * 2 - player->position.x, position.y, position.z), 0.05fx);
        if (bossTimer <= 0){
            bossTimer = 100;
            updateStep = 2;
            player->spawnActor(280, 0, &position);
        }
    }

    if (updateStep == 2){
        if (bossTimer <= 0){
            updateStep = 3;
        }
    }

    if (updateStep == 3){
        Vec3 mirroredPos = mirrorPosition;
        mirroredPos.x = mirrorPosition.x * 2 - player->position.x;
        mirroredPos.y = player->position.y;
        position.lerp(mirroredPos, 0.05fx);
    }

    // Decrement the timer
    bossTimer--;
}

void SpookyBoss::switchState(void (SpookyBoss::*updateFunc)()) {
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

// Utility functions
static void copyChunk(u32 srcChunk, u32 dstChunk){
#ifdef NTR_DEBUG 
    u16 chunkCount = rcast<u16*>(Stage::stageLayout)[0x232];
    if (srcChunk >= chunkCount || dstChunk >= chunkCount) {
        Log::print("ERROR: Chunk index out of bounds. (src: %u, dst: %u, count: %u)", srcChunk, dstChunk, scast<u32>(chunkCount));
        OS_Terminate();
        return;
    }
#endif
    void** chunks = rcast<void**>(0x020CAFE0);
    MI_CpuCopyFast(chunks[srcChunk], chunks[dstChunk], 0x200);
}

static inline void refreshChunks(){
    rcast<u16*>(Stage::stageLayout)[0x46E/2] = rcast<u16*>(Stage::stageLayout)[0x46C/2];
}